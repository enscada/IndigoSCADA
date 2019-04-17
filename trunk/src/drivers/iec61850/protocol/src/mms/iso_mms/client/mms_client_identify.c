/*
 *  mms_client_identify.c
 *
 *  Copyright 2013 Michael Zillgith
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
#include "stack_config.h"
#include "mms_common.h"
#include "mms_client_connection.h"
#include "byte_buffer.h"

#include "mms_client_internal.h"
#include "ber_encoder.h"
#include "ber_decode.h"

void
mmsClient_createIdentifyRequest(uint32_t invokeId, ByteBuffer* request)
{
    uint32_t invokeIdSize = BerEncoder_UInt32determineEncodedSize(invokeId);
    uint32_t confirmedRequestPduSize = 2 + 2 + invokeIdSize;

    int bufPos = 0;
    uint8_t* buffer = request->buffer;

    bufPos = BerEncoder_encodeTL(0xa0, confirmedRequestPduSize, buffer, bufPos);
    bufPos = BerEncoder_encodeTL(0x02, invokeIdSize, buffer, bufPos);
    bufPos = BerEncoder_encodeUInt32(invokeId, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0x82, 0, buffer, bufPos);

    request->size = bufPos;
}

MmsServerIdentity*
mmsClient_parseIdentifyResponse(MmsConnection self)
{
    uint8_t* buffer = self->lastResponse->buffer;
    int maxBufPos = self->lastResponse->size;
    int bufPos = self->lastResponseBufPos;
    int length;
    MmsServerIdentity* identityInfo = NULL;
    uint8_t tag = buffer[bufPos++];
	int endPos;
	char* vendorName = NULL;
    char* modelName = NULL;
    char* revision = NULL;

    if (tag != 0xa2)
        goto exit_error;

    bufPos = BerDecoder_decodeLength(buffer, &length, bufPos, maxBufPos);
    if (bufPos < 0) goto exit_error;

    endPos = bufPos + length;

    if (endPos > maxBufPos) {
        if (DEBUG_MMS_CLIENT)
            printf("mmsClient_parseIdentifyResponse: Message to short!\n");
        goto exit_error;
    }

    while (bufPos < endPos) {
        tag = buffer[bufPos++];

        bufPos = BerDecoder_decodeLength(buffer, &length, bufPos, maxBufPos);
        if (bufPos < 0) goto exit_error;

        switch (tag) {
        case 0x80: /* vendorName */
            vendorName = StringUtils_createStringFromBuffer(buffer + bufPos, length);
            bufPos += length;
            break;
        case 0x81: /* modelName */
            modelName = StringUtils_createStringFromBuffer(buffer + bufPos, length);
            bufPos += length;
            break;
        case 0x82: /* revision */
            revision = StringUtils_createStringFromBuffer(buffer + bufPos, length);
            bufPos += length;
            break;
        case 0x83: /* list of abstract syntaxes */
            bufPos += length;
            break;
        default: /* ignore unknown tags */
        	bufPos += length;
        	break;
        }
    }

    identityInfo = (MmsServerIdentity*) GLOBAL_MALLOC(sizeof(MmsServerIdentity));

    identityInfo->vendorName = vendorName;
    identityInfo->modelName = modelName;
    identityInfo->revision = revision;

exit_error:
    return identityInfo;
}



