/*
 *  string_utilities.h
 *
 *  Copyright 2013 Michael Zillgith
 *
 *	This file is part of libIEC61850.
 *
 *	libIEC61850 is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	libIEC61850 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	See COPYING file for the complete license text.
 */

#ifndef STRING_UTILITIES_H_
#define STRING_UTILITIES_H_

#include "libiec61850_platform_includes.h"
#include "linked_list.h"

#ifdef __cplusplus
extern "C" {
#endif

char*
StringUtils_copyString(const char* string);

char*
StringUtils_copyStringToBuffer(const char* string, char* buffer);

char*
StringUtils_copySubString(char* startPos, char* endPos);

/**
 * \brief Concatenate strings. count indicates the number of strings
 * to concatenate.
 */
char*
StringUtils_createString(int count, ...);

/**
 * \brief Concatenate strings in user provided buffer. count indicates the number of strings
 * to concatenate.
 */
char*
StringUtils_createStringInBuffer(char* buffer, int count, ...);

char*
StringUtils_createStringFromBuffer(const uint8_t* buf, int size);

char*
StringUtils_createStringFromBufferInBuffer(char* newString, const uint8_t* buf, int size);

void
StringUtils_replace(char* string, char oldChar, char newChar);

bool
StringUtils_isDigit(char character);

int
StringUtils_digitToInt(char digit);

int
StringUtils_digitsToInt(const char* digits, int count);

int
StringUtils_createBufferFromHexString(char* hexString, uint8_t* buffer);

/**
 * \brief test if string starts with prefix
 */
bool
StringUtils_startsWith(char* string, char* prefix);

bool
StringUtils_endsWith(const char* str, const char* suffix);

/**
 * \brief Compare to characters using the collation order as defined in ISO 9506-2 7.5.2
 *
 * \param a the first string
 * \param b the second string
 *
 * \returns 0 if a equals b; a positive number if b > a; a negative number if b < a
 */
int
StringUtils_compareChars(char a, char b);

/**
 * \brief Compare to strings using the collation order as defined in ISO 9506-2 7.5.2
 *
 * \param a the first string
 * \param b the second string
 *
 * \returns 0 if a equals b; a positive number if b > a; a negative number if b < a
 */
int
StringUtils_compareStrings(const char* a, const char* b);

/**
 * \brief sort a list of strings alphabetically (according to the MMS identifier collation order)
 *
 * \param list a list that contains string elements
 */
void
StringUtils_sortList(LinkedList list);

/**
 * \brief Convert IPv6 address string to byte array (uint8_t array)
 *
 * \param addressString the IPv6 address string in standard notation
 * \param ipV6Addr buffer to store the IPv6 address bytes (has to be at least 16 bytes)
 *
 * \return true when parsing has been successful, false otherwise
 */
bool
StringUtils_convertIPv6AdddressStringToByteArray(const char* addressString, uint8_t ipV6Addr[]);

#ifdef __cplusplus
}
#endif


#endif /* STRING_UTILITIES_H_ */
