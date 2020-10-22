//
// --------------------------------------------------------------------------
//  Gurux Ltd
//
//
//
// Filename:        $HeadURL$
//
// Version:         $Revision$,
//                  $Date$
//                  $Author$
//
// Copyright (c) Gurux Ltd
//
//---------------------------------------------------------------------------
//
//  DESCRIPTION
//
// This file is a part of Gurux Device Framework.
//
// Gurux Device Framework is Open Source software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2 of the License.
// Gurux Device Framework is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// This code is licensed under the GNU General Public License v2.
// Full text may be retrieved at http://www.gnu.org/licenses/gpl-2.0.txt
//---------------------------------------------------------------------------

#ifndef COSEM_H
#define COSEM_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "gxobjects.h"

#ifndef DLMS_IGNORE_MALLOC
  int cosem_createObject(
      DLMS_OBJECT_TYPE type,
      gxObject** object);

  int cosem_createObject2(
      DLMS_OBJECT_TYPE type,
      const char* ln,
      gxObject** object);
#endif //DLMS_IGNORE_MALLOC

  int cosem_setLogicalName(
      gxObject* object,
      unsigned char* value);

#ifndef DLMS_IGNORE_MALLOC
  int cosem_init(
      gxObject* object,
      DLMS_OBJECT_TYPE type,
      const char* logicalNameString);
#endif //DLMS_IGNORE_MALLOC

  int cosem_init2(
    gxObject* object,
    DLMS_OBJECT_TYPE type,
    const unsigned char* ln);

  //This initialize method will also check the size of the object type and compare it with the expected size.
  int cosem_init3(
      gxObject* object,
      const unsigned char expectedSize,
      DLMS_OBJECT_TYPE type,
      const unsigned char* ln);

  int cosem_checkStructure(gxByteBuffer* bb, uint16_t expectedItemCount);

  int cosem_getStructure(gxByteBuffer* bb, uint16_t* count);

  int cosem_checkArray(gxByteBuffer* bb, uint16_t* count);

  int cosem_getUInt8(gxByteBuffer* bb, unsigned char* value);

  int cosem_getUInt16(gxByteBuffer* bb, uint16_t* value);

  int cosem_getUInt32(gxByteBuffer* bb, uint32_t* value);

  int cosem_getInt8(gxByteBuffer* bb, signed char* value);

  int cosem_getInt16(gxByteBuffer* bb, int16_t* value);

  int cosem_getInt32(gxByteBuffer* bb, int32_t* value);

  int cosem_getOctectString(gxByteBuffer* bb, gxByteBuffer* value);


  int cosem_getString(gxByteBuffer* bb, gxByteBuffer* value);

  int cosem_getOctectString2(gxByteBuffer* bb, unsigned char* value, uint16_t capacity, uint16_t* size);

  int cosem_getOctectString3(gxByteBuffer* bb, gxByteBuffer* value, unsigned char exact);

  int cosem_getString2(gxByteBuffer* bb, char* value, uint16_t capacity);

  int cosem_getDateTimeFromOctectString(gxByteBuffer* bb, gxtime* value);

  int cosem_getDateFromOctectString(gxByteBuffer* bb, gxtime* value);

  int cosem_getTimeFromOctectString(gxByteBuffer* bb, gxtime* value);

  int cosem_getDateTime(gxByteBuffer* bb, gxtime* value);

  int cosem_getDate(gxByteBuffer* bb, gxtime* value);

  int cosem_getTime(gxByteBuffer* bb, gxtime* value);

  int cosem_getBitString(gxByteBuffer* bb, bitArray* value);

  //Get integer value from bit string.
  int cosem_getIntegerFromBitString(gxByteBuffer* bb, uint32_t* value);

  int cosem_getVariant(gxByteBuffer* bb, dlmsVARIANT* value);

  int cosem_getEnum(gxByteBuffer* bb, unsigned char* value);

  int cosem_getBoolean(gxByteBuffer* bb, unsigned char* value);

  int cosem_getUtf8String(gxByteBuffer* bb, gxByteBuffer* value);

  int cosem_getUtf8String2(gxByteBuffer* bb, char* value, uint16_t capacity, uint16_t* size);

  int cosem_setDateTimeAsOctectString(gxByteBuffer* bb, gxtime* value);

  int cosem_setDateAsOctectString(gxByteBuffer* bb, gxtime* value);

  int cosem_setBitString(gxByteBuffer* bb, uint32_t value, uint16_t count);

  int cosem_setTimeAsOctectString(gxByteBuffer* bb, gxtime* value);

  int cosem_setDateTime(gxByteBuffer* bb, gxtime* value);

  int cosem_setDate(gxByteBuffer* bb, gxtime* value);

  int cosem_setTime(gxByteBuffer* bb, gxtime* value);

  int cosem_setOctectString(gxByteBuffer* bb, gxByteBuffer* value);

  int cosem_setOctectString2(
      gxByteBuffer* bb,
      const unsigned char* value,
      uint16_t size);

  int cosem_setUInt8(gxByteBuffer* bb, unsigned char value);
  int cosem_setUInt16(gxByteBuffer* bb, uint16_t value);
  int cosem_setUInt32(gxByteBuffer* bb, uint32_t value);
  int cosem_setUInt64(gxByteBuffer* bb, uint64_t* value);

  int cosem_setInt8(gxByteBuffer* bb, char value);
  int cosem_setInt16(gxByteBuffer* bb, int16_t value);
  int cosem_setInt32(gxByteBuffer* bb, int32_t value);
  int cosem_setInt64(gxByteBuffer* bb, int64_t* value);
  int cosem_setVariant(gxByteBuffer* bb, dlmsVARIANT* value);

  int cosem_setStructure(gxByteBuffer* bb, uint16_t count);
  int cosem_setArray(gxByteBuffer* bb, uint16_t count);

#ifndef DLMS_IGNORE_PROFILE_GENERIC
  int cosem_getColumns(
      gxArray* captureObjects,
      unsigned char selector,
      dlmsVARIANT* parameters,
      gxArray* columns);
#endif //DLMS_IGNORE_PROFILE_GENERIC

#ifdef  __cplusplus
}
#endif

#endif //COSEM_H
