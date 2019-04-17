/*
 *  mms_named_variable_list_service.c
 *
 *  Copyright 2013-2015 Michael Zillgith
 *
 *	This file is part of libIEC61850.
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
 *  along with libIEC61850. If not, see <http://www.gnu.org/licenses/>.
 *
 *  See COPYING file for the complete license text.
 */

#include "libiec61850_platform_includes.h"
#include "mms_server_internal.h"
#include "mms_named_variable_list.h"

#include "ber_encoder.h"

#if (MMS_DATA_SET_SERVICE == 1)

#if (MMS_DYNAMIC_DATA_SETS == 1)

#ifndef CONFIG_MMS_MAX_NUMBER_OF_DOMAIN_SPECIFIC_DATA_SETS
#define CONFIG_MMS_MAX_NUMBER_OF_DOMAIN_SPECIFIC_DATA_SETS 10
#endif

#ifndef CONFIG_MMS_MAX_NUMBER_OF_ASSOCIATION_SPECIFIC_DATA_SETS
#define CONFIG_MMS_MAX_NUMBER_OF_ASSOCIATION_SPECIFIC_DATA_SETS 10
#endif

#ifndef CONFIG_MMS_MAX_NUMBER_OF_VMD_SPECIFIC_DATA_SETS
#define CONFIG_MMS_MAX_NUMBER_OF_VMD_SPECIFIC_DATA_SETS 10
#endif

#ifndef CONFIG_MMS_MAX_NUMBER_OF_DATA_SET_MEMBERS
#define CONFIG_MMS_MAX_NUMBER_OF_DATA_SET_MEMBERS 50
#endif

MmsError
mmsServer_callVariableListChangedHandler(bool create, MmsVariableListType listType, MmsDomain* domain,
        char* listName, MmsServerConnection connection)
{
    MmsServer self = connection->server;

    if (self->variableListChangedHandler != NULL) {
        if (DEBUG_MMS_SERVER)
            printf("MMS_SERVER: call MmsNamedVariableListChangedHandler for new list %s\n", listName);

        return self->variableListChangedHandler(self->variableListChangedHandlerParameter,
                create, listType, domain, listName, connection);
    }
    else
        return MMS_ERROR_NONE;
}

static void
createDeleteNamedVariableListResponse(uint32_t invokeId, ByteBuffer* response,
        uint32_t numberMatched, uint32_t numberDeleted)
{
    uint32_t invokeIdSize = BerEncoder_UInt32determineEncodedSize(invokeId) + 2;

    uint32_t numberMatchedSize =
            2 + BerEncoder_UInt32determineEncodedSize(numberMatched);

    uint32_t numberDeletedSize =
            2 + BerEncoder_UInt32determineEncodedSize(numberDeleted);

    uint32_t deleteNVLSize = 2 + numberMatchedSize + numberDeletedSize;

    uint32_t confirmedResponsePDUSize = invokeIdSize + deleteNVLSize;

    int bufPos = 0;
    uint8_t* buffer = response->buffer;

    bufPos = BerEncoder_encodeTL(0xa1, confirmedResponsePDUSize, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0x02, invokeIdSize - 2, buffer, bufPos);
    bufPos = BerEncoder_encodeUInt32(invokeId, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0xad, numberMatchedSize + numberDeletedSize, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0x80, numberMatchedSize - 2, buffer, bufPos);
    bufPos = BerEncoder_encodeUInt32(numberMatched, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0x81, numberDeletedSize - 2, buffer, bufPos);
    bufPos = BerEncoder_encodeUInt32(numberDeleted, buffer, bufPos);

    response->size = bufPos;
}

static void /* Confirmed service error (ServiceError) */
createServiceErrorDeleteVariableLists(uint32_t invokeId, ByteBuffer* response,
        MmsError errorType, uint32_t numberDeleted)
{
    uint8_t buffer[8];

    int size = BerEncoder_encodeUInt32WithTL(0x86, numberDeleted, buffer, 0);

    mmsServer_createServiceErrorPduWithServiceSpecificInfo(invokeId, response, errorType,
            buffer, size);
}

void
mmsServer_handleDeleteNamedVariableListRequest(MmsServerConnection connection,
		uint8_t* buffer, int bufPos, int maxBufPos,
		uint32_t invokeId,
		ByteBuffer* response)
{
	DeleteNamedVariableListRequest_t* request = 0;

    MmsPdu_t* mmsPdu = 0;
	long scopeOfDelete;
	MmsDevice* device;

    asn_dec_rval_t rval = ber_decode(NULL, &asn_DEF_MmsPdu, (void**) &mmsPdu, buffer, maxBufPos);

    if (rval.code != RC_OK) {
        mmsMsg_createMmsRejectPdu(&invokeId, MMS_ERROR_REJECT_INVALID_PDU, response);
        goto exit_function;
    }

    request = &(mmsPdu->choice.confirmedRequestPdu.confirmedServiceRequest.choice.deleteNamedVariableList);

	scopeOfDelete = DeleteNamedVariableListRequest__scopeOfDelete_specific;

	if (request->scopeOfDelete)
	    asn_INTEGER2long(request->scopeOfDelete, &scopeOfDelete);

	device = MmsServer_getDevice(connection->server);

	if (scopeOfDelete == DeleteNamedVariableListRequest__scopeOfDelete_specific) {
	    MmsError serviceError = MMS_ERROR_NONE;

		int numberMatched = 0;
		int numberDeleted = 0;

		int numberItems = request->listOfVariableListName->list.count;

		int i;
		MmsDomain* domain;
		MmsNamedVariableList variableList;
		MmsError deleteError;

		for (i = 0; i < numberItems; i++) {
			if (request->listOfVariableListName->list.array[i]->present == ObjectName_PR_domainspecific) {
		        char domainName[65];
		        char listName[65];

		        mmsMsg_copyAsn1IdentifierToStringBuffer(request->listOfVariableListName->list.array[i]->choice.domainspecific.domainId,
		                domainName, 65);

		        mmsMsg_copyAsn1IdentifierToStringBuffer(request->listOfVariableListName->list.array[i]->choice.domainspecific.itemId,
		                listName, 65);

		        domain = MmsDevice_getDomain(device, domainName);

		        if (domain != NULL) {

                    MmsNamedVariableList variableList = MmsDomain_getNamedVariableList(domain, listName);

                    if (variableList != NULL) {
                        numberMatched++;

                        if (MmsNamedVariableList_isDeletable(variableList)) {

                            MmsError deleteError = mmsServer_callVariableListChangedHandler(false, MMS_DOMAIN_SPECIFIC, domain, listName, connection);

                            if (deleteError == MMS_ERROR_NONE) {
                                MmsDomain_deleteNamedVariableList(domain, listName);
                                numberDeleted++;
                            }
                            else
                                serviceError = deleteError;
                        }
                    }
		        }
			}
			else if (request->listOfVariableListName->list.array[i]->present == ObjectName_PR_aaspecific) {
			    char listName[65];

			    mmsMsg_copyAsn1IdentifierToStringBuffer(request->listOfVariableListName->list.array[i]->choice.aaspecific,
			            listName, 65);

				variableList = MmsServerConnection_getNamedVariableList(connection, listName);

				if (variableList != NULL) {
					numberMatched++;

					deleteError = mmsServer_callVariableListChangedHandler(false, MMS_ASSOCIATION_SPECIFIC, NULL, listName, connection);

					if (deleteError == MMS_ERROR_NONE) {
					    numberDeleted++;
					    MmsServerConnection_deleteNamedVariableList(connection, listName);
					}
					else
					    serviceError = deleteError;
				}
			}
			else if (request->listOfVariableListName->list.array[i]->present == ObjectName_PR_vmdspecific) {
			    char listName[65];

                mmsMsg_copyAsn1IdentifierToStringBuffer(request->listOfVariableListName->list.array[i]->choice.vmdspecific,
                        listName, 65);

                variableList = mmsServer_getNamedVariableListWithName(device->namedVariableLists, listName);

                if (variableList != NULL) {
                    numberMatched++;

                    deleteError = mmsServer_callVariableListChangedHandler(false, MMS_VMD_SPECIFIC, NULL, listName, connection);

                    if (deleteError == MMS_ERROR_NONE) {
                        numberDeleted++;
                        mmsServer_deleteVariableList(device->namedVariableLists, listName);
                    }
                    else
                        serviceError = deleteError;
                }
			}
		}

		if (serviceError ==  MMS_ERROR_NONE)
		    createDeleteNamedVariableListResponse(invokeId, response, numberMatched, numberDeleted);
		else
		    createServiceErrorDeleteVariableLists(invokeId, response, serviceError, numberDeleted);
	}
	else {
		mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_ACCESS_UNSUPPORTED);
	}

exit_function:

    asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);

    return;
}

static void
createDefineNamedVariableListResponse(uint32_t invokeId, ByteBuffer* response)
{
    uint32_t invokeIdSize = BerEncoder_UInt32determineEncodedSize((uint32_t) invokeId) + 2;

    uint32_t confirmedResponsePDUSize = 2 + invokeIdSize;

    int bufPos = 0;
    uint8_t* buffer = response->buffer;

    bufPos = BerEncoder_encodeTL(0xa1, confirmedResponsePDUSize, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0x02, invokeIdSize - 2, buffer, bufPos);
    bufPos = BerEncoder_encodeUInt32(invokeId, buffer, bufPos);

    bufPos = BerEncoder_encodeTL(0x8b, 0, buffer, bufPos);

    response->size = bufPos;
}

static bool
checkIfVariableExists(MmsDevice* device, MmsAccessSpecifier* accessSpecifier)
{
	MmsVariableSpecification* variableSpec;

    if (accessSpecifier->domain == NULL)
        return false;

    variableSpec =
            MmsDomain_getNamedVariable(accessSpecifier->domain, accessSpecifier->variableName);

    if (variableSpec == NULL)
        return false;

    if (accessSpecifier->arrayIndex != -1) {
        if (variableSpec->type != MMS_ARRAY)
            return false;

        if (accessSpecifier->arrayIndex >= variableSpec->typeSpec.array.elementCount)
            return false;

        if (accessSpecifier->componentName != NULL) {
            if (MmsVariableSpecification_getNamedVariableRecursive(variableSpec, accessSpecifier->componentName) == NULL)
                return false;
        }
    }

    return true;
}


static MmsNamedVariableList
createNamedVariableList(MmsServer server, MmsDomain* domain, MmsDevice* device,
		DefineNamedVariableListRequest_t* request,
		char* variableListName, MmsError* mmsError)
{
    MmsNamedVariableList namedVariableList = NULL;

	int variableCount = request->listOfVariable.list.count;
	int i;

#if (CONFIG_MMS_SERVER_CONFIG_SERVICES_AT_RUNTIME == 1)
	if ((variableCount == 0 ) || (variableCount > server->maxDataSetEntries)) {
#else
	if ((variableCount == 0 ) || (variableCount > CONFIG_MMS_MAX_NUMBER_OF_DATA_SET_MEMBERS)) {
#endif
	    *mmsError = MMS_ERROR_DEFINITION_OTHER;
	    goto exit_function;
	}

	namedVariableList = MmsNamedVariableList_create(domain, variableListName, true);

	for (i = 0; i < variableCount; i++) {
		VariableSpecification_t* varSpec =
				&request->listOfVariable.list.array[i]->variableSpecification;

		long arrayIndex = -1;

		char componentNameBuf[65];
		char* componentName = NULL;

		/* Handle alternate access specification - for array element definition */
		if (request->listOfVariable.list.array[i]->alternateAccess != NULL) {

			if (request->listOfVariable.list.array[i]->alternateAccess->list.count != 1) {
				MmsNamedVariableList_destroy(namedVariableList);
				namedVariableList = NULL;
				break;
			}
			else {

				struct AlternateAccess__Member* alternateAccess =
						request->listOfVariable.list.array[i]->alternateAccess->list.array[0];

				Identifier_t componentIdentifier;

				if ((alternateAccess->present == AlternateAccess__Member_PR_unnamed)
				    &&(alternateAccess->choice.unnamed->present == AlternateAccessSelection_PR_selectAlternateAccess)
				    && (alternateAccess->choice.unnamed->choice.selectAlternateAccess.accessSelection.present ==
				               AlternateAccessSelection__selectAlternateAccess__accessSelection_PR_index))
				{
					asn_INTEGER2long(&(alternateAccess->choice.unnamed->choice.selectAlternateAccess.accessSelection.choice.index),
							&arrayIndex);

					componentIdentifier = alternateAccess->choice.unnamed->
                            choice.selectAlternateAccess.alternateAccess->list.array[0]->
                            choice.unnamed->choice.selectAccess.choice.component;

					componentName =
					        StringUtils_createStringFromBufferInBuffer(componentNameBuf,
					                componentIdentifier.buf, componentIdentifier.size);

				}
				else {
					MmsNamedVariableList_destroy(namedVariableList);
					namedVariableList = NULL;
					*mmsError = MMS_ERROR_DEFINITION_INVALID_ADDRESS;
					break;
				}

			}

		}

		if (varSpec->present == VariableSpecification_PR_name) {

		    char variableName[65];
		    char domainId[65];
			MmsDomain* elementDomain;
			MmsAccessSpecifier accessSpecifier;

		    StringUtils_createStringFromBufferInBuffer(variableName,
		            varSpec->choice.name.choice.domainspecific.itemId.buf,
                    varSpec->choice.name.choice.domainspecific.itemId.size);

		    StringUtils_createStringFromBufferInBuffer(domainId,
		            varSpec->choice.name.choice.domainspecific.domainId.buf,
                    varSpec->choice.name.choice.domainspecific.domainId.size);

			elementDomain = MmsDevice_getDomain(device, domainId);
			
			accessSpecifier.domain = elementDomain;
			accessSpecifier.variableName = variableName;
			accessSpecifier.arrayIndex = arrayIndex;
			accessSpecifier.componentName = componentName;

			// check if element exists
			if (checkIfVariableExists(device, &accessSpecifier) == true) {

                MmsNamedVariableListEntry variable =
                        MmsNamedVariableListEntry_create(accessSpecifier);

                MmsNamedVariableList_addVariable(namedVariableList, variable);
			}
			else {
			    MmsNamedVariableList_destroy(namedVariableList);
                namedVariableList = NULL;
                i = variableCount; // exit loop after freeing loop variables
                *mmsError = MMS_ERROR_DEFINITION_OBJECT_UNDEFINED;
			}
		}
		else {
			MmsNamedVariableList_destroy(namedVariableList);
			namedVariableList = NULL;
			*mmsError = MMS_ERROR_DEFINITION_INVALID_ADDRESS;
			break;
		}
	}

exit_function:

	return namedVariableList;
}

void
mmsServer_handleDefineNamedVariableListRequest(
		MmsServerConnection connection,
		uint8_t* buffer, int bufPos, int maxBufPos,
		uint32_t invokeId,
		ByteBuffer* response)
{
	DefineNamedVariableListRequest_t* request = 0;

	MmsPdu_t* mmsPdu = 0;

	MmsDevice* device;
	MmsDomain* domain;

	asn_dec_rval_t rval = ber_decode(NULL, &asn_DEF_MmsPdu, (void**) &mmsPdu, buffer, maxBufPos);

	if (rval.code != RC_OK) {
	    mmsMsg_createMmsRejectPdu(&invokeId, MMS_ERROR_REJECT_INVALID_PDU, response);
	    goto exit_free_struct;
	}

	request = &(mmsPdu->choice.confirmedRequestPdu.confirmedServiceRequest.choice.defineNamedVariableList);

	device = MmsServer_getDevice(connection->server);

	if (request->variableListName.present == ObjectName_PR_domainspecific) {

	    char domainName[65];

	    if (request->variableListName.choice.domainspecific.domainId.size > 64) {
	        mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_NON_EXISTENT);
	        goto exit_free_struct;
	    }

	    StringUtils_createStringFromBufferInBuffer(domainName,
	            request->variableListName.choice.domainspecific.domainId.buf,
	            request->variableListName.choice.domainspecific.domainId.size);

		domain = MmsDevice_getDomain(device, domainName);

		if (domain == NULL) {
			mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_NON_EXISTENT);
			goto exit_free_struct;
		}

#if (CONFIG_MMS_SERVER_CONFIG_SERVICES_AT_RUNTIME == 1)
		if (LinkedList_size(domain->namedVariableLists) < connection->server->maxDomainSpecificDataSets) {
#else
		if (LinkedList_size(domain->namedVariableLists) < CONFIG_MMS_MAX_NUMBER_OF_DOMAIN_SPECIFIC_DATA_SETS) {
#endif
		    char variableListName[65];

		    if (request->variableListName.choice.domainspecific.itemId.size > 64) {
		        mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_NON_EXISTENT);
                goto exit_free_struct;
		    }

		    StringUtils_createStringFromBufferInBuffer(variableListName,
		            request->variableListName.choice.domainspecific.itemId.buf,
                    request->variableListName.choice.domainspecific.itemId.size);

            if (MmsDomain_getNamedVariableList(domain, variableListName) != NULL) {
                mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_DEFINITION_OBJECT_EXISTS);
            }
            else {
                MmsError mmsError;

                MmsNamedVariableList namedVariableList = createNamedVariableList(connection->server, domain, device,
                                request, variableListName, &mmsError);

                if (namedVariableList != NULL) {

                    mmsError = mmsServer_callVariableListChangedHandler(true, MMS_DOMAIN_SPECIFIC, domain, variableListName, connection);

                    if (mmsError == MMS_ERROR_NONE) {
                        MmsDomain_addNamedVariableList(domain, namedVariableList);
                        createDefineNamedVariableListResponse(invokeId, response);
                    }
                    else {
                        MmsNamedVariableList_destroy(namedVariableList);
                        mmsMsg_createServiceErrorPdu(invokeId, response, mmsError);
                    }
                }
                else
                    mmsMsg_createServiceErrorPdu(invokeId, response, mmsError);
            }
		}
		else
		    mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_RESOURCE_CAPABILITY_UNAVAILABLE);


	}
	else if (request->variableListName.present == ObjectName_PR_aaspecific) {

#if (CONFIG_MMS_SERVER_CONFIG_SERVICES_AT_RUNTIME == 1)
	    if (LinkedList_size(connection->namedVariableLists) < connection->server->maxAssociationSpecificDataSets) {
#else
	    if (LinkedList_size(connection->namedVariableLists) < CONFIG_MMS_MAX_NUMBER_OF_ASSOCIATION_SPECIFIC_DATA_SETS) {
#endif

	        char variableListName[65];

	        if (request->variableListName.choice.aaspecific.size > 64) {
                mmsMsg_createMmsRejectPdu(&invokeId, MMS_ERROR_REJECT_REQUEST_INVALID_ARGUMENT, response);
                goto exit_free_struct;
            }

	        StringUtils_createStringFromBufferInBuffer(variableListName,
	                request->variableListName.choice.aaspecific.buf,
	                request->variableListName.choice.aaspecific.size);

            if (MmsServerConnection_getNamedVariableList(connection, variableListName) != NULL) {
                mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_DEFINITION_OBJECT_EXISTS);
            }
            else {
                MmsError mmsError;

                MmsNamedVariableList namedVariableList = createNamedVariableList(connection->server, NULL, device,
                        request, variableListName, &mmsError);

                if (namedVariableList != NULL) {

                    if (mmsServer_callVariableListChangedHandler(true, MMS_ASSOCIATION_SPECIFIC, NULL, variableListName, connection) == MMS_ERROR_NONE) {
                        MmsServerConnection_addNamedVariableList(connection, namedVariableList);
                        createDefineNamedVariableListResponse(invokeId, response);
                    }
                    else {
                        MmsNamedVariableList_destroy(namedVariableList);
                        mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_ACCESS_DENIED);
                    }

                }
                else
                    mmsMsg_createServiceErrorPdu(invokeId, response, mmsError);
            }
	    }
	    else
	        mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_RESOURCE_CAPABILITY_UNAVAILABLE);
	}
	else if (request->variableListName.present == ObjectName_PR_vmdspecific) {
	    LinkedList vmdScopeNVLs = MmsDevice_getNamedVariableLists(connection->server->device);

	    if (LinkedList_size(vmdScopeNVLs) < CONFIG_MMS_MAX_NUMBER_OF_VMD_SPECIFIC_DATA_SETS) {

	        char variableListName[65];

	        if (request->variableListName.choice.vmdspecific.size > 64) {
	            mmsMsg_createMmsRejectPdu(&invokeId, MMS_ERROR_REJECT_REQUEST_INVALID_ARGUMENT, response);
                goto exit_free_struct;
	        }

	        StringUtils_createStringFromBufferInBuffer(variableListName,
                    request->variableListName.choice.vmdspecific.buf,
                    request->variableListName.choice.vmdspecific.size);

	        if (mmsServer_getNamedVariableListWithName(MmsDevice_getNamedVariableLists(connection->server->device), variableListName) != NULL) {
	            mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_DEFINITION_OBJECT_EXISTS);
	        }
	        else {
	            MmsError mmsError;

                MmsNamedVariableList namedVariableList = createNamedVariableList(connection->server, NULL, device,
                        request, variableListName, &mmsError);

                if (namedVariableList != NULL) {
                    if (mmsServer_callVariableListChangedHandler(true, MMS_VMD_SPECIFIC, NULL, variableListName, connection)
                            == MMS_ERROR_NONE) {
                        LinkedList_add(vmdScopeNVLs, (void*) namedVariableList);

                        createDefineNamedVariableListResponse(invokeId, response);
                    }
                    else {
                        MmsNamedVariableList_destroy(namedVariableList);
                        mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_ACCESS_DENIED);
                    }

                }
	        }
	    }
	}
	else
		mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_DEFINITION_TYPE_UNSUPPORTED);

exit_free_struct:
	asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);

    return;
}

#endif /* (MMS_DYNAMIC_DATA_SETS == 1) */

#if (MMS_GET_DATA_SET_ATTRIBUTES == 1)

static void
createGetNamedVariableListAttributesResponse(int invokeId, ByteBuffer* response,
		MmsNamedVariableList variableList)
{
	MmsPdu_t* mmsPdu = mmsServer_createConfirmedResponse(invokeId);

	GetNamedVariableListAttributesResponse_t* varListResponse;
	LinkedList variables;
	int variableCount;
	LinkedList variable;
	int i;
	char* variableDomainName;

	mmsPdu->choice.confirmedResponsePdu.confirmedServiceResponse.present =
			ConfirmedServiceResponse_PR_getNamedVariableListAttributes;

	varListResponse =
		&(mmsPdu->choice.confirmedResponsePdu.confirmedServiceResponse.
				choice.getNamedVariableListAttributes);

	varListResponse->mmsDeletable = MmsNamedVariableList_isDeletable(variableList);

	variables = MmsNamedVariableList_getVariableList(variableList);

	variableCount = LinkedList_size(variables);

	varListResponse->listOfVariable.list.count = variableCount;
	varListResponse->listOfVariable.list.size = variableCount;

	varListResponse->listOfVariable.list.array = (struct GetNamedVariableListAttributesResponse__listOfVariable__Member**) 
        GLOBAL_CALLOC(variableCount, sizeof(void*));

	variable = LinkedList_getNext(variables);
	
	for (i = 0; i < variableCount; i++) {
		MmsNamedVariableListEntry variableEntry = (MmsNamedVariableListEntry) variable->data;

		varListResponse->listOfVariable.list.array[i] =  (struct GetNamedVariableListAttributesResponse__listOfVariable__Member*) 
                GLOBAL_CALLOC(1, sizeof(struct GetNamedVariableListAttributesResponse__listOfVariable__Member));

		varListResponse->listOfVariable.list.array[i]->variableSpecification.present =
				VariableSpecification_PR_name;

		varListResponse->listOfVariable.list.array[i]->variableSpecification.choice.name.present =
				ObjectName_PR_domainspecific;

		variableDomainName = MmsDomain_getName(variableEntry->domain);

		varListResponse->listOfVariable.list.array[i]->variableSpecification.choice.name.choice.
			domainspecific.domainId.buf = (uint8_t*) StringUtils_copyString(variableDomainName);

		varListResponse->listOfVariable.list.array[i]->variableSpecification.choice.name.choice.
			domainspecific.domainId.size = strlen(variableDomainName);

		varListResponse->listOfVariable.list.array[i]->variableSpecification.choice.name.choice.
			domainspecific.itemId.buf = (uint8_t*) StringUtils_copyString(variableEntry->variableName);

		varListResponse->listOfVariable.list.array[i]->variableSpecification.choice.name.choice.
			domainspecific.itemId.size = strlen(variableEntry->variableName);

		variable = LinkedList_getNext(variable);
	}

	der_encode(&asn_DEF_MmsPdu, mmsPdu,	mmsServer_write_out, (void*) response);

	asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);
}

void
mmsServer_handleGetNamedVariableListAttributesRequest(
		MmsServerConnection connection,
		uint8_t* buffer, int bufPos, int maxBufPos,
		uint32_t invokeId,
		ByteBuffer* response)
{
	GetNamedVariableListAttributesRequest_t* request = 0;

	asn_dec_rval_t rval = ber_decode(NULL, &asn_DEF_GetNamedVariableListAttributesRequest,
				(void**) &request, buffer + bufPos, maxBufPos - bufPos);

	MmsDevice* mmsDevice;
	MmsDomain* domain;

	if (rval.code != RC_OK) {
	    mmsMsg_createMmsRejectPdu(&invokeId, MMS_ERROR_REJECT_INVALID_PDU, response);
	    goto exit_function;
	}

	if (request->present == ObjectName_PR_domainspecific) {

	    char domainName[65];
	    char itemName[65];

	    if ((request->choice.domainspecific.domainId.size > 64) ||
	        (request->choice.domainspecific.itemId.size > 64)) {
	        mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OTHER);
	        goto exit_function;
	    }

	    StringUtils_createStringFromBufferInBuffer(domainName, request->choice.domainspecific.domainId.buf,
	            request->choice.domainspecific.domainId.size);

	    StringUtils_createStringFromBufferInBuffer(itemName, request->choice.domainspecific.itemId.buf,
                request->choice.domainspecific.itemId.size);

		mmsDevice = MmsServer_getDevice(connection->server);

		domain = MmsDevice_getDomain(mmsDevice, domainName);

		if (domain != NULL) {
			MmsNamedVariableList variableList =
					MmsDomain_getNamedVariableList(domain, itemName);

			if (variableList != NULL)
				createGetNamedVariableListAttributesResponse(invokeId, response, variableList);
			else
				mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_NON_EXISTENT);
		}
		else
			mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_NON_EXISTENT);

	}
#if (MMS_DYNAMIC_DATA_SETS == 1)
	else if (request->present == ObjectName_PR_aaspecific) {

	    char listName[65];
		MmsNamedVariableList varList;

        if (request->choice.aaspecific.size > 64) {
            mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OTHER);
            goto exit_function;
        }

	    StringUtils_createStringFromBufferInBuffer(listName, request->choice.aaspecific.buf,
	            request->choice.aaspecific.size);

	    varList = MmsServerConnection_getNamedVariableList(connection, listName);

	    if (varList != NULL)
	        createGetNamedVariableListAttributesResponse(invokeId, response, varList);
	    else
	        mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_NON_EXISTENT);
	}
#endif /* (MMS_DYNAMIC_DATA_SETS == 1) */
	else if (request->present == ObjectName_PR_vmdspecific) {
	    char listName[65];
		MmsDevice* mmsDevice;
		MmsNamedVariableList varList;

	    if (request->choice.vmdspecific.size > 64) {
	        mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OTHER);
            goto exit_function;
	    }

	    StringUtils_createStringFromBufferInBuffer(listName, request->choice.vmdspecific.buf,
                request->choice.vmdspecific.size);

	    mmsDevice = MmsServer_getDevice(connection->server);

	    varList = mmsServer_getNamedVariableListWithName(mmsDevice->namedVariableLists, listName);

        if (varList != NULL)
            createGetNamedVariableListAttributesResponse(invokeId, response, varList);
        else
            mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_NON_EXISTENT);
	}
	else {
		mmsMsg_createServiceErrorPdu(invokeId, response, MMS_ERROR_ACCESS_OBJECT_ACCESS_UNSUPPORTED);
	}

exit_function:

	asn_DEF_GetVariableAccessAttributesRequest.free_struct(&asn_DEF_GetNamedVariableListAttributesRequest,
			request, 0);
}

#endif /* (MMS_GET_DATA_SET_ATTRIBUTES == 1) */

#endif /* (MMS_DATA_SET_SERVICE == 1) */
