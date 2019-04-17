/*
 *  mms_client_connection.c
 *
 *  Copyright 2013, 2014, 2015 Michael Zillgith
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

#include "libiec61850_platform_includes.h"

#include "mms_client_connection.h"
#include "iso_client_connection.h"
#include "mms_client_internal.h"
#include "stack_config.h"

#include <MmsPdu.h>

#include "byte_buffer.h"
#include "ber_decode.h"

#include <assert.h>
#include "tls_config.h"

#define CONFIG_MMS_CONNECTION_DEFAULT_TIMEOUT 5000
#define CONFIG_MMS_CONNECTION_DEFAULT_CONNECT_TIMEOUT 10000
#define OUTSTANDING_CALLS 10

static void
setAssociationState(MmsConnection self, AssociationState newState)
{
    Semaphore_wait(self->associationStateLock);
    self->associationState = newState;
    Semaphore_post(self->associationStateLock);
}

static AssociationState
getAssociationState(MmsConnection self)
{
    AssociationState state;

    Semaphore_wait(self->associationStateLock);
    state = self->associationState;
    Semaphore_post(self->associationStateLock);

    return state;
}

static void
setConnectionState(MmsConnection self, ConnectionState newState)
{
    Semaphore_wait(self->connectionStateLock);
    self->connectionState = newState;
    Semaphore_post(self->connectionStateLock);
}

static ConnectionState
getConnectionState(MmsConnection self)
{
    ConnectionState state;

    Semaphore_wait(self->connectionStateLock);
    state = self->connectionState;
    Semaphore_post(self->connectionStateLock);

    return state;
}

static void
setConcludeState(MmsConnection self, int newState)
{
    Semaphore_wait(self->concludeStateLock);
    self->concludeState = newState;
    Semaphore_post(self->concludeStateLock);
}

static int
getConcludeState(MmsConnection self)
{
    int state;

    Semaphore_wait(self->concludeStateLock);
    state = self->concludeState;
    Semaphore_post(self->concludeStateLock);

    return state;
}

static void
handleUnconfirmedMmsPdu(MmsConnection self, ByteBuffer* message)
{
	MmsValue* values;
	int i;

    if (self->reportHandler != NULL) {
        MmsPdu_t* mmsPdu = 0; /* allow asn1c to allocate structure */
		asn_dec_rval_t rval;

        if (DEBUG_MMS_CLIENT)
            printf("MMS_CLIENT: report handler rcvd size:%i\n", ByteBuffer_getSize(message));

        rval = ber_decode(NULL, &asn_DEF_MmsPdu,
                (void**) &mmsPdu, ByteBuffer_getBuffer(message), ByteBuffer_getSize(message));

        if (rval.code == RC_OK) {
            if (DEBUG_MMS_CLIENT)
                printf("MMS_CLIENT: received report (size:%i)\n", (int) rval.consumed);

            if (mmsPdu->present == MmsPdu_PR_unconfirmedPDU) {

                if (mmsPdu->choice.unconfirmedPDU.unconfirmedService.present ==
                        UnconfirmedService_PR_informationReport)
                        {
                    char* domainId = NULL;

                    InformationReport_t* report =
                            &(mmsPdu->choice.unconfirmedPDU.unconfirmedService.choice.informationReport);

                    if (report->variableAccessSpecification.present ==
                            VariableAccessSpecification_PR_variableListName)
                            {
                        if (report->variableAccessSpecification.choice.variableListName.present
                                == ObjectName_PR_vmdspecific)
                                {
                            int nameSize =
                                    report->variableAccessSpecification.choice.variableListName.choice.vmdspecific.size;
                            uint8_t* buffer =
                                    report->variableAccessSpecification.choice.variableListName.choice.vmdspecific.buf;

                            char* variableListName = StringUtils_createStringFromBuffer(buffer, nameSize);

                            int listSize = report->listOfAccessResult.list.count;

                            MmsValue* values = mmsClient_parseListOfAccessResults(
                                    report->listOfAccessResult.list.array, listSize, true);

                            self->reportHandler(self->reportHandlerParameter, domainId, variableListName, values, true);

                            GLOBAL_FREEMEM(variableListName);
                        }
                        else {
                            // Ignore domain and association specific information reports (not used by IEC 61850)
                        }

                    }
                    else if (report->variableAccessSpecification.present == VariableAccessSpecification_PR_listOfVariable)
                            {
                        int listSize = report->listOfAccessResult.list.count;
                        int variableSpecSize = report->variableAccessSpecification.choice.listOfVariable.list.count;

                        if (listSize != variableSpecSize) {
                            if (DEBUG_MMS_CLIENT)
                                printf("report contains wrong number of access results\n");
                            return;
                        }

                        values = mmsClient_parseListOfAccessResults(
                                report->listOfAccessResult.list.array, listSize, false);
                        
                        for (i = 0; i < variableSpecSize; i++) {
                            if (report->variableAccessSpecification.choice.listOfVariable.list.array[i]->variableSpecification.present
                                    == VariableSpecification_PR_name)
                                    {
                                if (report->variableAccessSpecification.choice.listOfVariable.list.array[i]
                                        ->variableSpecification.choice.name.present == ObjectName_PR_vmdspecific)
                                        {
                                    int nameSize =
                                            report->variableAccessSpecification.choice.listOfVariable.list.array[i]
                                                    ->variableSpecification.choice.name.choice.vmdspecific.size;

                                    uint8_t* buffer =
                                            report->variableAccessSpecification.choice.listOfVariable.list.array[i]
                                                    ->variableSpecification.choice.name.choice.vmdspecific.buf;

                                    if (nameSize < 129) {
                                        char variableListName[129];
										MmsValue* value;

                                        memcpy(variableListName, buffer, nameSize);
                                        variableListName[nameSize] = 0;

                                        value = values;

                                        if (variableSpecSize != 1)
                                            value = MmsValue_getElement(values, i);

                                        self->reportHandler(self->reportHandlerParameter, domainId, variableListName,
                                                value, false);

                                        // report handler should have deleted the MmsValue!
                                        if (variableSpecSize != 1)
                                            MmsValue_setElement(values, i, NULL);
                                        else
                                            values = NULL;
                                    }
                                }
                                else if (report->variableAccessSpecification.choice.listOfVariable.list.array[i]
                                        ->variableSpecification.choice.name.present == ObjectName_PR_domainspecific) {

                                    int domainNameSize =
                                            report->variableAccessSpecification.choice.listOfVariable.list.array[i]
                                                    ->variableSpecification.choice.name.choice.domainspecific.domainId.size;

                                    int itemNameSize =
                                            report->variableAccessSpecification.choice.listOfVariable.list.array[i]
                                                    ->variableSpecification.choice.name.choice.domainspecific.itemId.size;

                                    if (domainNameSize < 65 && itemNameSize < 65) {
                                        char domainNameStr[65];
                                        char itemNameStr[65];
										MmsValue* value;

                                        uint8_t* domainNameBuffer =
                                                report->variableAccessSpecification.choice.listOfVariable.list.array[i]
                                                        ->variableSpecification.choice.name.choice.domainspecific.domainId.buf;

                                        uint8_t* itemNamebuffer =
                                                report->variableAccessSpecification.choice.listOfVariable.list.array[i]
                                                        ->variableSpecification.choice.name.choice.domainspecific.itemId.buf;

                                        memcpy(domainNameStr, domainNameBuffer, domainNameSize);
                                        domainNameStr[domainNameSize] = 0;
                                        memcpy(itemNameStr, itemNamebuffer, itemNameSize);
                                        itemNameStr[itemNameSize] = 0;

                                        value = values;

                                        if (variableSpecSize != 1)
                                            value = MmsValue_getElement(values, i);

                                        self->reportHandler(self->reportHandlerParameter, domainNameStr, itemNameStr,
                                                value, false);

                                        // report handler should have deleted the MmsValue!
                                        if (variableSpecSize != 1)
                                            MmsValue_setElement(values, i, NULL);
                                        else
                                            values = NULL;

                                    }
                                }
                            }
                        }

                        if (values != NULL)
                            MmsValue_delete(values);
                    }
                    else {
                        // Ignore
                        if (DEBUG_MMS_CLIENT)
                            printf("unrecognized information report\n");
                    }

                }

            }
        }
        else {
            if (DEBUG_MMS_CLIENT)
                printf("handleUnconfirmedMmsPdu: error parsing PDU at %u\n", (uint32_t) rval.consumed);
        }

        asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);
    }
}

static uint32_t
getNextInvokeId(MmsConnection self)
{
    uint32_t nextInvokeId;

    Semaphore_wait(self->lastInvokeIdLock);
    self->lastInvokeId++;
    nextInvokeId = self->lastInvokeId;
    Semaphore_post(self->lastInvokeIdLock);

    return nextInvokeId;
}

static bool
checkForOutstandingCall(MmsConnection self, uint32_t invokeId)
{
    int i = 0;

    Semaphore_wait(self->outstandingCallsLock);

    for (i = 0; i < OUTSTANDING_CALLS; i++) {
        if (self->outstandingCalls[i] == invokeId) {
            Semaphore_post(self->outstandingCallsLock);
            return true;
        }
    }

    Semaphore_post(self->outstandingCallsLock);

    return false;
}

static bool
addToOutstandingCalls(MmsConnection self, uint32_t invokeId)
{
    int i = 0;

    Semaphore_wait(self->outstandingCallsLock);

    for (i = 0; i < OUTSTANDING_CALLS; i++) {
        if (self->outstandingCalls[i] == 0) {
            self->outstandingCalls[i] = invokeId;
            Semaphore_post(self->outstandingCallsLock);
            return true;
        }
    }

    Semaphore_post(self->outstandingCallsLock);

    return false;
}

static void
removeFromOutstandingCalls(MmsConnection self, uint32_t invokeId)
{
    int i = 0;

    Semaphore_wait(self->outstandingCallsLock);

    for (i = 0; i < OUTSTANDING_CALLS; i++) {
        if (self->outstandingCalls[i] == invokeId) {
            self->outstandingCalls[i] = 0;
            break;
        }
    }

    Semaphore_post(self->outstandingCallsLock);
}

static ByteBuffer*
sendRequestAndWaitForResponse(MmsConnection self, uint32_t invokeId, ByteBuffer* message, MmsError* mmsError)
{
    ByteBuffer* receivedMessage = NULL;

    uint64_t currentTime = Hal_getTimeInMs();

    uint64_t waitUntilTime = currentTime + self->requestTimeout;

    bool success = false;

    if (addToOutstandingCalls(self, invokeId) == false) {
        *mmsError = MMS_ERROR_OUTSTANDING_CALL_LIMIT;
        return NULL;
    }

    *mmsError = MMS_ERROR_NONE;

#if (CONFIG_MMS_RAW_MESSAGE_LOGGING == 1)
    if (self->rawMmsMessageHandler != NULL) {
        MmsRawMessageHandler handler = (MmsRawMessageHandler) self->rawMmsMessageHandler;
        handler(self->rawMmsMessageHandlerParameter, message->buffer, message->size, false);
    }
#endif /* (CONFIG_MMS_RAW_MESSAGE_LOGGING == 1) */

    IsoClientConnection_sendMessage(self->isoClient, message);

    while (currentTime < waitUntilTime) {
        uint32_t receivedInvokeId;

        if (getAssociationState(self) == MMS_STATE_CLOSED) {
            *mmsError = MMS_ERROR_CONNECTION_LOST;
            goto connection_lost;
        }

        Semaphore_wait(self->lastResponseLock);

        receivedInvokeId = self->responseInvokeId;

        if (receivedInvokeId == invokeId) {

            receivedMessage = self->lastResponse;
            Semaphore_post(self->lastResponseLock);

            success = true;
            break;
        }

        Semaphore_post(self->lastResponseLock);

        Thread_sleep(10);

        currentTime = Hal_getTimeInMs();
    }

    if (!success) {
        if (DEBUG_MMS_CLIENT)
            printf("MMS_CLIENT: TIMEOUT for request %u: \n", invokeId);

        *mmsError = MMS_ERROR_SERVICE_TIMEOUT;
    }
    else {
        *mmsError = self->lastResponseError;

        if (*mmsError != MMS_ERROR_NONE)
            receivedMessage = NULL;
    }

    connection_lost:

    removeFromOutstandingCalls(self, invokeId);

    return receivedMessage;
}

static void
releaseResponse(MmsConnection self)
{
    Semaphore_wait(self->lastResponseLock);
    self->responseInvokeId = 0;
    self->lastResponseError = MMS_ERROR_NONE;
    Semaphore_post(self->lastResponseLock);

    IsoClientConnection_releaseReceiveBuffer(self->isoClient);
}

static uint32_t
getResponseInvokeId(MmsConnection self)
{
    uint32_t currentInvokeId;

    Semaphore_wait(self->lastResponseLock);
    currentInvokeId = self->responseInvokeId;
    Semaphore_post(self->lastResponseLock);

    return currentInvokeId;
}

static void
waitUntilLastResponseHasBeenProcessed(MmsConnection self)
{
    uint32_t currentInvokeId = getResponseInvokeId(self);

    while (currentInvokeId != 0) {
        Thread_sleep(1);
        currentInvokeId = getResponseInvokeId(self);
    }
}

static MmsError
convertRejectCodesToMmsError(int rejectType, int rejectReason)
{
    if ((rejectType == 1) && (rejectReason == 1))
        return MMS_ERROR_REJECT_UNRECOGNIZED_SERVICE;
    else if ((rejectType == 5) && (rejectReason == 0))
        return MMS_ERROR_REJECT_UNKNOWN_PDU_TYPE;
    else if ((rejectType == 1) && (rejectReason == 4))
        return MMS_ERROR_REJECT_REQUEST_INVALID_ARGUMENT;
    else if ((rejectType == 5) && (rejectReason == 1))
        return MMS_ERROR_REJECT_INVALID_PDU;
    else
        return MMS_ERROR_REJECT_OTHER;
}

static MmsError
convertServiceErrorToMmsError(MmsServiceError serviceError)
{
    MmsError mmsError;

    switch (serviceError.errorClass)
    {
    case 0: /* class: vmd-state */
        mmsError = MMS_ERROR_VMDSTATE_OTHER;
        break;

    case 1: /* class: application-reference */
        mmsError = MMS_ERROR_APPLICATION_REFERENCE_OTHER;
        break;

    case 2: /* class: definition */
        switch (serviceError.errorCode)
        {
        case 1:
            mmsError = MMS_ERROR_DEFINITION_OBJECT_UNDEFINED;
            break;
        case 2:
            mmsError = MMS_ERROR_DEFINITION_INVALID_ADDRESS;
            break;
        case 3:
            mmsError = MMS_ERROR_DEFINITION_TYPE_UNSUPPORTED;
            break;
        case 4:
            mmsError = MMS_ERROR_DEFINITION_TYPE_INCONSISTENT;
            break;
        case 5:
            mmsError = MMS_ERROR_DEFINITION_OBJECT_EXISTS;
            break;
        case 6:
            mmsError = MMS_ERROR_DEFINITION_OBJECT_ATTRIBUTE_INCONSISTENT;
            break;
        default:
            mmsError = MMS_ERROR_DEFINITION_OTHER;
            break;
        }
        break;

    case 3: /* class: resource */
        mmsError = MMS_ERROR_RESOURCE_OTHER;
        break;

    case 4: /* class: service */
        mmsError = MMS_ERROR_SERVICE_OTHER;
        break;

    case 5: /* class: service-preempt */
        mmsError = MMS_ERROR_SERVICE_PREEMPT_OTHER;
        break;

    case 6: /* class: time-resolution */
        mmsError = MMS_ERROR_TIME_RESOLUTION_OTHER;
        break;

    case 7: /* class: access */
        switch (serviceError.errorCode)
        {
        case 1:
            mmsError = MMS_ERROR_ACCESS_OBJECT_ACCESS_UNSUPPORTED;
            break;
        case 2:
            mmsError = MMS_ERROR_ACCESS_OBJECT_NON_EXISTENT;
            break;
        case 3:
            mmsError = MMS_ERROR_ACCESS_OBJECT_ACCESS_DENIED;
            break;
        case 4:
            mmsError = MMS_ERROR_ACCESS_OBJECT_INVALIDATED;
            break;
        default:
            mmsError = MMS_ERROR_ACCESS_OTHER;
            break;
        }
        break;

    case 11: /* class: file */
        switch (serviceError.errorCode)
        {
        case 1:
            mmsError = MMS_ERROR_FILE_FILENAME_AMBIGUOUS;
            break;
        case 2:
            mmsError = MMS_ERROR_FILE_FILE_BUSY;
            break;
        case 3:
            mmsError = MMS_ERROR_FILE_FILENAME_SYNTAX_ERROR;
            break;
        case 4:
            mmsError = MMS_ERROR_FILE_CONTENT_TYPE_INVALID;
            break;
        case 5:
            mmsError = MMS_ERROR_FILE_POSITION_INVALID;
            break;
        case 6:
            mmsError = MMS_ERROR_FILE_FILE_ACCESS_DENIED;
            break;
        case 7:
            mmsError = MMS_ERROR_FILE_FILE_NON_EXISTENT;
            break;
        case 8:
            mmsError = MMS_ERROR_FILE_DUPLICATE_FILENAME;
            break;
        case 9:
            mmsError = MMS_ERROR_FILE_INSUFFICIENT_SPACE_IN_FILESTORE;
            break;
        default:
            mmsError = MMS_ERROR_FILE_OTHER;
            break;
        }
        break;

    default:
        mmsError = MMS_ERROR_OTHER;
    }

    return mmsError;
}

static int
parseServiceError(uint8_t* buffer, int bufPos, int maxLength, MmsServiceError* error)
{
    int endPos = bufPos + maxLength;
    int length;

    while (bufPos < endPos) {
        uint8_t tag = buffer[bufPos++];
        bufPos = BerDecoder_decodeLength(buffer, &length, bufPos, endPos);

        if (bufPos < 0)
            return -1;

        switch (tag)
        {
        case 0xa0: /* errorClass */
            {
                uint8_t errorClassTag = buffer[bufPos++];
                bufPos = BerDecoder_decodeLength(buffer, &length, bufPos, endPos);

                if (bufPos < 0)
                    return -1;

                error->errorClass = errorClassTag - 0x80;
                error->errorCode = BerDecoder_decodeUint32(buffer, length, bufPos);

                bufPos += length;
            }
            break;
        case 0x81: /* additionalCode */
            bufPos += length; /* ignore */
            break;
        case 0x82: /* additionalDescription */
            bufPos += length; /* ignore */
            break;
        case 0xa3: /* serviceSpecificInfo */
            bufPos += length; /* ignore */
            break;
        default:
            bufPos += length; /* ignore */
            break;
        }
    }

    return bufPos;
}

int
mmsMsg_parseConfirmedErrorPDU(uint8_t* buffer, int bufPos, int maxBufPos, uint32_t* invokeId, MmsServiceError* serviceError)
{
    int length;
	int endPos;

    uint8_t tag = buffer[bufPos++];
    if (tag != 0xa2)
        goto exit_error;

    bufPos = BerDecoder_decodeLength(buffer, &length, bufPos, maxBufPos);
    if (bufPos < 0)
        goto exit_error;

    endPos = bufPos + length;

    if (endPos > maxBufPos)
        goto exit_error;

    while (bufPos < endPos) {
        tag = buffer[bufPos++];

        bufPos = BerDecoder_decodeLength(buffer, &length, bufPos, maxBufPos);
        if (bufPos < 0)
            goto exit_error;

        switch (tag)
        {
        case 0x80: /* invoke Id */
            if (invokeId != NULL)
                *invokeId = BerDecoder_decodeUint32(buffer, length, bufPos);
            bufPos += length;
            break;
        case 0x81: /* modifierPosition */
            bufPos += length; /* ignore */
            break;
        case 0xa2: /* serviceError */
            bufPos = parseServiceError(buffer, bufPos, length, serviceError);
            if (bufPos < 0)
                goto exit_error;
            break;
        default:
            bufPos += length; /* ignore */
            break;
        }
    }

    return bufPos;

    exit_error:
    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: error parsing confirmed error PDU\n");

    return -1;
}

int
mmsMsg_parseRejectPDU(uint8_t* buffer, int bufPos, int maxBufPos, uint32_t* invokeId, int* rejectType, int* rejectReason)
{
    int length;

    uint8_t tag = buffer[bufPos++];

	int endPos;

    if (tag != 0xa4)
        goto exit_error;

    bufPos = BerDecoder_decodeLength(buffer, &length, bufPos, maxBufPos);
    if (bufPos < 0)
        goto exit_error;

    if (bufPos + length > maxBufPos)
        goto exit_error;

    endPos = bufPos + length;

    if (endPos > maxBufPos)
        goto exit_error;

    while (bufPos < endPos) {
        tag = buffer[bufPos++];

        bufPos = BerDecoder_decodeLength(buffer, &length, bufPos, maxBufPos);
        if (bufPos < 0)
            goto exit_error;

        if (tag == 0x80) { /* invoke id */
            if (invokeId != NULL)
                *invokeId = BerDecoder_decodeUint32(buffer, length, bufPos);
        }
        else if (tag > 0x80 && tag < 0x8c) {
            *rejectType = tag - 0x80;
            *rejectReason = BerDecoder_decodeInt32(buffer, length, bufPos);
        }
        else {
            /* unknown - ignore */
        }

        bufPos += length;
    }

    return bufPos;

    exit_error:
    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: error parsing reject PDU\n");

    return -1;
}

static void
mmsIsoCallback(IsoIndication indication, void* parameter, ByteBuffer* payload)
{
    MmsConnection self = (MmsConnection) parameter;
	uint8_t* buf;
	uint8_t tag;
	uint32_t invokeId;
    int rejectType;
    int rejectReason;
    MmsServiceError serviceError = { 0, 0 };
	int bufPos = 1;
    int length;

    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: mmsIsoCallback called with indication %i\n", indication);

    if (indication == ISO_IND_CLOSED) {
        if (DEBUG_MMS_CLIENT)
            printf("MMS_CLIENT: mmsIsoCallback: Connection lost or closed by client!\n");
        setConnectionState(self, MMS_CON_IDLE);
        setAssociationState(self, MMS_STATE_CLOSED);

        /* Call user provided callback function */
        if (self->connectionLostHandler != NULL)
            self->connectionLostHandler(self, self->connectionLostHandlerParameter);

        return;
    }

    if (indication == ISO_IND_ASSOCIATION_FAILED) {
        if (DEBUG_MMS_CLIENT)
            printf("MMS_CLIENT: mmsIsoCallback: association failed!\n");
        setConnectionState(self, MMS_CON_ASSOCIATION_FAILED);
        setAssociationState(self, MMS_STATE_CLOSED);
        return;
    }

    if (payload != NULL) {
        if (ByteBuffer_getSize(payload) < 1) {
            IsoClientConnection_releaseReceiveBuffer(self->isoClient);
            return;
        }
    }

    buf = ByteBuffer_getBuffer(payload);

#if (CONFIG_MMS_RAW_MESSAGE_LOGGING == 1)
    if (self->rawMmsMessageHandler != NULL) {
        MmsRawMessageHandler handler = (MmsRawMessageHandler) self->rawMmsMessageHandler;
        handler(self->rawMmsMessageHandlerParameter, buf, payload->size, true);
    }
#endif /* (CONFIG_MMS_RAW_MESSAGE_LOGGING == 1) */

    tag = buf[0];

    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: MMS-PDU: %02x\n", tag);

    if (tag == 0xa9) { /* initiate response PDU */

        if (indication == ISO_IND_ASSOCIATION_SUCCESS)
            setConnectionState(self, MMS_CON_ASSOCIATED);
        else
            setConnectionState(self, MMS_CON_ASSOCIATION_FAILED);

        self->lastResponse = payload;

        IsoClientConnection_releaseReceiveBuffer(self->isoClient);
    }
    else if (tag == 0xa3) { /* unconfirmed PDU */
        handleUnconfirmedMmsPdu(self, payload);
        IsoClientConnection_releaseReceiveBuffer(self->isoClient);
    }
    else if (tag == 0x8b) { /* conclude request PDU */
        if (DEBUG_MMS_CLIENT)
            printf("MMS_CLIENT: received conclude.request\n");

        setConcludeState(self, CONCLUDE_STATE_REQUESTED);

        /* TODO block all new user requests? */

        IsoClientConnection_releaseReceiveBuffer(self->isoClient);
    }
    else if (tag == 0x8c) { /* conclude response PDU */
        if (DEBUG_MMS_CLIENT)
            printf("MMS_CLIENT: received conclude.reponse+\n");

        setConcludeState(self, CONCLUDE_STATE_ACCEPTED);

        IsoClientConnection_release(self->isoClient);

        IsoClientConnection_releaseReceiveBuffer(self->isoClient);
    }
    else if (tag == 0x8d) { /* conclude error PDU */
        if (DEBUG_MMS_CLIENT)
            printf("MMS_CLIENT: received conclude.reponse-\n");

        setConcludeState(self, CONCLUDE_STATE_REJECTED);

        IsoClientConnection_releaseReceiveBuffer(self->isoClient);
    }
    else if (tag == 0xa2) { /* confirmed error PDU */
        if (DEBUG_MMS_CLIENT)
            printf("MMS_CLIENT: Confirmed error PDU!\n");
        //uint32_t invokeId;
        //MmsServiceError serviceError =
        //{ 0, 0 };

        if (mmsMsg_parseConfirmedErrorPDU(payload->buffer, 0, payload->size, &invokeId, &serviceError) < 0) {
            if (DEBUG_MMS_CLIENT)
                printf("MMS_CLIENT: Error parsing confirmedErrorPDU!\n");

            goto exit_with_error;
        }
        else {
            if (checkForOutstandingCall(self, invokeId)) {

                /* wait for application thread to handle last received response */
                waitUntilLastResponseHasBeenProcessed(self);

                Semaphore_wait(self->lastResponseLock);
                self->lastResponseError = convertServiceErrorToMmsError(serviceError);
                self->responseInvokeId = invokeId;
                Semaphore_post(self->lastResponseLock);
            }
            else {
                if (DEBUG_MMS_CLIENT)
                    printf("MMS_CLIENT: unexpected message from server!\n");
                IsoClientConnection_releaseReceiveBuffer(self->isoClient);
                return;
            }
        }
    }
    else if (tag == 0xa4) { /* reject PDU */
        if (DEBUG_MMS_CLIENT)
            printf("MMS_CLIENT: reject PDU!\n");

        //uint32_t invokeId;
        //int rejectType;
        //int rejectReason;

        if (mmsMsg_parseRejectPDU(payload->buffer, 0, payload->size, &invokeId, &rejectType, &rejectReason) >= 0) {

            if (DEBUG_MMS_CLIENT)
                printf("MMS_CLIENT: reject PDU invokeID: %i type: %i reason: %i\n", (int) invokeId, rejectType, rejectReason);

            if (checkForOutstandingCall(self, invokeId)) {

                /* wait for application thread to handle last received response */
                waitUntilLastResponseHasBeenProcessed(self);

                Semaphore_wait(self->lastResponseLock);
                self->lastResponseError = convertRejectCodesToMmsError(rejectType, rejectReason);
                self->responseInvokeId = invokeId;
                Semaphore_post(self->lastResponseLock);
            }
            else {
                IsoClientConnection_releaseReceiveBuffer(self->isoClient);
                return;
            }
        }
        else
            goto exit_with_error;
    }
    else if (tag == 0xa1) { /* confirmed response PDU */

        int length;
        int bufPos = 1;

        bufPos = BerDecoder_decodeLength(buf, &length, bufPos, payload->size);
        if (bufPos == -1)
            goto exit_with_error;

        if (buf[bufPos++] == 0x02) {
            int invokeIdLength;

            bufPos = BerDecoder_decodeLength(buf, &invokeIdLength, bufPos, payload->size);
            if (bufPos == -1)
                goto exit_with_error;

            invokeId =
                    BerDecoder_decodeUint32(buf, invokeIdLength, bufPos);

            if (DEBUG_MMS_CLIENT)
                printf("MMS_CLIENT: mms_client_connection: rcvd confirmed resp - invokeId: %i length: %i bufLen: %i\n",
                        invokeId, length, payload->size);

            bufPos += invokeIdLength;

            if (checkForOutstandingCall(self, invokeId)) {

                waitUntilLastResponseHasBeenProcessed(self);

                Semaphore_wait(self->lastResponseLock);
                self->lastResponse = payload;
                self->lastResponseBufPos = bufPos;
                self->responseInvokeId = invokeId;
                Semaphore_post(self->lastResponseLock);
            }
            else {
                if (DEBUG_MMS_CLIENT)
                    printf("MMS_CLIENT: unexpected message from server!\n");
                IsoClientConnection_releaseReceiveBuffer(self->isoClient);
                return;
            }
        }
        else
            goto exit_with_error;
    }
#if (MMS_OBTAIN_FILE_SERVICE == 1)
    else if (tag == 0xa0) {

        if (DEBUG_MMS_CLIENT)
            printf("MMS_CLIENT: received confirmed request PDU (size=%i)\n", payload->size);

        // TODO extract function

        bufPos = 1;
        
        bufPos = BerDecoder_decodeLength(buf, &length, bufPos, payload->size);
        if (bufPos == -1)
            goto exit_with_error;

        while (bufPos < payload->size) {

            uint8_t nestedTag = buf[bufPos++];

            bool extendedTag = false;

            if ((nestedTag & 0x1f) == 0x1f) {
                extendedTag = true;
                nestedTag = buf[bufPos++];
            }

            bufPos = BerDecoder_decodeLength(buf, &length, bufPos, payload->size);
            if (bufPos == -1)
                goto exit_with_error;

            if (extendedTag) {
                switch (nestedTag)
                {

#if (MMS_FILE_SERVICE == 1)
                case 0x48: /* file-open-request */
                    {
						ByteBuffer* response;
                        if (DEBUG_MMS_CLIENT)
                            printf("MMS_CLIENT: received file-open-request\n");

                        response = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

                        mmsClient_handleFileOpenRequest(self, buf, bufPos, bufPos + length, invokeId, response);

                        IsoClientConnection_sendMessage(self->isoClient, response);

                        IsoClientConnection_releaseReceiveBuffer(self->isoClient);
                    }
                    break;

                case 0x49: /* file-read-request */
                    {
						ByteBuffer* response;

                        if (DEBUG_MMS_CLIENT)
                            printf("MMS_CLIENT: received file-read-request\n");

                        response = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

                        mmsClient_handleFileReadRequest(self, buf, bufPos, bufPos + length, invokeId, response);

                        IsoClientConnection_sendMessage(self->isoClient, response);

                        IsoClientConnection_releaseReceiveBuffer(self->isoClient);
                    }
                    break;

                case 0x4a: /* file-close-request */
                    {
						ByteBuffer* response;
                        if (DEBUG_MMS_CLIENT)
                            printf("MMS_CLIENT: received file-close-request\n");

                        response = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

                        mmsClient_handleFileCloseRequest(self, buf, bufPos, bufPos + length, invokeId, response);

                        IsoClientConnection_sendMessage(self->isoClient, response);

                        IsoClientConnection_releaseReceiveBuffer(self->isoClient);
                    }
                    break;
#endif /* MMS_FILE_SERVICE == 1 */

                default:
                    // mmsServer_writeMmsRejectPdu(&invokeId, MMS_ERROR_REJECT_UNRECOGNIZED_SERVICE, response);
                    if (DEBUG_MMS_CLIENT)
                        printf("MMS_CLIENT: unexpected message from server!\n");

                    IsoClientConnection_releaseReceiveBuffer(self->isoClient);

                    break;
                }
            }
            else {
                switch (nestedTag)
                {
                case 0x02: /* invoke Id */
                    invokeId = BerDecoder_decodeUint32(buf, length, bufPos);
                    if (DEBUG_MMS_CLIENT)
                        printf("MMS_CLIENT: received request with invokeId: %i\n", invokeId);
                    self->lastInvokeId = invokeId;
                    break;

                default:
                    //  mmsServer_writeMmsRejectPdu(&invokeId, MMS_ERROR_REJECT_UNRECOGNIZED_SERVICE, response);
                    if (DEBUG_MMS_CLIENT)
                        printf("MMS_CLIENT: unexpected message from server!\n");

                    IsoClientConnection_releaseReceiveBuffer(self->isoClient);

                    goto exit_with_error;

                    break;
                }
            }

            bufPos += length;
        }

    }
#endif /* (MMS_OBTAIN_FILE_SERVICE == 1) */

    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: LEAVE mmsIsoCallback - OK\n");

    return;

    exit_with_error:

    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: received malformed message from server!\n");

    IsoClientConnection_releaseReceiveBuffer(self->isoClient);

    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: LEAVE mmsIsoCallback - NOT OK!\n");
    return;
}

MmsConnection
MmsConnection_create()
{
    MmsConnection self = (MmsConnection) GLOBAL_CALLOC(1, sizeof(struct sMmsConnection));

	/* Load default values for connection parameters */
    TSelector tSelector = { 2, { 0, 1 } };
    SSelector sSelector = { 2, { 0, 1 } };

    self->parameters.dataStructureNestingLevel = -1;
    self->parameters.maxServOutstandingCalled = -1;
    self->parameters.maxServOutstandingCalling = -1;
    self->parameters.maxPduSize = -1;

    self->parameters.maxPduSize = CONFIG_MMS_MAXIMUM_PDU_SIZE;

    self->requestTimeout = CONFIG_MMS_CONNECTION_DEFAULT_TIMEOUT;

    self->lastInvokeIdLock = Semaphore_create(1);
    self->lastResponseLock = Semaphore_create(1);
    self->outstandingCallsLock = Semaphore_create(1);

    self->connectionStateLock = Semaphore_create(1);
    self->concludeStateLock = Semaphore_create(1);
    self->associationStateLock = Semaphore_create(1);

    self->lastResponseError = MMS_ERROR_NONE;

    self->outstandingCalls = (uint32_t*) GLOBAL_CALLOC(OUTSTANDING_CALLS, sizeof(uint32_t));

    self->isoParameters = IsoConnectionParameters_create();

    /* Load default values for connection parameters */
    //TSelector tSelector = { 2, { 0, 1 } };
    //SSelector sSelector = { 2, { 0, 1 } };

    IsoConnectionParameters_setLocalAddresses(self->isoParameters, 1, sSelector, tSelector);
    IsoConnectionParameters_setLocalApTitle(self->isoParameters, "1.1.1.999", 12);
    IsoConnectionParameters_setRemoteAddresses(self->isoParameters, 1, sSelector, tSelector);
    IsoConnectionParameters_setRemoteApTitle(self->isoParameters, "1.1.1.999.1", 12);

    self->connectTimeout = CONFIG_MMS_CONNECTION_DEFAULT_CONNECT_TIMEOUT;

    self->isoClient = IsoClientConnection_create((IsoIndicationCallback) mmsIsoCallback, (void*) self);

    return self;
}

MmsConnection
MmsConnection_createSecure(TLSConfiguration tlsConfig)
{
    MmsConnection self = MmsConnection_create();

#if (CONFIG_MMS_SUPPORT_TLS == 1)
    if (self != NULL) {
        IsoConnectionParameters connectionParameters = MmsConnection_getIsoConnectionParameters(self);

        TLSConfiguration_setClientMode(tlsConfig);

        IsoConnectionParameters_setTlsConfiguration(connectionParameters, tlsConfig);
    }
#endif /* (CONFIG_MMS_SUPPORT_TLS == 1) */

    return self;
}

void
MmsConnection_destroy(MmsConnection self)
{
    if (self->isoClient != NULL)
        IsoClientConnection_destroy(self->isoClient);

    if (self->isoParameters != NULL)
        IsoConnectionParameters_destroy(self->isoParameters);

    Semaphore_destroy(self->lastInvokeIdLock);
    Semaphore_destroy(self->lastResponseLock);
    Semaphore_destroy(self->outstandingCallsLock);

    Semaphore_destroy(self->associationStateLock);
    Semaphore_destroy(self->connectionStateLock);
    Semaphore_destroy(self->concludeStateLock);

    GLOBAL_FREEMEM(self->outstandingCalls);

#if (MMS_OBTAIN_FILE_SERVICE == 1)
#if (CONFIG_SET_FILESTORE_BASEPATH_AT_RUNTIME == 1)
    if (self->filestoreBasepath != NULL)
        GLOBAL_FREEMEM(self->filestoreBasepath);
#endif
#endif

    GLOBAL_FREEMEM(self);
}

void
MmsConnection_setFilestoreBasepath(MmsConnection self, const char* basepath)
{
#if (MMS_OBTAIN_FILE_SERVICE == 1)
#if (CONFIG_SET_FILESTORE_BASEPATH_AT_RUNTIME == 1)
    if (self->filestoreBasepath != NULL) {
        GLOBAL_FREEMEM(self->filestoreBasepath);
        self->filestoreBasepath = NULL;
    }

    if (basepath != NULL)
        self->filestoreBasepath = StringUtils_copyString(basepath);
#endif
#endif
}

char*
MmsConnection_getFilestoreBasepath(MmsConnection self)
{
#if (MMS_OBTAIN_FILE_SERVICE == 1)
#if (CONFIG_SET_FILESTORE_BASEPATH_AT_RUNTIME == 1)
    if (self->filestoreBasepath != NULL)
        return self->filestoreBasepath;
    else
        return CONFIG_VIRTUAL_FILESTORE_BASEPATH;
#else
    return CONFIG_VIRTUAL_FILESTORE_BASEPATH;
#endif

#else
    return CONFIG_VIRTUAL_FILESTORE_BASEPATH;
#endif
}

void
MmsConnection_setRawMessageHandler(MmsConnection self, MmsRawMessageHandler handler, void* parameter)
{
#if (CONFIG_MMS_RAW_MESSAGE_LOGGING == 1)
    self->rawMmsMessageHandler = (void*) handler;
    self->rawMmsMessageHandlerParameter = parameter;
#endif
}

void
MmsConnection_setConnectionLostHandler(MmsConnection self, MmsConnectionLostHandler handler, void* handlerParameter)
{
    self->connectionLostHandler = handler;
    self->connectionLostHandlerParameter = handlerParameter;
}

void
MmsConnection_setRequestTimeout(MmsConnection self, uint32_t timeoutInMs)
{
    self->requestTimeout = timeoutInMs;
}

void
MmsConnection_setConnectTimeout(MmsConnection self, uint32_t timeoutInMs)
{
    self->connectTimeout = timeoutInMs;
}

void
MmsConnection_setLocalDetail(MmsConnection self, int32_t localDetail)
{
    self->parameters.maxPduSize = localDetail;
}

int32_t
MmsConnection_getLocalDetail(MmsConnection self)
{
    return self->parameters.maxPduSize;
}

IsoConnectionParameters
MmsConnection_getIsoConnectionParameters(MmsConnection self)
{
    return self->isoParameters;
}

MmsConnectionParameters
MmsConnection_getMmsConnectionParameters(MmsConnection self)
{
    return self->parameters;
}

static void
waitForConnectResponse(MmsConnection self)
{
    uint64_t currentTime = Hal_getTimeInMs();

    uint64_t waitUntilTime = currentTime + self->requestTimeout;

    while (currentTime < waitUntilTime) {
        if (getConnectionState(self) != MMS_CON_WAITING)
            break;

        Thread_sleep(10);

        currentTime = Hal_getTimeInMs();
    }
}

bool
MmsConnection_connect(MmsConnection self, MmsError* mmsError, const char* serverName, int serverPort)
{
	ByteBuffer* payload;

    if (serverPort == -1) {
#if (CONFIG_MMS_SUPPORT_TLS == 1)
        if (self->isoParameters->tlsConfiguration)
            serverPort = 3782;
        else
            serverPort = 102;
#else
        serverPort = 102;
#endif
    }

    IsoConnectionParameters_setTcpParameters(self->isoParameters, serverName, serverPort);

    if (self->parameters.maxPduSize == -1)
        self->parameters.maxPduSize = CONFIG_MMS_MAXIMUM_PDU_SIZE;

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    mmsClient_createInitiateRequest(self, payload);

#if (CONFIG_MMS_RAW_MESSAGE_LOGGING == 1)
    if (self->rawMmsMessageHandler != NULL) {
        MmsRawMessageHandler handler = (MmsRawMessageHandler) self->rawMmsMessageHandler;
        handler(self->rawMmsMessageHandlerParameter, payload->buffer, payload->size, false);
    }
#endif /* (CONFIG_MMS_RAW_MESSAGE_LOGGING == 1) */

    setConnectionState(self, MMS_CON_WAITING);

    IsoClientConnection_associate(self->isoClient, self->isoParameters, payload,
            self->connectTimeout);

    waitForConnectResponse(self);

    if (DEBUG_MMS_CLIENT)
        printf("MmsConnection_connect: received response conState: %i assocState: %i\n", getConnectionState(self), getAssociationState(self));

    if (getConnectionState(self) == MMS_CON_ASSOCIATED) {
        if (mmsClient_parseInitiateResponse(self) == false) {
            if (DEBUG_MMS_CLIENT)
                printf("MmsConnection_connect: failed to parse initiate response\n");

            setAssociationState(self, MMS_STATE_CLOSED);

            setConnectionState(self, MMS_CON_ASSOCIATION_FAILED);
        }
        else {
            setAssociationState(self, MMS_STATE_CONNECTED);
        }

        releaseResponse(self);


    }
    else {
        setAssociationState(self, MMS_STATE_CLOSED);

        setConnectionState(self, MMS_CON_ASSOCIATION_FAILED);
    }


    if (DEBUG_MMS_CLIENT)
        printf("MmsConnection_connect: states: con %i ass %i\n", getConnectionState(self), getAssociationState(self));

    if (getAssociationState(self) == MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_NONE;
        return true;
    }
    else {
        *mmsError = MMS_ERROR_CONNECTION_REJECTED;
        return false;
    }
}

void
MmsConnection_close(MmsConnection self)
{
    self->connectionLostHandler = NULL;

    if (getAssociationState(self) == MMS_STATE_CONNECTED)
        IsoClientConnection_close(self->isoClient);
}

void
MmsConnection_abort(MmsConnection self, MmsError* mmsError)
{
	bool success = true;

    *mmsError = MMS_ERROR_NONE;

    self->connectionLostHandler = NULL;

    if (getAssociationState(self) == MMS_STATE_CONNECTED)
        success = IsoClientConnection_abort(self->isoClient);

    if (success == false) {
        IsoClientConnection_close(self->isoClient);
        *mmsError = MMS_ERROR_SERVICE_TIMEOUT;
    }
}

static void
sendConcludeRequestAndWaitForResponse(MmsConnection self)
{
    uint64_t startTime = Hal_getTimeInMs();

    uint64_t waitUntilTime = startTime + self->requestTimeout;

    uint64_t currentTime = startTime;

    bool success = false;

    ByteBuffer* concludeMessage = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    mmsClient_createConcludeRequest(self, concludeMessage);

    setConcludeState(self, CONCLUDE_STATE_REQUESTED);

    IsoClientConnection_sendMessage(self->isoClient, concludeMessage);

    while (currentTime < waitUntilTime) {

        if (getAssociationState(self) == MMS_STATE_CLOSED)
            goto exit_function;

        if (getConcludeState(self) != CONCLUDE_STATE_REQUESTED) {
            success = true;
            break;
        }

        Thread_sleep(1);

        currentTime = Hal_getTimeInMs();
    }

    if (!success) {
        if (DEBUG_MMS_CLIENT)
            printf("TIMEOUT for conclude request\n");
        self->lastResponseError = MMS_ERROR_SERVICE_TIMEOUT;
    }

    exit_function:
    return;
}

void
MmsConnection_conclude(MmsConnection self, MmsError* mmsError)
{
    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    *mmsError = MMS_ERROR_NONE;

    sendConcludeRequestAndWaitForResponse(self);

    if (self->lastResponseError != MMS_ERROR_NONE)
        *mmsError = self->lastResponseError;

    releaseResponse(self);

    if (getConcludeState(self) != CONCLUDE_STATE_ACCEPTED) {

        if (getAssociationState(self) == MMS_STATE_CLOSED)
            *mmsError = MMS_ERROR_CONNECTION_LOST;

        if (getConcludeState(self) == CONCLUDE_STATE_REJECTED)
            *mmsError = MMS_ERROR_CONCLUDE_REJECTED;
    }

    self->connectionLostHandler = NULL;

    exit_function:
    return;
}

void
MmsConnection_setInformationReportHandler(MmsConnection self, MmsInformationReportHandler handler,
        void* parameter)
{
    self->reportHandler = handler;
    self->reportHandlerParameter = parameter;
}

static bool
mmsClient_getNameListSingleRequest(
        LinkedList* nameList,
        MmsConnection self,
        MmsError* mmsError,
        const char* domainId,
        MmsObjectClass objectClass,
        bool associationSpecific,
        const char* continueAfter)
{
    bool moreFollows = false;
	uint32_t invokeId;
	ByteBuffer* payload;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    invokeId = getNextInvokeId(self);

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    if (associationSpecific)
        mmsClient_createMmsGetNameListRequestAssociationSpecific(invokeId,
                payload, continueAfter);
    else {

        if (objectClass == MMS_OBJECT_CLASS_DOMAIN)
            mmsClient_createMmsGetNameListRequestVMDspecific(invokeId,
                    payload, continueAfter);
        else
            mmsClient_createGetNameListRequestDomainOrVMDSpecific(invokeId, domainId,
                    payload, objectClass, continueAfter);
    }

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        moreFollows = mmsClient_parseGetNameListResponse(nameList, self->lastResponse, NULL);

    releaseResponse(self);

    exit_function:
    return moreFollows;
}

static LinkedList /* <char*> */
mmsClient_getNameList(MmsConnection self, MmsError *mmsError,
        const char* domainId,
        MmsObjectClass objectClass,
        bool associationSpecific)
{
    LinkedList list = NULL;

    bool moreFollows;

    moreFollows = mmsClient_getNameListSingleRequest(&list, self, mmsError, domainId,
            objectClass, associationSpecific, NULL);

    while ((moreFollows == true) && (list != NULL)) {
        LinkedList lastElement = LinkedList_getLastElement(list);

        char* lastIdentifier = (char*) lastElement->data;

        if (DEBUG_MMS_CLIENT)
            printf("getNameList: identifier: %s\n", lastIdentifier);

        moreFollows = mmsClient_getNameListSingleRequest(&list, self, mmsError, domainId,
                objectClass, associationSpecific, lastIdentifier);
    }

    return list;
}

LinkedList /* <char*> */
MmsConnection_getVMDVariableNames(MmsConnection self, MmsError* mmsError)
{
    return mmsClient_getNameList(self, mmsError, NULL, MMS_OBJECT_CLASS_NAMED_VARIABLE, false);
}

LinkedList /* <char*> */
MmsConnection_getDomainNames(MmsConnection self, MmsError* mmsError)
{
    return mmsClient_getNameList(self, mmsError, NULL, MMS_OBJECT_CLASS_DOMAIN, false);
}

LinkedList /* <char*> */
MmsConnection_getDomainVariableNames(MmsConnection self, MmsError* mmsError, const char* domainId)
{
    return mmsClient_getNameList(self, mmsError, domainId, MMS_OBJECT_CLASS_NAMED_VARIABLE, false);
}

LinkedList /* <char*> */
MmsConnection_getDomainVariableListNames(MmsConnection self, MmsError* mmsError, const char* domainId)
{
    return mmsClient_getNameList(self, mmsError, domainId, MMS_OBJECT_CLASS_NAMED_VARIABLE_LIST, false);
}

LinkedList /* <char*> */
MmsConnection_getDomainJournals(MmsConnection self, MmsError* mmsError, const char* domainId)
{
    return mmsClient_getNameList(self, mmsError, domainId, MMS_OBJECT_CLASS_JOURNAL, false);
}

LinkedList /* <char*> */
MmsConnection_getVariableListNamesAssociationSpecific(MmsConnection self, MmsError* mmsError)
{
    return mmsClient_getNameList(self, mmsError, NULL, MMS_OBJECT_CLASS_NAMED_VARIABLE_LIST, true);
}

MmsValue*
MmsConnection_readVariable(MmsConnection self, MmsError* mmsError,
        const char* domainId, const char* itemId)
{
    MmsValue* value = NULL;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createReadRequest(invokeId, domainId, itemId, payload);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        value = mmsClient_parseReadResponse(self->lastResponse, NULL, false);

    releaseResponse(self);

    exit_function:
    return value;
}

MmsValue*
MmsConnection_readVariableComponent(MmsConnection self, MmsError* mmsError,
        const char* domainId, const char* itemId, const char* componentId)
{
    MmsValue* value = NULL;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createReadRequestComponent(invokeId, domainId, itemId, componentId, payload);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        value = mmsClient_parseReadResponse(self->lastResponse, NULL, false);

    releaseResponse(self);

    exit_function:
    return value;
}

MmsValue*
MmsConnection_readArrayElements(MmsConnection self, MmsError* mmsError,
        const char* domainId, const char* itemId,
        uint32_t startIndex, uint32_t numberOfElements)
{
    MmsValue* value = NULL;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createReadRequestAlternateAccessIndex(invokeId, domainId, itemId, startIndex,
            numberOfElements, payload);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        value = mmsClient_parseReadResponse(self->lastResponse, NULL, false);

    releaseResponse(self);

exit_function:
    return value;
}

MmsValue*
MmsConnection_readSingleArrayElementWithComponent(MmsConnection self, MmsError* mmsError,
        const char* domainId, const char* itemId, uint32_t index, const char* componentId)
{
    MmsValue* value = NULL;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createReadRequestAlternateAccessSingleIndexComponent(invokeId, domainId, itemId, index, componentId,
            payload);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        value = mmsClient_parseReadResponse(self->lastResponse, NULL, false);

    releaseResponse(self);

exit_function:
    return value;
}

MmsValue*
MmsConnection_readMultipleVariables(MmsConnection self, MmsError* mmsError,
        const char* domainId, LinkedList /*<char*>*/items)
{
    MmsValue* value = NULL;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createReadRequestMultipleValues(invokeId, domainId, items, payload);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        value = mmsClient_parseReadResponse(self->lastResponse, NULL, true);

    releaseResponse(self);

    exit_function:
    return value;
}

MmsValue*
MmsConnection_readNamedVariableListValues(MmsConnection self, MmsError* mmsError,
        const char* domainId, const char* listName,
        bool specWithResult)
{
    MmsValue* value = NULL;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createReadNamedVariableListRequest(invokeId, domainId, listName,
            payload, specWithResult);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL) {
        value = mmsClient_parseReadResponse(self->lastResponse, NULL, true);

        if (value == NULL)
            *mmsError = MMS_ERROR_PARSING_RESPONSE;
    }

    releaseResponse(self);

    exit_function:
    return value;
}

MmsValue*
MmsConnection_readNamedVariableListValuesAssociationSpecific(
        MmsConnection self, MmsError* mmsError,
        const char* listName,
        bool specWithResult)
{
    MmsValue* value = NULL;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createReadAssociationSpecificNamedVariableListRequest(invokeId, listName,
            payload, specWithResult);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        value = mmsClient_parseReadResponse(self->lastResponse, NULL, true);

    releaseResponse(self);

    exit_function:
    return value;
}

LinkedList /* <MmsVariableAccessSpecification*> */
MmsConnection_readNamedVariableListDirectory(MmsConnection self, MmsError* mmsError,
        const char* domainId, const char* listName, bool* deletable)
{
    LinkedList attributes = NULL;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createGetNamedVariableListAttributesRequest(invokeId, payload, domainId,
            listName);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        attributes = mmsClient_parseGetNamedVariableListAttributesResponse(self->lastResponse, NULL,
                deletable);

    releaseResponse(self);

    exit_function:
    return attributes;
}

LinkedList /* <MmsVariableAccessSpecification*> */
MmsConnection_readNamedVariableListDirectoryAssociationSpecific(MmsConnection self, MmsError* mmsError,
        const char* listName, bool* deletable)
{
    LinkedList attributes = NULL;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createGetNamedVariableListAttributesRequestAssociationSpecific(invokeId, payload,
            listName);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        attributes = mmsClient_parseGetNamedVariableListAttributesResponse(self->lastResponse, NULL,
                deletable);

    releaseResponse(self);

    exit_function:
    return attributes;
}

void
MmsConnection_defineNamedVariableList(MmsConnection self, MmsError* mmsError,
        const char* domainId, const char* listName, LinkedList variableSpecs)
{
    ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

	if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createDefineNamedVariableListRequest(invokeId, payload, domainId,
            listName, variableSpecs, false);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        if (!mmsClient_parseDefineNamedVariableResponse(self->lastResponse, NULL))
            *mmsError = MMS_ERROR_PARSING_RESPONSE;

    releaseResponse(self);

    exit_function:
    return;
}

void
MmsConnection_defineNamedVariableListAssociationSpecific(MmsConnection self,
        MmsError* mmsError, const char* listName, LinkedList variableSpecs)
{
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createDefineNamedVariableListRequest(invokeId, payload, NULL,
            listName, variableSpecs, true);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        if (!mmsClient_parseDefineNamedVariableResponse(self->lastResponse, NULL))
            *mmsError = MMS_ERROR_PARSING_RESPONSE;

    releaseResponse(self);

    exit_function:
    return;
}

bool
MmsConnection_deleteNamedVariableList(MmsConnection self, MmsError* mmsError,
        const char* domainId, const char* listName)
{
    bool isDeleted = false;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createDeleteNamedVariableListRequest(invokeId, payload, domainId, listName);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        if (mmsClient_parseDeleteNamedVariableListResponse(self->lastResponse, NULL))
            isDeleted = true;

    releaseResponse(self);

    exit_function:
    return isDeleted;
}

bool
MmsConnection_deleteAssociationSpecificNamedVariableList(MmsConnection self,
        MmsError* mmsError, const char* listName)
{
    bool isDeleted = false;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createDeleteAssociationSpecificNamedVariableListRequest(
            invokeId, payload, listName);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        if (mmsClient_parseDeleteNamedVariableListResponse(self->lastResponse, NULL))
            isDeleted = true;

    releaseResponse(self);

    exit_function:
    return isDeleted;
}

MmsVariableSpecification*
MmsConnection_getVariableAccessAttributes(MmsConnection self, MmsError* mmsError,
        const char* domainId, const char* itemId)
{
    MmsVariableSpecification* typeSpec = NULL;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createGetVariableAccessAttributesRequest(invokeId, domainId, itemId, payload);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        typeSpec = mmsClient_parseGetVariableAccessAttributesResponse(self->lastResponse, NULL);

    releaseResponse(self);

    exit_function:
    return typeSpec;
}

MmsServerIdentity*
MmsConnection_identify(MmsConnection self, MmsError* mmsError)
{
    MmsServerIdentity* identity = NULL;
	ByteBuffer* payload;
	uint32_t invokeId;
	ByteBuffer* responseMessage;

    if (getAssociationState(self) != MMS_STATE_CONNECTED) {
        *mmsError = MMS_ERROR_CONNECTION_LOST;
        goto exit_function;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createIdentifyRequest(invokeId, payload);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        identity = mmsClient_parseIdentifyResponse(self);

    releaseResponse(self);

    exit_function:
    return identity;
}

void
MmsConnection_getServerStatus(MmsConnection self, MmsError* mmsError, int* vmdLogicalStatus, int* vmdPhysicalStatus,
bool extendedDerivation)
{
    ByteBuffer* payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    uint32_t invokeId = getNextInvokeId(self);

	ByteBuffer* responseMessage;

    mmsClient_createStatusRequest(invokeId, payload, extendedDerivation);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL) {
        if (mmsClient_parseStatusResponse(self, vmdLogicalStatus, vmdPhysicalStatus) == false)
            *mmsError = MMS_ERROR_PARSING_RESPONSE;
    }

    releaseResponse(self);
}

static LinkedList
readJournal(MmsConnection self, MmsError* mmsError, uint32_t invokeId, ByteBuffer* payload, bool* moreFollows)
{
    ByteBuffer* responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    LinkedList response = NULL;

    if (responseMessage != NULL) {

        if (mmsClient_parseReadJournalResponse(self, moreFollows, &response) == false)
            *mmsError = MMS_ERROR_PARSING_RESPONSE;
    }

    releaseResponse(self);

    return response;
}

static void
MmsJournalVariable_destroy(MmsJournalVariable self)
{
    if (self != NULL) {
        GLOBAL_FREEMEM(self->tag);
        MmsValue_delete(self->value);
        GLOBAL_FREEMEM(self);
    }
}

void
MmsJournalEntry_destroy(MmsJournalEntry self)
{
    if (self != NULL) {
        MmsValue_delete(self->entryID);
        MmsValue_delete(self->occurenceTime);
        LinkedList_destroyDeep(self->journalVariables,
                (LinkedListValueDeleteFunction) MmsJournalVariable_destroy);
        GLOBAL_FREEMEM(self);
    }
}

const MmsValue*
MmsJournalEntry_getEntryID(MmsJournalEntry self)
{
    return self->entryID;
}

const MmsValue*
MmsJournalEntry_getOccurenceTime(MmsJournalEntry self)
{
    return self->occurenceTime;
}

const LinkedList /* <MmsJournalVariable> */
MmsJournalEntry_getJournalVariables(MmsJournalEntry self)
{
    return self->journalVariables;
}

const char*
MmsJournalVariable_getTag(MmsJournalVariable self)
{
    return self->tag;
}

const MmsValue*
MmsJournalVariable_getValue(MmsJournalVariable self)
{
    return self->value;
}

LinkedList
MmsConnection_readJournalTimeRange(MmsConnection self, MmsError* mmsError, const char* domainId, const char* itemId,
        MmsValue* startingTime, MmsValue* endingTime, bool* moreFollows)
{
    ByteBuffer* payload;
	uint32_t invokeId;
	
	if ((MmsValue_getType(startingTime) != MMS_BINARY_TIME) ||
            (MmsValue_getType(endingTime) != MMS_BINARY_TIME)) {

        *mmsError = MMS_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createReadJournalRequestWithTimeRange(invokeId, payload, domainId, itemId, startingTime, endingTime);

    return readJournal(self, mmsError, invokeId, payload, moreFollows);
}

LinkedList
MmsConnection_readJournalStartAfter(MmsConnection self, MmsError* mmsError, const char* domainId, const char* itemId,
        MmsValue* timeSpecification, MmsValue* entrySpecification, bool* moreFollows)
{

	ByteBuffer* payload;
	uint32_t invokeId;

    if ((MmsValue_getType(timeSpecification) != MMS_BINARY_TIME) ||
            (MmsValue_getType(entrySpecification) != MMS_OCTET_STRING)) {

        *mmsError = MMS_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }

    payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    invokeId = getNextInvokeId(self);

    mmsClient_createReadJournalRequestStartAfter(invokeId, payload, domainId, itemId, timeSpecification, entrySpecification);

    return readJournal(self, mmsError, invokeId, payload, moreFollows);
}

int32_t
MmsConnection_fileOpen(MmsConnection self, MmsError* mmsError, const char* filename, uint32_t initialPosition,
        uint32_t* fileSize, uint64_t* lastModified)
{
#if (MMS_FILE_SERVICE == 1)
    ByteBuffer* payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    uint32_t invokeId = getNextInvokeId(self);

    int32_t frsmId = -1;

	ByteBuffer* responseMessage;

    mmsClient_createFileOpenRequest(invokeId, payload, filename, initialPosition);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL) {

        uint8_t* buffer = self->lastResponse->buffer;
        int maxBufPos = self->lastResponse->size;
        int bufPos = self->lastResponseBufPos;

        if (mmsMsg_parseFileOpenResponse(buffer, bufPos, maxBufPos, &frsmId, fileSize, lastModified) == false)
            *mmsError = MMS_ERROR_PARSING_RESPONSE;
    }

    releaseResponse(self);

    return frsmId;
#else
    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: service not supported\n");

    *mmsError = MMS_ERROR_OTHER;
    return 0;
#endif
}

void
MmsConnection_fileClose(MmsConnection self, MmsError* mmsError, int32_t frsmId)
{
#if (MMS_FILE_SERVICE == 1)
    ByteBuffer* payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    uint32_t invokeId = getNextInvokeId(self);

    mmsClient_createFileCloseRequest(invokeId, payload, frsmId);

    sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    /* nothing to do - response contains no data to evaluate */

    releaseResponse(self);
#else
    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: service not supported\n");

    *mmsError = MMS_ERROR_OTHER;
#endif
}

void
MmsConnection_fileDelete(MmsConnection self, MmsError* mmsError, const char* fileName)
{
#if (MMS_FILE_SERVICE == 1)
    ByteBuffer* payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    uint32_t invokeId = getNextInvokeId(self);

    mmsClient_createFileDeleteRequest(invokeId, payload, fileName);

    sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    /* nothing to do - response contains no data to evaluate */

    releaseResponse(self);
#else
    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: service not supported\n");

    *mmsError = MMS_ERROR_OTHER;
#endif
}

bool
MmsConnection_fileRead(MmsConnection self, MmsError* mmsError, int32_t frsmId, MmsFileReadHandler handler,
        void* handlerParameter)
{
#if (MMS_FILE_SERVICE == 1)
    ByteBuffer* payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    uint32_t invokeId = getNextInvokeId(self);

    bool moreFollows = false;

	ByteBuffer* responseMessage;

    mmsClient_createFileReadRequest(invokeId, payload, frsmId);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL) {
        uint8_t* buffer = self->lastResponse->buffer;
        int maxBufPos = self->lastResponse->size;
        int bufPos = self->lastResponseBufPos;

        if (mmsMsg_parseFileReadResponse(buffer, bufPos, maxBufPos, frsmId, &moreFollows, handler, handlerParameter) == false)
            *mmsError = MMS_ERROR_PARSING_RESPONSE;
    }

    releaseResponse(self);

    return moreFollows;
#else
    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: service not supported\n");

    *mmsError = MMS_ERROR_OTHER;
    return false;
#endif
}

bool
MmsConnection_getFileDirectory(MmsConnection self, MmsError* mmsError, const char* fileSpecification, const char* continueAfter,
        MmsFileDirectoryHandler handler, void* handlerParameter)
{
#if (MMS_FILE_SERVICE == 1)
    ByteBuffer* payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    uint32_t invokeId = getNextInvokeId(self);

	ByteBuffer* responseMessage;
	bool moreFollows;

    mmsClient_createFileDirectoryRequest(invokeId, payload, fileSpecification, continueAfter);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    moreFollows = false;

    if (responseMessage != NULL) {
        if (mmsClient_parseFileDirectoryResponse(self, handler, handlerParameter, &moreFollows) == false)
            *mmsError = MMS_ERROR_PARSING_RESPONSE;
    }

    releaseResponse(self);

    return moreFollows;
#else
    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: service not supported\n");

    *mmsError = MMS_ERROR_OTHER;
    return false;
#endif
}

void
MmsConnection_fileRename(MmsConnection self, MmsError* mmsError, const char* currentFileName, const char* newFileName)
{
#if (MMS_FILE_SERVICE == 1)
    ByteBuffer* payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    uint32_t invokeId = getNextInvokeId(self);

    mmsClient_createFileRenameRequest(invokeId, payload, currentFileName, newFileName);

    sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    /* nothing to do - response contains no data to evaluate */

    releaseResponse(self);
#else
    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: service not supported\n");

    *mmsError = MMS_ERROR_OTHER;
#endif
}

void
MmsConnection_obtainFile(MmsConnection self, MmsError* mmsError, const char* sourceFile, const char* destinationFile)
{
#if ((MMS_FILE_SERVICE == 1) && (MMS_OBTAIN_FILE_SERVICE == 1))
    ByteBuffer* payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    uint32_t invokeId = getNextInvokeId(self);

    mmsClient_createObtainFileRequest(invokeId, payload, sourceFile, destinationFile);

    sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    /* nothing to do - response contains no data to evaluate */

    releaseResponse(self);
#else
    if (DEBUG_MMS_CLIENT)
        printf("MMS_CLIENT: service not supported\n");

    *mmsError = MMS_ERROR_OTHER;
#endif
}

MmsDataAccessError
MmsConnection_writeVariable(MmsConnection self, MmsError* mmsError,
        const char* domainId, const char* itemId,
        MmsValue* value)
{
    MmsDataAccessError retVal = DATA_ACCESS_ERROR_UNKNOWN;

    ByteBuffer* payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

    uint32_t invokeId = getNextInvokeId(self);
	ByteBuffer* responseMessage;

    mmsClient_createWriteRequest(invokeId, domainId, itemId, value, payload);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        retVal = mmsClient_parseWriteResponse(self->lastResponse, self->lastResponseBufPos, mmsError);

    releaseResponse(self);

    return retVal;
}

void
MmsConnection_writeMultipleVariables(MmsConnection self, MmsError* mmsError, const char* domainId,
        LinkedList /*<char*>*/items,
        LinkedList /* <MmsValue*> */values,
        /* OUTPUT */LinkedList* /* <MmsValue*> */accessResults)
{
    uint32_t invokeId = getNextInvokeId(self);

    ByteBuffer* payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

	ByteBuffer* responseMessage;

    mmsClient_createWriteMultipleItemsRequest(invokeId, domainId, items, values, payload);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL) {

        int numberOfItems = LinkedList_size(items);

        mmsClient_parseWriteMultipleItemsResponse(self->lastResponse, self->lastResponseBufPos, mmsError,
                numberOfItems, accessResults);
    }

    releaseResponse(self);
}

MmsDataAccessError
MmsConnection_writeArrayElements(MmsConnection self, MmsError* mmsError,
        const char* domainId, const char* itemId, int index, int numberOfElements,
        MmsValue* value)
{
    MmsDataAccessError retVal = DATA_ACCESS_ERROR_UNKNOWN;

    uint32_t invokeId = getNextInvokeId(self);

    ByteBuffer* payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

	ByteBuffer* responseMessage;

    mmsClient_createWriteRequestArray(invokeId, domainId, itemId, index, numberOfElements, value, payload);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL)
        retVal = mmsClient_parseWriteResponse(self->lastResponse, self->lastResponseBufPos, mmsError);

    releaseResponse(self);

    return retVal;
}

void
MmsConnection_writeNamedVariableList(MmsConnection self, MmsError* mmsError, bool isAssociationSpecific,
        const char* domainId, const char* itemId, LinkedList /* <MmsValue*> */values,
        /* OUTPUT */LinkedList* /* <MmsValue*> */accessResults)
{
    uint32_t invokeId = getNextInvokeId(self);

    ByteBuffer* payload = IsoClientConnection_allocateTransmitBuffer(self->isoClient);

	ByteBuffer* responseMessage;

    mmsClient_createWriteRequestNamedVariableList(invokeId, isAssociationSpecific, domainId, itemId, values, payload);

    responseMessage = sendRequestAndWaitForResponse(self, invokeId, payload, mmsError);

    if (responseMessage != NULL) {

        int numberOfItems = LinkedList_size(values);

        mmsClient_parseWriteMultipleItemsResponse(self->lastResponse, self->lastResponseBufPos, mmsError,
                numberOfItems, accessResults);
    }

    releaseResponse(self);
}

void
MmsServerIdentity_destroy(MmsServerIdentity* self)
{
    if (self->modelName != NULL)
        GLOBAL_FREEMEM(self->modelName);

    if (self->vendorName != NULL)
        GLOBAL_FREEMEM(self->vendorName);

    if (self->revision != NULL)
        GLOBAL_FREEMEM(self->revision);

    GLOBAL_FREEMEM(self);
}

MmsVariableAccessSpecification*
MmsVariableAccessSpecification_create(char* domainId, char* itemId)
{
    MmsVariableAccessSpecification* self = (MmsVariableAccessSpecification*)
            GLOBAL_MALLOC(sizeof(MmsVariableAccessSpecification));

    self->domainId = domainId;
    self->itemId = itemId;
    self->arrayIndex = -1;
    self->componentName = NULL;

    return self;
}

MmsVariableAccessSpecification*
MmsVariableAccessSpecification_createAlternateAccess(char* domainId, char* itemId, int32_t index,
        char* componentName)
{
    MmsVariableAccessSpecification* self = (MmsVariableAccessSpecification*)
            GLOBAL_MALLOC(sizeof(MmsVariableAccessSpecification));

    self->domainId = domainId;
    self->itemId = itemId;
    self->arrayIndex = index;
    self->componentName = componentName;

    return self;
}

void
MmsVariableAccessSpecification_destroy(MmsVariableAccessSpecification* self)
{
    if (self->domainId != NULL)
        GLOBAL_FREEMEM((void* ) self->domainId);

    if (self->itemId != NULL)
        GLOBAL_FREEMEM((void* ) self->itemId);

    if (self->componentName != NULL)
        GLOBAL_FREEMEM((void* ) self->componentName);

    GLOBAL_FREEMEM(self);
}

void
MmsConnection_setIsoConnectionParameters(MmsConnection self, IsoConnectionParameters* params)
{
	self->isoParameters = params;
}

