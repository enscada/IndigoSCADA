#include "ua_util.h"
#include "ua_securechannel.h"
#include "ua_session.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_transport_generated_encoding_binary.h"
#include "ua_types_generated_handling.h"
#include "ua_transport_generated_handling.h"

#define UA_SECURE_MESSAGE_HEADER_LENGTH 24

void UA_SecureChannel_init(UA_SecureChannel *channel) {
    memset(channel, 0, sizeof(UA_SecureChannel));
    LIST_INIT(&channel->sessions);
    LIST_INIT(&channel->chunks);
}

void UA_SecureChannel_deleteMembersCleanup(UA_SecureChannel *channel) {
    /* Delete members */
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&channel->serverAsymAlgSettings);
    UA_ByteString_deleteMembers(&channel->serverNonce);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&channel->clientAsymAlgSettings);
    UA_ByteString_deleteMembers(&channel->clientNonce);
    UA_ChannelSecurityToken_deleteMembers(&channel->securityToken);
    UA_ChannelSecurityToken_deleteMembers(&channel->nextSecurityToken);

    /* Detach from the channel */
    if(channel->connection)
        UA_Connection_detachSecureChannel(channel->connection);

    /* Remove session pointers (not the sessions) */
    struct SessionEntry *se, *temp;
    LIST_FOREACH_SAFE(se, &channel->sessions, pointers, temp) {
        if(se->session)
            se->session->channel = NULL;
        LIST_REMOVE(se, pointers);
        UA_free(se);
    }

    /* Remove the buffered chunks */
    struct ChunkEntry *ch, *temp_ch;
    LIST_FOREACH_SAFE(ch, &channel->chunks, pointers, temp_ch) {
        UA_ByteString_deleteMembers(&ch->bytes);
        LIST_REMOVE(ch, pointers);
        UA_free(ch);
    }
}

//TODO implement real nonce generator - DUMMY function
UA_StatusCode UA_SecureChannel_generateNonce(UA_ByteString *nonce) {
    if(!(nonce->data = UA_malloc(1)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    nonce->length  = 1;
    nonce->data[0] = 'a';
    return UA_STATUSCODE_GOOD;
}

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

void UA_SecureChannel_attachSession(UA_SecureChannel *channel, UA_Session *session) {
    struct SessionEntry *se = UA_malloc(sizeof(struct SessionEntry));
    if(!se)
        return;
    se->session = session;
    if(UA_atomic_cmpxchg((void**)&session->channel, NULL, channel) != NULL) {
        UA_free(se);
        return;
    }
    LIST_INSERT_HEAD(&channel->sessions, se, pointers);
}

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

void UA_SecureChannel_detachSession(UA_SecureChannel *channel, UA_Session *session) {
    if(session)
        session->channel = NULL;
    struct SessionEntry *se;
    LIST_FOREACH(se, &channel->sessions, pointers) {
        if(se->session == session)
            break;
    }
    if(!se)
        return;
    LIST_REMOVE(se, pointers);
    UA_free(se);
}

UA_Session * UA_SecureChannel_getSession(UA_SecureChannel *channel, UA_NodeId *token) {
    struct SessionEntry *se;
    LIST_FOREACH(se, &channel->sessions, pointers) {
        if(UA_NodeId_equal(&se->session->authenticationToken, token))
            break;
    }
    if(!se)
        return NULL;
    return se->session;
}

void UA_SecureChannel_revolveTokens(UA_SecureChannel *channel) {
    if(channel->nextSecurityToken.tokenId == 0) //no security token issued
        return;

    //FIXME: not thread-safe
    memcpy(&channel->securityToken, &channel->nextSecurityToken,
           sizeof(UA_ChannelSecurityToken));
    UA_ChannelSecurityToken_init(&channel->nextSecurityToken);
}

/***********************/
/* Send Binary Message */
/***********************/

static UA_StatusCode
UA_SecureChannel_sendChunk(UA_ChunkInfo *ci, UA_ByteString *dst, size_t offset) {
    UA_SecureChannel *channel = ci->channel;
    UA_Connection *connection = channel->connection;
    if(!connection)
       return UA_STATUSCODE_BADINTERNALERROR;

    /* adjust the buffer where the header was hidden */
    dst->data = &dst->data[-UA_SECURE_MESSAGE_HEADER_LENGTH];
    dst->length += UA_SECURE_MESSAGE_HEADER_LENGTH;
    offset += UA_SECURE_MESSAGE_HEADER_LENGTH;

    if(ci->messageSizeSoFar + offset > connection->remoteConf.maxMessageSize &&
       connection->remoteConf.maxMessageSize > 0)
        ci->errorCode = UA_STATUSCODE_BADRESPONSETOOLARGE;
    if(++ci->chunksSoFar > connection->remoteConf.maxChunkCount &&
       connection->remoteConf.maxChunkCount > 0)
        ci->errorCode = UA_STATUSCODE_BADRESPONSETOOLARGE;

    /* Prepare the chunk headers */
    UA_SecureConversationMessageHeader respHeader;
    respHeader.secureChannelId = channel->securityToken.channelId;
    respHeader.messageHeader.messageTypeAndChunkType = ci->messageType;
    if(ci->errorCode == UA_STATUSCODE_GOOD) {
        if(ci->final)
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_FINAL;
        else
            respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_INTERMEDIATE;
    } else {
        /* abort message */
        ci->final = true; /* mark as finished */
        respHeader.messageHeader.messageTypeAndChunkType += UA_CHUNKTYPE_ABORT;
        UA_String errorMsg;
        UA_String_init(&errorMsg);
        offset = UA_SECURE_MESSAGE_HEADER_LENGTH;
        UA_UInt32_encodeBinary(&ci->errorCode, dst, &offset);
        UA_String_encodeBinary(&errorMsg, dst, &offset);
    }
    respHeader.messageHeader.messageSize = (UA_UInt32)offset;
    ci->messageSizeSoFar += offset;

    /* Encode the header at the beginning of the buffer */
    UA_SymmetricAlgorithmSecurityHeader symSecHeader;
    symSecHeader.tokenId = channel->securityToken.tokenId;
    UA_SequenceHeader seqHeader;
    seqHeader.requestId = ci->requestId;
    seqHeader.sequenceNumber = UA_atomic_add(&channel->sendSequenceNumber, 1);
    size_t offset_header = 0;
    UA_SecureConversationMessageHeader_encodeBinary(&respHeader, dst, &offset_header);
    UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symSecHeader, dst, &offset_header);
    UA_SequenceHeader_encodeBinary(&seqHeader, dst, &offset_header);

    /* Send the chunk, the buffer is freed in the network layer */
    dst->length = offset; /* set the buffer length to the content length */
    connection->send(channel->connection, dst);

    /* Replace with the buffer for the next chunk */
    if(!ci->final) {
        UA_StatusCode retval =
            connection->getSendBuffer(connection, connection->localConf.sendBufferSize, dst);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        /* Hide the header of the buffer, so that the ensuing encoding does not overwrite anything */
        dst->data = &dst->data[UA_SECURE_MESSAGE_HEADER_LENGTH];
        dst->length = connection->localConf.sendBufferSize - UA_SECURE_MESSAGE_HEADER_LENGTH;
    }
    return ci->errorCode;
}

UA_StatusCode
UA_SecureChannel_sendBinaryMessage(UA_SecureChannel *channel, UA_UInt32 requestId,
                                   const void *content, const UA_DataType *contentType) {
    UA_Connection *connection = channel->connection;
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the message buffer */
    UA_ByteString message;
    UA_StatusCode retval =
        connection->getSendBuffer(connection, connection->localConf.sendBufferSize, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Hide the message beginning where the header will be encoded */
    message.data = &message.data[UA_SECURE_MESSAGE_HEADER_LENGTH];
    message.length -= UA_SECURE_MESSAGE_HEADER_LENGTH;

    /* Encode the message type */
    size_t messagePos = 0;
    UA_NodeId typeId = contentType->typeId; /* always numeric */
    typeId.identifier.numeric = contentType->binaryEncodingId;
    UA_NodeId_encodeBinary(&typeId, &message, &messagePos);

    /* Encode with the chunking callback */
    UA_ChunkInfo ci;
    ci.channel = channel;
    ci.requestId = requestId;
    ci.chunksSoFar = 0;
    ci.messageSizeSoFar = 0;
    ci.final = false;
    ci.messageType = UA_MESSAGETYPE_MSG;
    ci.errorCode = UA_STATUSCODE_GOOD;
    if(typeId.identifier.numeric == 446 || typeId.identifier.numeric == 449)
        ci.messageType = UA_MESSAGETYPE_OPN;
    else if(typeId.identifier.numeric == 452 || typeId.identifier.numeric == 455)
        ci.messageType = UA_MESSAGETYPE_CLO;
    retval = UA_encodeBinary(content, contentType,
                             (UA_exchangeEncodeBuffer)UA_SecureChannel_sendChunk,
                             &ci, &message, &messagePos);

    /* Encoding failed, release the message */
    if(retval != UA_STATUSCODE_GOOD) {
        if(!ci.final) {
            /* the abort message was not send */
            ci.errorCode = retval;
            UA_SecureChannel_sendChunk(&ci, &message, messagePos);
        }
        return retval;
    }

    /* Encoding finished, send the final chunk */
    ci.final = UA_TRUE;
    return UA_SecureChannel_sendChunk(&ci, &message, messagePos);
}

/***************************/
/* Process Received Chunks */
/***************************/

static void
UA_SecureChannel_removeChunk(UA_SecureChannel *channel, UA_UInt32 requestId) {
    struct ChunkEntry *ch;
    LIST_FOREACH(ch, &channel->chunks, pointers) {
        if(ch->requestId == requestId) {
            UA_ByteString_deleteMembers(&ch->bytes);
            LIST_REMOVE(ch, pointers);
            UA_free(ch);
            return;
        }
    }
}

/* assume that chunklength fits */
static void
appendChunk(struct ChunkEntry *ch, const UA_ByteString *msg,
            size_t offset, size_t chunklength) {
    UA_Byte* new_bytes = UA_realloc(ch->bytes.data, ch->bytes.length + chunklength);
    if(!new_bytes) {
        UA_ByteString_deleteMembers(&ch->bytes);
        return;
    }
    ch->bytes.data = new_bytes;
    memcpy(&ch->bytes.data[ch->bytes.length], &msg->data[offset], chunklength);
    ch->bytes.length += chunklength;
}

static void
UA_SecureChannel_appendChunk(UA_SecureChannel *channel, UA_UInt32 requestId,
                             const UA_ByteString *msg, size_t offset,
                             size_t chunklength) {
    /* Check if the chunk fits into the message */
    if(msg->length - offset < chunklength) {
        /* can't process all chunks for that request */
        UA_SecureChannel_removeChunk(channel, requestId);
        return;
    }

    /* Get the chunkentry */
    struct ChunkEntry *ch;
    LIST_FOREACH(ch, &channel->chunks, pointers) {
        if(ch->requestId == requestId)
            break;
    }

    /* No chunkentry on the channel, create one */
    if(!ch) {
        ch = UA_malloc(sizeof(struct ChunkEntry));
        if(!ch)
            return;
        ch->requestId = requestId;
        UA_ByteString_init(&ch->bytes);
        LIST_INSERT_HEAD(&channel->chunks, ch, pointers);
    }

    appendChunk(ch, msg, offset, chunklength);
}

static UA_ByteString
UA_SecureChannel_finalizeChunk(UA_SecureChannel *channel, UA_UInt32 requestId,
                               const UA_ByteString *msg, size_t offset,
                               size_t chunklength, UA_Boolean *deleteChunk) {
    if(msg->length - offset < chunklength) {
        /* can't process all chunks for that request */
        UA_SecureChannel_removeChunk(channel, requestId);
        return UA_BYTESTRING_NULL;
    }

    struct ChunkEntry *ch;
    LIST_FOREACH(ch, &channel->chunks, pointers) {
        if(ch->requestId == requestId)
            break;
    }

    UA_ByteString bytes;
    if(!ch) {
        *deleteChunk = false;
        bytes.length = chunklength;
        bytes.data = msg->data + offset;
    } else {
        *deleteChunk = true;
        appendChunk(ch, msg, offset, chunklength);
        bytes = ch->bytes;
        LIST_REMOVE(ch, pointers);
        UA_free(ch);
    }
    return bytes;
}

static UA_StatusCode
UA_SecureChannel_processSequenceNumber(UA_SecureChannel *channel, UA_UInt32 SequenceNumber) {
    /* Does the sequence number match? */
    if(SequenceNumber != channel->receiveSequenceNumber + 1) {
        if(channel->receiveSequenceNumber + 1 > 4294966271 && SequenceNumber < 1024)
            channel->receiveSequenceNumber = SequenceNumber - 1; /* Roll over */
        else
            return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
    ++channel->receiveSequenceNumber;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SecureChannel_processChunks(UA_SecureChannel *channel, const UA_ByteString *chunks,
                               UA_ProcessMessageCallback callback, void *application) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    size_t offset= 0;
    do {
        /* Store the initial offset to compute the header length */
        size_t initial_offset = offset;

        /* Decode header */
        UA_SecureConversationMessageHeader header;
        retval = UA_SecureConversationMessageHeader_decodeBinary(chunks, &offset, &header);
        if(retval != UA_STATUSCODE_GOOD)
            break;

        /* Is the channel attached to connection? */
        if(header.secureChannelId != channel->securityToken.channelId) {
            //Service_CloseSecureChannel(server, channel);
            //connection->close(connection);
            return UA_STATUSCODE_BADCOMMUNICATIONERROR;
        }

        /* Use requestId = 0 with OPN as argument for the callback */
        UA_SequenceHeader sequenceHeader;
        UA_SequenceHeader_init(&sequenceHeader);

        if((header.messageHeader.messageTypeAndChunkType & 0x00ffffff) != UA_MESSAGETYPE_OPN) {
            /* Check the symmetric security header (not for OPN) */
            UA_UInt32 tokenId = 0;
            retval |= UA_UInt32_decodeBinary(chunks, &offset, &tokenId);
            retval |= UA_SequenceHeader_decodeBinary(chunks, &offset, &sequenceHeader);
            if(retval != UA_STATUSCODE_GOOD)
                return UA_STATUSCODE_BADCOMMUNICATIONERROR;

            /* Does the token match? */
            if(tokenId != channel->securityToken.tokenId) {
                if(tokenId != channel->nextSecurityToken.tokenId)
                    return UA_STATUSCODE_BADCOMMUNICATIONERROR;
                UA_SecureChannel_revolveTokens(channel);
            }

            /* Does the sequence number match? */
            retval = UA_SecureChannel_processSequenceNumber(channel, sequenceHeader.sequenceNumber);
            if(retval != UA_STATUSCODE_GOOD)
                return UA_STATUSCODE_BADCOMMUNICATIONERROR;
        }

        /* Process chunk */
        size_t processed_header = offset - initial_offset;
        switch(header.messageHeader.messageTypeAndChunkType & 0xff000000) {
        case UA_CHUNKTYPE_INTERMEDIATE:
            UA_SecureChannel_appendChunk(channel, sequenceHeader.requestId, chunks, offset,
                                         header.messageHeader.messageSize - processed_header);
            break;
        case UA_CHUNKTYPE_FINAL: {
            UA_Boolean realloced = false;
            UA_ByteString message =
                UA_SecureChannel_finalizeChunk(channel, sequenceHeader.requestId, chunks, offset,
                                               header.messageHeader.messageSize - processed_header,
                                               &realloced);
            if(message.length > 0) {
                callback(application, channel, header.messageHeader.messageTypeAndChunkType & 0x00ffffff,
                         sequenceHeader.requestId, &message);
                if(realloced)
                    UA_ByteString_deleteMembers(&message);
            }
            break; }
        case UA_CHUNKTYPE_ABORT:
            UA_SecureChannel_removeChunk(channel, sequenceHeader.requestId);
            break;
        default:
            return UA_STATUSCODE_BADDECODINGERROR;
        }

        /* Jump to the end of the chunk */
        offset += (header.messageHeader.messageSize - processed_header);
    } while(chunks->length > offset);

    return retval;
}
