/* Copyright (c) 1996-2016, OPC Foundation. All rights reserved.

   The source code in this file is covered under a dual-license scenario:
     - RCL: for OPC Foundation members in good-standing
     - GPL V2: everybody else

   RCL license terms accompanied with this source code. See http://opcfoundation.org/License/RCL/1.00/

   GNU General Public License as published by the Free Software Foundation;
   version 2 of the License are accompanied with this source code. See http://opcfoundation.org/License/GPLv2

   This source code is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/******************************************************************************************************/
/* Platform Portability Layer                                                                         */
/* Modify the content of this file according to the event implementation on your system.              */
/* This is the win32 implementation                                                                   */
/******************************************************************************************************/

/**
 * @brief Returns the current time in UTC.
 */
OpcUa_DateTime OPCUA_DLLCALL OpcUa_P_DateTime_UtcNow(void);

/**
 * @brief Returns the time in OpcUa_TimeVal format.
 */
OpcUa_Void OPCUA_DLLCALL OpcUa_P_DateTime_GetTimeOfDay(OpcUa_TimeVal*  pValue);

/**
 * @brief Converts the given OpcUa_DateTime into a ascii string.
 * 
 * @param DateTime   [in]        The DateTime value to convert.
 * @param Buffer     [in/out]    At least 25 bytes of buffer. 
 * @param DateTime   [in]        The length of the given buffer.
 *
 * @return Error Code
 */
OpcUa_StatusCode OPCUA_DLLCALL OpcUa_P_DateTime_GetStringFromDateTime(  OpcUa_DateTime datetime, 
                                                                        OpcUa_StringA  buffer,
                                                                        OpcUa_UInt32   length);

/**
 * @brief Converts the given (ascii) string into OpcUa_DateTime format.
 *
 * @param DateTimeString [in]    Buffer containing the DateTime string. Must not be OpcUa_Null!
 * @param DateTime      [out]    Pointer to the OpcUa_DateTime value in which the converted value will be stored.
 *
 * @return Status Code.
 */
OpcUa_StatusCode OPCUA_DLLCALL OpcUa_P_DateTime_GetDateTimeFromString(  OpcUa_StringA   DateTimeString, 
                                                                        OpcUa_DateTime* DateTime);
