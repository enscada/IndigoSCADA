/*
 *  sv_receiver.c
 *
 *  Copyright 2015-2018 Michael Zillgith
 *
 *  This file is part of libIEC61850.
 *
 *  libIEC61850 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libIEC61850 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  See COPYING file for the complete license text.
 */

#include "stack_config.h"

#include "libiec61850_platform_includes.h"

#include "hal_ethernet.h"
#include "hal_thread.h"
#include "ber_decode.h"
#include "ber_encoder.h"

#include "sv_subscriber.h"

#ifndef DEBUG_SV_SUBSCRIBER
#define DEBUG_SV_SUBSCRIBER 1
#endif

#define ETH_BUFFER_LENGTH 1518

#define ETH_P_SV 0x88ba

struct sSVReceiver {
    bool running;
    bool stopped;

    bool checkDestAddr; /* option: check destination address (additionally to AppID) to identify application */

    char* interfaceId;

    uint8_t* buffer;
    EthernetSocket ethSocket;

    LinkedList subscriberList;

#if (CONFIG_MMS_THREADLESS_STACK == 0)
    Semaphore subscriberListLock;
#endif

};

struct sSVSubscriber {
    uint8_t ethAddr[6];
    uint16_t appId;

    SVUpdateListener listener;
    void* listenerParameter;
};

struct sSVSubscriber_ASDU {

    char* svId;
    char* datSet;

    uint8_t* smpCnt;
    uint8_t* confRev;
    uint8_t* refrTm;
    uint8_t* smpSynch;
    uint8_t* smpMod;
    uint8_t* smpRate;

    int dataBufferLength;
    uint8_t* dataBuffer;
};


SVReceiver
SVReceiver_create(void)
{
    SVReceiver self = (SVReceiver) GLOBAL_CALLOC(1, sizeof(struct sSVReceiver));

    if (self != NULL) {
        self->subscriberList = LinkedList_create();
        self->buffer = (uint8_t*) GLOBAL_MALLOC(ETH_BUFFER_LENGTH);

        self->checkDestAddr = false;

#if (CONFIG_MMS_THREADLESS_STACK == 0)
        self->subscriberListLock = Semaphore_create(1);
#endif
    }

    return self;
}

void
SVReceiver_setInterfaceId(SVReceiver self, const char* interfaceId)
{
    if (self->interfaceId != NULL)
        GLOBAL_FREEMEM(self->interfaceId);

    self->interfaceId = StringUtils_copyString(interfaceId);
}

void
SVReceiver_disableDestAddrCheck(SVReceiver self)
{
    self->checkDestAddr = false;
}

void
SVReceiver_enableDestAddrCheck(SVReceiver self)
{
    self->checkDestAddr = false;
}

void
SVReceiver_addSubscriber(SVReceiver self, SVSubscriber subscriber)
{
#if (CONFIG_MMS_THREADLESS_STACK == 0)
    Semaphore_wait(self->subscriberListLock);
#endif

    LinkedList_add(self->subscriberList, (void*) subscriber);

#if (CONFIG_MMS_THREADLESS_STACK == 0)
    Semaphore_post(self->subscriberListLock);
#endif
}

void
SVReceiver_removeSubscriber(SVReceiver self, SVSubscriber subscriber)
{
#if (CONFIG_MMS_THREADLESS_STACK == 0)
    Semaphore_wait(self->subscriberListLock);
#endif

    LinkedList_remove(self->subscriberList, (void*) subscriber);

#if (CONFIG_MMS_THREADLESS_STACK == 0)
    Semaphore_post(self->subscriberListLock);
#endif
}

static void
svReceiverLoop(void* threadParameter)
{
    SVReceiver self = (SVReceiver) threadParameter;

    self->running = true;
    self->stopped = false;

    SVReceiver_startThreadless(self);

    while (self->running) {

        if (SVReceiver_tick(self) == false)
            Thread_sleep(1);
    }

    SVReceiver_stopThreadless(self);

    self->stopped = true;
}


void
SVReceiver_start(SVReceiver self)
{
    Thread thread = Thread_create((ThreadExecutionFunction) svReceiverLoop, (void*) self, true);

    if (thread != NULL) {
        if (DEBUG_SV_SUBSCRIBER)
            printf("SV_SUBSCRIBER: SV receiver started for interface %s\n", self->interfaceId);

        Thread_start(thread);
    }
    else {
        if (DEBUG_SV_SUBSCRIBER)
            printf("SV_SUBSCRIBER: Starting SV receiver failed for interface %s\n", self->interfaceId);
    }
}

bool
SVReceiver_isRunning(SVReceiver self)
{
    return self->running;
}


void
SVReceiver_stop(SVReceiver self)
{
    self->running = false;

    while (self->stopped == false)
        Thread_sleep(1);
}

void
SVReceiver_destroy(SVReceiver self)
{
    LinkedList_destroyDeep(self->subscriberList,
            (LinkedListValueDeleteFunction) SVSubscriber_destroy);

#if (CONFIG_MMS_THREADLESS_STACK == 0)
        Semaphore_destroy(self->subscriberListLock);
#endif

    GLOBAL_FREEMEM(self->buffer);
    GLOBAL_FREEMEM(self);
}

EthernetSocket
SVReceiver_startThreadless(SVReceiver self)
{
    if (self->interfaceId == NULL)
        self->ethSocket = Ethernet_createSocket(CONFIG_ETHERNET_INTERFACE_ID, NULL);
    else
        self->ethSocket = Ethernet_createSocket(self->interfaceId, NULL);

    Ethernet_setProtocolFilter(self->ethSocket, ETH_P_SV);

    self->running = true;
    
    return self->ethSocket;
}

void
SVReceiver_stopThreadless(SVReceiver self)
{
    Ethernet_destroySocket(self->ethSocket);

    self->running = false;
}

static void
parseASDU(SVReceiver self, SVSubscriber subscriber, uint8_t* buffer, int length)
{
    int bufPos = 0;
    int svIdLength = 0;
    int datSetLength = 0;

    struct sSVSubscriber_ASDU asdu;
    memset(&asdu, 0, sizeof(struct sSVSubscriber_ASDU));

    while (bufPos < length) {
        int elementLength;

        uint8_t tag = buffer[bufPos++];

        bufPos = BerDecoder_decodeLength(buffer, &elementLength, bufPos, length);
        if (bufPos < 0) {
            if (DEBUG_SV_SUBSCRIBER) printf("SV_SUBSCRIBER: Malformed message: failed to decode BER length tag!\n");
            return;
        }

        switch (tag) {

        case 0x80:
            asdu.svId = (char*) (buffer + bufPos);
            svIdLength = elementLength;
            break;

        case 0x81:
            asdu.datSet = (char*) (buffer + bufPos);
            datSetLength = elementLength;
            break;

        case 0x82:
            asdu.smpCnt = buffer + bufPos;
            break;

        case 0x83:
            asdu.confRev = buffer + bufPos;
            break;

        case 0x84:
            asdu.refrTm = buffer + bufPos;
            break;

        case 0x85:
            asdu.smpSynch = buffer + bufPos;
            break;

        case 0x86:
            asdu.smpRate = buffer + bufPos;
            break;

        case 0x87:
            asdu.dataBuffer = buffer + bufPos;
            asdu.dataBufferLength = elementLength;
            break;

        case 0x88:
            asdu.smpMod = buffer + bufPos;
            break;

        default: /* ignore unknown tag */
            if (DEBUG_SV_SUBSCRIBER) printf("SV_SUBSCRIBER: found unknown tag %02x\n", tag);
            break;
        }

        bufPos += elementLength;
    }

    if (asdu.svId != NULL)
        asdu.svId[svIdLength] = 0;
    if (asdu.datSet != NULL)
        asdu.datSet[datSetLength] = 0;
    
    if (DEBUG_SV_SUBSCRIBER) {
        printf("SV_SUBSCRIBER:   SV ASDU: ----------------\n");
        printf("SV_SUBSCRIBER:     DataLength: %d\n", asdu.dataBufferLength);
        printf("SV_SUBSCRIBER:     SvId: %s\n", asdu.svId);
        printf("SV_SUBSCRIBER:     SmpCnt: %d\n", SVSubscriber_ASDU_getSmpCnt(&asdu));
        printf("SV_SUBSCRIBER:     ConfRev: %d\n", SVSubscriber_ASDU_getConfRev(&asdu));
        
        if (SVSubscriber_ASDU_hasDatSet(&asdu))
            printf("SV_SUBSCRIBER:     DatSet: %s\n", asdu.datSet);
        if (SVSubscriber_ASDU_hasRefrTm(&asdu))
            printf("SV_SUBSCRIBER:     RefrTm: %lu\n", SVSubscriber_ASDU_getRefrTmAsMs(&asdu));
        if (SVSubscriber_ASDU_hasSmpMod(&asdu))
            printf("SV_SUBSCRIBER:     SmpMod: %d\n", SVSubscriber_ASDU_getSmpMod(&asdu));
        if (SVSubscriber_ASDU_hasSmpRate(&asdu))
            printf("SV_SUBSCRIBER:     SmpRate: %d\n", SVSubscriber_ASDU_getSmpRate(&asdu));
    }

    /* Call callback handler */
    if (subscriber) {
        if (subscriber->listener != NULL)
            subscriber->listener(subscriber, subscriber->listenerParameter, &asdu);
    }
}

static void
parseSequenceOfASDU(SVReceiver self, SVSubscriber subscriber, uint8_t* buffer, int length)
{
    int bufPos = 0;

    while (bufPos < length) {
        int elementLength;

        uint8_t tag = buffer[bufPos++];

        bufPos = BerDecoder_decodeLength(buffer, &elementLength, bufPos, length);
        if (bufPos < 0) {
            if (DEBUG_SV_SUBSCRIBER) printf("SV_SUBSCRIBER: Malformed message: failed to decode BER length tag!\n");
            return;
        }

        switch (tag) {
        case 0x30:
            parseASDU(self, subscriber, buffer + bufPos, elementLength);
            break;

        default: /* ignore unknown tag */
            if (DEBUG_SV_SUBSCRIBER) printf("SV_SUBSCRIBER: found unknown tag %02x\n", tag);
            break;
        }

        bufPos += elementLength;
    }
}

static void
parseSVPayload(SVReceiver self, SVSubscriber subscriber, uint8_t* buffer, int apduLength)
{
    int bufPos = 0;

    if (buffer[bufPos++] == 0x60) {
        int elementLength;

        bufPos = BerDecoder_decodeLength(buffer, &elementLength, bufPos, apduLength);
        if (bufPos < 0) {
            if (DEBUG_SV_SUBSCRIBER) printf("SV_SUBSCRIBER: Malformed message: failed to decode BER length tag!\n");
            return;
        }

        int svEnd = bufPos + elementLength;

        while (bufPos < svEnd) {
            uint8_t tag = buffer[bufPos++];

            bufPos = BerDecoder_decodeLength(buffer, &elementLength, bufPos, svEnd);
            if (bufPos < 0) {
                if (DEBUG_SV_SUBSCRIBER) printf("SV_SUBSCRIBER: Malformed message: failed to decode BER length tag!\n");
                return;
            }

            if (bufPos + elementLength > apduLength) {
                if (DEBUG_SV_SUBSCRIBER)
                    printf("SV_SUBSCRIBER: Malformed message: sub element is too large!\n");

                goto exit_error;
            }

            if (bufPos == -1)
                goto exit_error;

            switch(tag) {
            case 0x80: /* noASDU (INTEGER) */
                /* ignore */
                break;

            case 0xa2: /* asdu (SEQUENCE) */
                parseSequenceOfASDU(self, subscriber, buffer + bufPos, elementLength);
                break;

            default: /* ignore unknown tag */
                if (DEBUG_SV_SUBSCRIBER) printf("SV_SUBSCRIBER: found unknown tag %02x\n", tag);
                break;
            }


            bufPos += elementLength;
        }

        return;
    }

exit_error:
    if (DEBUG_SV_SUBSCRIBER)
        printf("SV_SUBSCRIBER: Invalid SV message!\n");

    return;
}

static void
parseSVMessage(SVReceiver self, int numbytes)
{
    int bufPos;
    bool subscriberFound = false;
    uint8_t* buffer = self->buffer;

    if (numbytes < 22) return;

    /* Ethernet source address */
    uint8_t* srcAddr = buffer;

    /* skip ethernet addresses */
    bufPos = 12;
    int headerLength = 14;

    /* check for VLAN tag */
    if ((buffer[bufPos] == 0x81) && (buffer[bufPos + 1] == 0x00)) {
        bufPos += 4; /* skip VLAN tag */
        headerLength += 4;
    }

    /* check for SV Ethertype */
    if (buffer[bufPos++] != 0x88)
        return;
    if (buffer[bufPos++] != 0xba)
        return;

    uint16_t appId;

    appId = buffer[bufPos++] * 0x100;
    appId += buffer[bufPos++];

    uint16_t length;

    length = buffer[bufPos++] * 0x100;
    length += buffer[bufPos++];

    /* skip reserved fields */
    bufPos += 4;

    int apduLength = length - 8;

    if (numbytes < length + headerLength) {
        if (DEBUG_SV_SUBSCRIBER)
            printf("SV_SUBSCRIBER: Invalid PDU size\n");
        return;
    }

    if (DEBUG_SV_SUBSCRIBER) {
        printf("SV_SUBSCRIBER: SV message: ----------------\n");
        printf("SV_SUBSCRIBER:   APPID: %u\n", appId);
        printf("SV_SUBSCRIBER:   LENGTH: %u\n", length);
        printf("SV_SUBSCRIBER:   APDU length: %i\n", apduLength);
    }


    /* check if there is a matching subscriber */

#if (CONFIG_MMS_THREADLESS_STACK == 0)
    Semaphore_wait(self->subscriberListLock);
#endif

    LinkedList element = LinkedList_getNext(self->subscriberList);

    SVSubscriber subscriber;

    while (element != NULL) {
        subscriber = (SVSubscriber) LinkedList_getData(element);

        if (subscriber->appId == appId) {

            if (self->checkDestAddr) {
                if (memcmp(srcAddr, subscriber->ethAddr, 6) == 0) {
                    subscriberFound = true;
                    break;
                }
                else
                    if (DEBUG_SV_SUBSCRIBER)
                        printf("SV_SUBSCRIBER: Checking ethernet src address failed!\n");
            }
            else {
                subscriberFound = true;
                break;
            }


        }

        element = LinkedList_getNext(element);
    }

#if (CONFIG_MMS_THREADLESS_STACK == 0)
    Semaphore_post(self->subscriberListLock);
#endif


    if (subscriberFound)
        parseSVPayload(self, subscriber, buffer + bufPos, apduLength);
    else {
        if (DEBUG_SV_SUBSCRIBER)
            printf("SV_SUBSCRIBER: SV message ignored due to unknown APPID value\n");
    }
}

bool
SVReceiver_tick(SVReceiver self)
{
    int packetSize = Ethernet_receivePacket(self->ethSocket, self->buffer, ETH_BUFFER_LENGTH);

    if (packetSize > 0) {
        parseSVMessage(self, packetSize);
        return true;
    }
    else
        return false;
}

SVSubscriber
SVSubscriber_create(const uint8_t* ethAddr, uint16_t appID)
{
    SVSubscriber self = (SVSubscriber) GLOBAL_CALLOC(1, sizeof(struct sSVSubscriber));

    if (self != NULL) {
        self->appId = appID;

        if (ethAddr != NULL)
            memcpy(self->ethAddr, ethAddr, 6);
    }

    return self;
}

void
SVSubscriber_destroy(SVSubscriber self)
{
    if (self != NULL)
        GLOBAL_FREEMEM(self);
}


void
SVSubscriber_setListener(SVSubscriber self,  SVUpdateListener listener, void* parameter)
{
    self->listener = listener;
    self->listenerParameter = parameter;
}

uint16_t
SVSubscriber_ASDU_getSmpCnt(SVSubscriber_ASDU self)
{
    uint16_t retVal;
    uint8_t* valBytes = (uint8_t*) &retVal;

#if (ORDER_LITTLE_ENDIAN == 1)
    valBytes[0] = self->smpCnt[1];
    valBytes[1] = self->smpCnt[0];
#else
    valBytes[0] = self->smpCnt[0];
    valBytes[1] = self->smpCnt[1];
#endif

    return retVal;
}

static uint64_t
decodeUtcTime(uint8_t* buffer, uint8_t* timeQuality)
{
    uint32_t timeval32;

    timeval32 = buffer[3];
    timeval32 += buffer[2] * 0x100;
    timeval32 += buffer[1] * 0x10000;
    timeval32 += buffer[0] * 0x1000000;

    uint32_t msVal;

    uint32_t fractionOfSecond;

    fractionOfSecond = buffer[6];
    fractionOfSecond += buffer[5] * 0x100;
    fractionOfSecond += buffer[4] * 0x10000;

    msVal = (uint32_t) (((uint64_t) fractionOfSecond * 1000) / 16777215);

    if (timeQuality != NULL)
        *timeQuality = buffer[7];

    uint64_t timeval64 = (uint64_t) timeval32 * 1000 + (uint64_t) msVal;

    return timeval64;
}

uint64_t
SVSubscriber_ASDU_getRefrTmAsMs(SVSubscriber_ASDU self)
{
    uint64_t msTime = 0;

    if (self->refrTm != NULL)
        msTime = decodeUtcTime(self->refrTm, NULL);

    return msTime;
}

bool
SVSubscriber_ASDU_hasRefrTm(SVSubscriber_ASDU self)
{
    return (self->refrTm != NULL);
}

bool
SVSubscriber_ASDU_hasDatSet(SVSubscriber_ASDU self)
{
    return (self->datSet != NULL);
}

bool
SVSubscriber_ASDU_hasSmpRate(SVSubscriber_ASDU self)
{
    return (self->smpRate != NULL);
}

bool
SVSubscriber_ASDU_hasSmpMod(SVSubscriber_ASDU self)
{
    return (self->smpMod != NULL);
}

const char*
SVSubscriber_ASDU_getSvId(SVSubscriber_ASDU self)
{
    return self->svId;
}

const char*
SVSubscriber_ASDU_getDatSet(SVSubscriber_ASDU self)
{
    return self->datSet;
}

static inline void
memcpy_reverse(void* to, const void* from, size_t size)
{
    size_t i;

    for (i = 0; i < size; i++)
        ((uint8_t*)to)[size - 1 - i] = ((uint8_t*)from)[i];
}

uint32_t
SVSubscriber_ASDU_getConfRev(SVSubscriber_ASDU self)
{
    uint32_t retVal;

#if (ORDER_LITTLE_ENDIAN == 1)
    memcpy_reverse(&retVal, self->confRev, sizeof(uint32_t));
#else
    memcpy(&retVal, self->confRev, sizeof(uint32_t));
#endif

    return retVal;
}

uint8_t
SVSubscriber_ASDU_getSmpMod(SVSubscriber_ASDU self)
{
    uint8_t retVal = *((uint8_t*) (self->smpMod));

    return retVal;
}

uint16_t
SVSubscriber_ASDU_getSmpRate(SVSubscriber_ASDU self)
{
    uint16_t retVal;

#if (ORDER_LITTLE_ENDIAN == 1)
    memcpy_reverse(&retVal, self->smpRate, sizeof(uint16_t));
#else
    memcpy(&retVal, self->smpRate, sizeof(uint16_t));
#endif

    return retVal;
}

int8_t
SVSubscriber_ASDU_getINT8(SVSubscriber_ASDU self, int index)
{
    int8_t retVal = *((int8_t*) (self->dataBuffer + index));

    return retVal;
}

int16_t
SVSubscriber_ASDU_getINT16(SVSubscriber_ASDU self, int index)
{
    int16_t retVal;

#if (ORDER_LITTLE_ENDIAN == 1)
    memcpy_reverse(&retVal, (self->dataBuffer + index), sizeof(uint16_t));
#else
    memcpy(&retVal, (self->dataBuffer + index), sizeof(uint16_t));
#endif

    return retVal;
}

int32_t
SVSubscriber_ASDU_getINT32(SVSubscriber_ASDU self, int index)
{
    int32_t retVal;

#if (ORDER_LITTLE_ENDIAN == 1)
    memcpy_reverse(&retVal, (self->dataBuffer + index), sizeof(int32_t));
#else
    memcpy(&retVal, (self->dataBuffer + index), sizeof(int32_t));
#endif

    return retVal;
}

int64_t
SVSubscriber_ASDU_getINT64(SVSubscriber_ASDU self, int index)
{
    int64_t retVal;

#if (ORDER_LITTLE_ENDIAN == 1)
    memcpy_reverse(&retVal, (self->dataBuffer + index), sizeof(int64_t));
#else
    memcpy(&retVal, (self->dataBuffer + index), sizeof(int64_t));
#endif

    return retVal;
}

uint8_t
SVSubscriber_ASDU_getINT8U(SVSubscriber_ASDU self, int index)
{
    uint8_t retVal = *((uint8_t*) (self->dataBuffer + index));

    return retVal;
}

uint16_t
SVSubscriber_ASDU_getINT16U(SVSubscriber_ASDU self, int index)
{
    uint16_t retVal;

#if (ORDER_LITTLE_ENDIAN == 1)
    memcpy_reverse(&retVal, (self->dataBuffer + index), sizeof(uint16_t));
#else
    memcpy(&retVal, (self->dataBuffer + index), sizeof(uint16_t));
#endif

    return retVal;
}

uint32_t
SVSubscriber_ASDU_getINT32U(SVSubscriber_ASDU self, int index)
{
    uint32_t retVal;

#if (ORDER_LITTLE_ENDIAN == 1)
    memcpy_reverse(&retVal, (self->dataBuffer + index), sizeof(uint32_t));
#else
    memcpy(&retVal, (self->dataBuffer + index), sizeof(uint32_t));
#endif

    return retVal;
}

uint64_t
SVSubscriber_ASDU_getINT64U(SVSubscriber_ASDU self, int index)
{
    uint64_t retVal;

#if (ORDER_LITTLE_ENDIAN == 1)
    memcpy_reverse(&retVal, (self->dataBuffer + index), sizeof(uint64_t));
#else
    memcpy(&retVal, (self->dataBuffer + index), sizeof(uint64_t));
#endif

    return retVal;
}

float
SVSubscriber_ASDU_getFLOAT32(SVSubscriber_ASDU self, int index)
{
    float retVal;

#if (ORDER_LITTLE_ENDIAN == 1)
    memcpy_reverse(&retVal, (self->dataBuffer + index), sizeof(float));
#else
    memcpy(&retVal, (self->dataBuffer + index), sizeof(float));
#endif

    return retVal;
}

double
SVSubscriber_ASDU_getFLOAT64(SVSubscriber_ASDU self, int index)
{
    double retVal;

#if (ORDER_LITTLE_ENDIAN == 1)
    memcpy_reverse(&retVal, (self->dataBuffer + index), sizeof(double));
#else
    memcpy(&retVal, (self->dataBuffer + index), sizeof(double));
#endif

    return retVal;
}

Timestamp
SVSubscriber_ASDU_getTimestamp(SVSubscriber_ASDU self, int index)
{
    Timestamp retVal;

    memcpy(retVal.val, self->dataBuffer + index, sizeof(retVal.val));

    return retVal;
}

Quality
SVSubscriber_ASDU_getQuality(SVSubscriber_ASDU self, int index)
{
    Quality retVal;

    uint8_t* buffer = self->dataBuffer + index;

    retVal = buffer[3];
    retVal += (buffer[2] * 0x100);

    return retVal;
}

int
SVSubscriber_ASDU_getDataSize(SVSubscriber_ASDU self)
{
    return self->dataBufferLength;
}

uint16_t
SVClientASDU_getSmpCnt(SVSubscriber_ASDU self)
{
    return SVSubscriber_ASDU_getSmpCnt(self);
}

const char*
SVClientASDU_getSvId(SVSubscriber_ASDU self)
{
    return SVSubscriber_ASDU_getSvId(self);
}

uint32_t
SVClientASDU_getConfRev(SVSubscriber_ASDU self)
{
    return SVSubscriber_ASDU_getConfRev(self);
}

bool
SVClientASDU_hasRefrTm(SVSubscriber_ASDU self)
{
    return SVSubscriber_ASDU_hasRefrTm(self);
}

uint64_t
SVClientASDU_getRefrTmAsMs(SVSubscriber_ASDU self)
{
    return SVSubscriber_ASDU_getRefrTmAsMs(self);
}

int8_t
SVClientASDU_getINT8(SVSubscriber_ASDU self, int index)
{
    return SVSubscriber_ASDU_getINT8(self, index);
}

int16_t
SVClientASDU_getINT16(SVSubscriber_ASDU self, int index)
{
    return SVSubscriber_ASDU_getINT16(self, index);
}

int32_t
SVClientASDU_getINT32(SVSubscriber_ASDU self, int index)
{
    return SVSubscriber_ASDU_getINT32(self, index);
}

int64_t
SVClientASDU_getINT64(SVSubscriber_ASDU self, int index)
{
    return SVSubscriber_ASDU_getINT64(self, index);
}

uint8_t
SVClientASDU_getINT8U(SVSubscriber_ASDU self, int index)
{
    return SVSubscriber_ASDU_getINT8U(self, index);
}

uint16_t
SVClientASDU_getINT16U(SVSubscriber_ASDU self, int index)
{
    return SVSubscriber_ASDU_getINT16U(self, index);
}

uint32_t
SVClientASDU_getINT32U(SVSubscriber_ASDU self, int index)
{
    return SVSubscriber_ASDU_getINT32U(self, index);
}

uint64_t
SVClientASDU_getINT64U(SVSubscriber_ASDU self, int index)
{
    return SVSubscriber_ASDU_getINT64U(self, index);
}

float
SVClientASDU_getFLOAT32(SVSubscriber_ASDU self, int index)
{
    return SVSubscriber_ASDU_getFLOAT32(self, index);
}

double
SVClientASDU_getFLOAT64(SVSubscriber_ASDU self, int index)
{
    return SVSubscriber_ASDU_getFLOAT64(self, index);
}

int
SVClientASDU_getDataSize(SVSubscriber_ASDU self)
{
    return SVSubscriber_ASDU_getDataSize(self);
}

