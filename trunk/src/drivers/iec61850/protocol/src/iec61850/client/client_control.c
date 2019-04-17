/*
 *  client_control.c
 *
 *  Copyright 2013, 2014 Michael Zillgith
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

#include "iec61850_client.h"
#include "mms_client_connection.h"
#include "ied_connection_private.h"

#if _MSC_VER
#define snprintf _snprintf
#endif

#ifndef DEBUG_IED_CLIENT
#define DEBUG_IED_CLIENT 0
#endif

struct sControlObjectClient
{
    ControlModel ctlModel;
    char* objectReference;
    IedConnection connection;
    bool test;
    bool interlockCheck;
    bool synchroCheck;
    bool hasTimeActivatedMode;

    MmsValue* analogValue; /* for APC-CDCs */

    int edition; /* 1 = Ed. 1 - 2 = Ed. 2 - to distinguish time stamp format */
    bool hasCtlNum; /* Check if ctlNum attribute is present - ctlNum is(/can be?) absent in edition 1 APC CDC */

    bool useConstantT; /* some servers require a constant T parameter for select and operate */
    uint64_t constantT; /* timestamp of select/operate to be used when constant T option is selected */

    LastApplError lastApplError;

    CommandTerminationHandler commandTerminationHandler;
    void* commandTerminaionHandlerParameter;

    /* control operation parameters */
    MmsValue* ctlVal;
    uint64_t opertime;
    uint8_t ctlNum;
    char* orIdent;
    int orCat;
};

static void
convertToMmsAndInsertFC(char* newItemId, const char* originalObjectName, const char* fc)
{
    int originalLength = strlen(originalObjectName);

    int srcIndex = 0;
    int dstIndex = 0;

    while (originalObjectName[srcIndex] != '.') {
        newItemId[dstIndex] = originalObjectName[srcIndex];
        srcIndex++;
        dstIndex++;
    }

    newItemId[dstIndex++] = '$';
    newItemId[dstIndex++] = fc[0];
    newItemId[dstIndex++] = fc[1];
    newItemId[dstIndex++] = '$';
    srcIndex++;

    while (srcIndex < originalLength) {
        if (originalObjectName[srcIndex] == '.')
            newItemId[dstIndex] = '$';
        else
            newItemId[dstIndex] = originalObjectName[srcIndex];

        dstIndex++;
        srcIndex++;
    }

    newItemId[dstIndex] = 0;
}

static void
resetLastApplError(ControlObjectClient self)
{
    self->lastApplError.error = 0;
    self->lastApplError.addCause = ADD_CAUSE_UNKNOWN;
    self->lastApplError.ctlNum = 0;
}

ControlObjectClient
ControlObjectClient_create(const char* objectReference, IedConnection connection)
{
    ControlObjectClient self = NULL;

    /* request control model from server */
    char reference[129];

	IedClientError error;

	uint32_t ctlModel;
	MmsVariableSpecification* ctlVarSpec;
	bool hasOper;
    bool hasTimeActivatedControl;
    bool hasCtlNum;
    bool isAPC;
    MmsVariableSpecification* ctlVal;
    MmsVariableSpecification* t;
	MmsVariableSpecification* operTm;
	MmsVariableSpecification* ctlNum;

    if (strlen(objectReference) < 120) {
        strcpy(reference, objectReference);
        strcat(reference, ".ctlModel");
    }
    else
        goto exit_function;

    
    ctlModel = IedConnection_readUnsigned32Value(connection, &error, reference, IEC61850_FC_CF);

    if (error != IED_ERROR_OK) {
        if (DEBUG_IED_CLIENT)
            printf("IED_CLIENT: ControlObjectClient_create: failed to get %s from server\n", reference);

        goto exit_function;
    }

    ctlVarSpec =
            IedConnection_getVariableSpecification(connection, &error, objectReference, IEC61850_FC_CO);

    if (error != IED_ERROR_OK) {
        if (DEBUG_IED_CLIENT)
            printf("IED_CLIENT: ControlObjectClient_create: failed to get data directory of control object\n");

        goto exit_function;
    }

    /* check what control elements are available */
    hasOper = false;
    hasTimeActivatedControl = false;
    hasCtlNum = false;
    isAPC = false;
    ctlVal = NULL;
    t = NULL;

    if (MmsVariableSpecification_getType(ctlVarSpec) == MMS_STRUCTURE) {
        MmsVariableSpecification* oper = MmsVariableSpecification_getNamedVariableRecursive(ctlVarSpec, "Oper");

        if (oper)
        {
            hasOper = true;

            ctlVal = MmsVariableSpecification_getNamedVariableRecursive(oper, "ctlVal");

            if (ctlVal == NULL)
                ctlVal = MmsVariableSpecification_getNamedVariableRecursive(oper, "setMag");

            if (ctlVal) {
                if (MmsVariableSpecification_getType(ctlVal) == MMS_STRUCTURE)
                    isAPC = true;
            }

            operTm = MmsVariableSpecification_getNamedVariableRecursive(oper, "operTm");

            if (operTm)
                hasTimeActivatedControl = true;

            ctlNum = MmsVariableSpecification_getNamedVariableRecursive(oper, "ctlNum");

            if (ctlNum)
                hasCtlNum = true;

            t = MmsVariableSpecification_getNamedVariableRecursive(oper, "T");
        }
    }

    if (hasOper == false) {
        if (DEBUG_IED_CLIENT)
            printf("IED_CLIENT: control is missing required element \"Oper\"\n");

        goto free_varspec;
    }

    if ((ctlVal == NULL) || (t == NULL)) {
        if (DEBUG_IED_CLIENT)
            printf("IED_CLIENT:   \"Oper\" is missing required element\n");

        goto free_varspec;
    }

    self = (ControlObjectClient) GLOBAL_CALLOC(1, sizeof(struct sControlObjectClient));

    if (self == NULL)
        goto free_varspec;

    self->objectReference = StringUtils_copyString(objectReference);
    self->connection = connection;
    self->ctlModel = (ControlModel) ctlModel;
    self->hasTimeActivatedMode = hasTimeActivatedControl;
    self->hasCtlNum = hasCtlNum;
    self->ctlVal = MmsValue_newDefaultValue(ctlVal);

    if (isAPC)
        self->analogValue = MmsValue_createEmptyStructure(1);
    else
        self->analogValue = NULL;

    /* Check for T element type (Binary time -> Ed.1,UTC time -> Ed.2) */
    if (t) {
        if (MmsVariableSpecification_getType(t) == MMS_BINARY_TIME)
            self->edition = 1;
        else
            self->edition = 2;
    }
    else
        self->edition = 1;

    if (DEBUG_IED_CLIENT)
        printf("IED_CLIENT: Detected edition %i control\n", self->edition);

    private_IedConnection_addControlClient(connection, self);

free_varspec:
    MmsVariableSpecification_destroy(ctlVarSpec);

exit_function:
    return self;
}

void
ControlObjectClient_destroy(ControlObjectClient self)
{
    if (self != NULL)
    {
        GLOBAL_FREEMEM(self->objectReference);

        private_IedConnection_removeControlClient(self->connection, self);

        if (self->ctlVal != NULL)
            MmsValue_delete(self->ctlVal);

        if (self->analogValue != NULL)
            MmsValue_delete(self->analogValue);

        if (self->orIdent != NULL)
            GLOBAL_FREEMEM(self->orIdent);

        GLOBAL_FREEMEM(self);
    }
}

void
ControlObjectClient_setCommandTerminationHandler(ControlObjectClient self, CommandTerminationHandler handler,
        void* handlerParameter)
{
    self->commandTerminaionHandlerParameter = handlerParameter;
    self->commandTerminationHandler = handler;
}

char*
ControlObjectClient_getObjectReference(ControlObjectClient self)
{
    return self->objectReference;
}

ControlModel
ControlObjectClient_getControlModel(ControlObjectClient self)
{
    return self->ctlModel;
}

MmsType
ControlObjectClient_getCtlValType(ControlObjectClient self)
{
    if (self->analogValue != NULL)
        return MmsValue_getType(self->analogValue);
    else
        return MmsValue_getType(self->ctlVal);
}

void
ControlObjectClient_setOrigin(ControlObjectClient self, const char* orIdent, int orCat)
{
    if (self->orIdent != NULL)
        GLOBAL_FREEMEM(self->orIdent);

    if (orIdent != NULL)
        self->orIdent = StringUtils_copyString(orIdent);
    else
        self->orIdent = NULL;

    self->orCat = orCat;
}

static MmsValue*
createOriginValue(ControlObjectClient self)
{
    MmsValue* origin = MmsValue_createEmptyStructure(2);
	MmsValue* orCat;
	MmsValue* orIdent;

    if (origin == NULL)
        goto exit_function;

    orCat = MmsValue_newIntegerFromInt16(self->orCat);

    if (orCat == NULL)
        goto cleanup_on_error;

    MmsValue_setElement(origin, 0, orCat);

    if (self->orIdent != NULL) {
        int octetStringLen = strlen(self->orIdent);
        orIdent = MmsValue_newOctetString(0, octetStringLen);

        if (orIdent == NULL)
            goto cleanup_on_error;

        MmsValue_setOctetString(orIdent, (uint8_t*) self->orIdent, octetStringLen);
    }
    else
        orIdent = MmsValue_newOctetString(0, 0);

    if (orIdent == NULL)
        goto cleanup_on_error;

    MmsValue_setElement(origin, 1, orIdent);

    goto exit_function;

    cleanup_on_error:
    MmsValue_delete(origin);
    origin = NULL;

    exit_function:
    return origin;
}

bool
ControlObjectClient_operate(ControlObjectClient self, MmsValue* ctlVal, uint64_t operTime)
{
    bool success = false;
	MmsValue* operParameters;
	int operElementCount;
	int index;
	MmsValue* origin;
	uint64_t timestamp;
	MmsValue* ctlTime;
	MmsValue* ctlTest;
	MmsValue* check;
    char domainId[65];
    char itemId[65];
	int controlObjectItemIdLen;
	MmsError mmsError;

    if (ctlVal == NULL) {
        if (DEBUG_IED_CLIENT)
            printf("IED_CLIENT: operate - (ctlVal == NULL)!\n");

        goto exit_function;
    }

    resetLastApplError(self);

    operElementCount = 5;

    if (self->hasTimeActivatedMode)
        operElementCount++;

    if (self->hasCtlNum)
        operElementCount++;

    operParameters = MmsValue_createEmptyStructure(operElementCount);

    /* support simplified usage of APC controls - user doesn't need to create the structure */
    if (self->analogValue != NULL) {
        if (MmsValue_getType(ctlVal) != MMS_STRUCTURE) {
            MmsValue_setElement(self->analogValue, 0, ctlVal);
            ctlVal = self->analogValue;
        }
    }

    MmsValue_setElement(operParameters, 0, ctlVal);

    index = 1;

    if (self->hasTimeActivatedMode) {
        MmsValue* operTm = MmsValue_newUtcTimeByMsTime(operTime);
        MmsValue_setElement(operParameters, index++, operTm);
    }

    origin = createOriginValue(self);
    MmsValue_setElement(operParameters, index++, origin);

    if (!((self->ctlModel == CONTROL_MODEL_SBO_NORMAL) ||
            (self->ctlModel == CONTROL_MODEL_SBO_ENHANCED)))
    {
        self->ctlNum++;
    }

    if (self->hasCtlNum) {
        MmsValue* ctlNum = MmsValue_newUnsignedFromUint32(self->ctlNum);
        MmsValue_setElement(operParameters, index++, ctlNum);
    }

    if ((self->ctlModel == CONTROL_MODEL_SBO_ENHANCED) && (self->useConstantT))
        timestamp = self->constantT;
    else
        timestamp = Hal_getTimeInMs();

    if (self->useConstantT)
        self->constantT = timestamp;

    if (self->edition == 2)
        ctlTime = MmsValue_newUtcTimeByMsTime(timestamp);
    else {
        ctlTime = MmsValue_newBinaryTime(false);
        MmsValue_setBinaryTime(ctlTime, timestamp);
    }

    MmsValue_setElement(operParameters, index++, ctlTime);

    ctlTest = MmsValue_newBoolean(self->test);
    MmsValue_setElement(operParameters, index++, ctlTest);

    check = MmsValue_newBitString(2);
    MmsValue_setBitStringBit(check, 1, self->interlockCheck);
    MmsValue_setBitStringBit(check, 0, self->synchroCheck);
    MmsValue_setElement(operParameters, index++, check);

    MmsMapping_getMmsDomainFromObjectReference(self->objectReference, domainId);

    convertToMmsAndInsertFC(itemId, self->objectReference + strlen(domainId) + 1, "CO");

    controlObjectItemIdLen = strlen(itemId);

    strncat(itemId, "$Oper", 64 - controlObjectItemIdLen);

    if (DEBUG_IED_CLIENT)
        printf("IED_CLIENT: operate: %s/%s\n", domainId, itemId);

    MmsConnection_writeVariable(IedConnection_getMmsConnection(self->connection),
            &mmsError, domainId, itemId, operParameters);

    MmsValue_setElement(operParameters, 0, NULL);
    MmsValue_delete(operParameters);

    if (mmsError != MMS_ERROR_NONE) {
        if (DEBUG_IED_CLIENT)
            printf("IED_CLIENT: operate failed!\n");

        goto exit_function;
    }

    MmsValue_update(self->ctlVal, ctlVal);

    if (self->analogValue)
        MmsValue_setElement(self->analogValue, 0, NULL);

    self->opertime = operTime;

    success = true;

    exit_function:
    return success;
}

bool
ControlObjectClient_selectWithValue(ControlObjectClient self, MmsValue* ctlVal)
{
	char domainId[65];
    char itemId[65];
	MmsError mmsError;
	int selValElementCount;
	MmsValue* selValParameters;
	int index;
	MmsValue* origin;
	uint64_t timestamp;
	MmsValue* ctlTime;
	MmsValue* ctlTest;
	MmsValue* check;

    resetLastApplError(self);

    MmsMapping_getMmsDomainFromObjectReference(self->objectReference, domainId);

    convertToMmsAndInsertFC(itemId, self->objectReference + strlen(domainId) + 1, "CO");

    strncat(itemId, "$SBOw", 64);

    if (DEBUG_IED_CLIENT)
        printf("IED_CLIENT: select with value: %s/%s\n", domainId, itemId);

    selValElementCount = 5;

    if (self->hasTimeActivatedMode)
        selValElementCount++;

    if (self->hasCtlNum)
        selValElementCount++;

    selValParameters = MmsValue_createEmptyStructure(selValElementCount);

    /* support simplified usage of APC controls - user doesn't need to create the structure */
    if (self->analogValue != NULL) {
        if (MmsValue_getType(ctlVal) != MMS_STRUCTURE) {
            MmsValue_setElement(self->analogValue, 0, ctlVal);
            ctlVal = self->analogValue;
        }
    }

    MmsValue_setElement(selValParameters, 0, ctlVal);

    index = 1;

    if (self->hasTimeActivatedMode) {
        MmsValue* operTm = MmsValue_newUtcTimeByMsTime(0);
        MmsValue_setElement(selValParameters, index++, operTm);
    }

    origin = createOriginValue(self);
    MmsValue_setElement(selValParameters, index++, origin);

    self->ctlNum++;

    if (self->hasCtlNum) {
        MmsValue* ctlNum = MmsValue_newUnsignedFromUint32(self->ctlNum);
        MmsValue_setElement(selValParameters, index++, ctlNum);
    }

    timestamp = Hal_getTimeInMs();
    
    if (self->useConstantT)
        self->constantT = timestamp;

    if (self->edition == 2)
        ctlTime = MmsValue_newUtcTimeByMsTime(timestamp);
    else {
        ctlTime = MmsValue_newBinaryTime(false);
        MmsValue_setBinaryTime(ctlTime, timestamp);
    }

    MmsValue_setElement(selValParameters, index++, ctlTime);

    ctlTest = MmsValue_newBoolean(self->test);
    MmsValue_setElement(selValParameters, index++, ctlTest);

    check = MmsValue_newBitString(2);
    MmsValue_setBitStringBit(check, 1, self->interlockCheck);
    MmsValue_setBitStringBit(check, 0, self->synchroCheck);
    MmsValue_setElement(selValParameters, index++, check);

    MmsConnection_writeVariable(IedConnection_getMmsConnection(self->connection),
            &mmsError, domainId, itemId, selValParameters);

    MmsValue_setElement(selValParameters, 0, NULL);
    MmsValue_delete(selValParameters);

    if (mmsError != MMS_ERROR_NONE) {
        if (DEBUG_IED_CLIENT)
            printf("IED_CLIENT: select-with-value failed!\n");
        return false;
    }

    MmsValue_update(self->ctlVal, ctlVal);

    if (self->analogValue)
        MmsValue_setElement(self->analogValue, 0, NULL);

    return true;
}

bool
ControlObjectClient_select(ControlObjectClient self)
{
	char domainId[65];
    char itemId[65];
	MmsError mmsError;
	MmsValue* value;
	bool selected;

    resetLastApplError(self);

    MmsMapping_getMmsDomainFromObjectReference(self->objectReference, domainId);

    convertToMmsAndInsertFC(itemId, self->objectReference + strlen(domainId) + 1, "CO");

    strncat(itemId, "$SBO", 64);

    if (DEBUG_IED_CLIENT)
        printf("IED_CLIENT: select: %s/%s\n", domainId, itemId);

    value = MmsConnection_readVariable(IedConnection_getMmsConnection(self->connection),
            &mmsError, domainId, itemId);

    selected = false;

    self->ctlNum++;

    if (value == NULL) {
        if (DEBUG_IED_CLIENT)
            printf("IED_CLIENT: select: read SBO failed!\n");
        goto exit_function;
    }

    if (MmsValue_getType(value) == MMS_VISIBLE_STRING) {
        if (strcmp(MmsValue_toString(value), "") == 0) {
            if (DEBUG_IED_CLIENT)
                printf("select-response-\n");
        }
        else {
            if (DEBUG_IED_CLIENT)
                printf("select-response+: (%s)\n", MmsValue_toString(value));
            selected = true;
        }
    }
    else {
        if (DEBUG_IED_CLIENT)
            printf("IED_CLIENT: select: unexpected response from server!\n");
    }

    MmsValue_delete(value);

    exit_function:
    return selected;
}

bool
ControlObjectClient_cancel(ControlObjectClient self)
{
	MmsValue* cancelParameters;
	int index;
	MmsValue* origin;
	MmsValue* ctlNum;
	uint64_t timestamp;
	MmsValue* ctlTime;
	MmsValue* ctlTest;
	char domainId[65];
    char itemId[65];
	MmsError mmsError;

    resetLastApplError(self);

    if (self->hasTimeActivatedMode)
        cancelParameters = MmsValue_createEmptyStructure(6);
    else
        cancelParameters = MmsValue_createEmptyStructure(5);

    MmsValue_setElement(cancelParameters, 0, self->ctlVal);

    index = 1;

    if (self->hasTimeActivatedMode) {
        MmsValue* operTm = MmsValue_newUtcTimeByMsTime(self->opertime);
        MmsValue_setElement(cancelParameters, index++, operTm);
    }

    origin = createOriginValue(self);

    MmsValue_setElement(cancelParameters, index++, origin);

    ctlNum = MmsValue_newUnsignedFromUint32(self->ctlNum);
    MmsValue_setElement(cancelParameters, index++, ctlNum);

    if (self->useConstantT)
        timestamp = self->constantT;
    else
        timestamp = Hal_getTimeInMs();

    if (self->edition == 2)
        ctlTime = MmsValue_newUtcTimeByMsTime(timestamp);
    else {
        ctlTime = MmsValue_newBinaryTime(false);
        MmsValue_setBinaryTime(ctlTime, timestamp);
    }
    MmsValue_setElement(cancelParameters, index++, ctlTime);

    ctlTest = MmsValue_newBoolean(self->test);
    MmsValue_setElement(cancelParameters, index++, ctlTest);

    MmsMapping_getMmsDomainFromObjectReference(self->objectReference, domainId);

    convertToMmsAndInsertFC(itemId, self->objectReference + strlen(domainId) + 1, "CO");

    strncat(itemId, "$Cancel", 64);

    if (DEBUG_IED_CLIENT)
        printf("IED_CLIENT: cancel: %s/%s\n", domainId, itemId);

    MmsConnection_writeVariable(IedConnection_getMmsConnection(self->connection),
            &mmsError, domainId, itemId, cancelParameters);

    MmsValue_setElement(cancelParameters, 0, NULL);
    MmsValue_delete(cancelParameters);

    if (mmsError != MMS_ERROR_NONE) {
        if (DEBUG_IED_CLIENT)
            printf("IED_CLIENT: cancel failed!\n");
        return false;
    }

    return true;
}

void
ControlObjectClient_useConstantT(ControlObjectClient self, bool useConstantT)
{
    self->useConstantT = useConstantT;
}

void
ControlObjectClient_enableInterlockCheck(ControlObjectClient self)
{
    self->interlockCheck = true;
}

void
ControlObjectClient_setInterlockCheck(ControlObjectClient self, bool value)
{
    self->interlockCheck = value;
}

void
ControlObjectClient_enableSynchroCheck(ControlObjectClient self)
{
    self->synchroCheck = true;
}

void
ControlObjectClient_setSynchroCheck(ControlObjectClient self, bool value)
{
    self->synchroCheck = value;
}

void
ControlObjectClient_setTestMode(ControlObjectClient self, bool value)
{
    self->test = value;
}

void
ControlObjectClient_setLastApplError(ControlObjectClient self, LastApplError lastApplError)
{
    self->lastApplError = lastApplError;
}

LastApplError
ControlObjectClient_getLastApplError(ControlObjectClient self)
{
    return self->lastApplError;
}

void
private_ControlObjectClient_invokeCommandTerminationHandler(ControlObjectClient self)
{
    if (self->commandTerminationHandler != NULL)
        self->commandTerminationHandler(self->commandTerminaionHandlerParameter, self);
}
