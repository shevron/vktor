/* 
 * vktor JSON pull-parser library
 * 
 * Copyright (c) 2009 Shahar Evron
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE. 
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>

#include "vktor_unicode.h"

/**
 * @brief Convert a hexadecimal digit to it's integer value
 * 
 * Convert a single char containing a hexadecimal digit to it's integer value
 * (0 - 15). Used when converting escaped Unicode sequences to UTF-* characters.
 * 
 * Note that this function does not do any error checking because it is for 
 * internal use only. If the character is not a valid hex digit, 0 is returned. 
 * Obviously since 0 is a valid value it should not be used for error checking.
 * 
 * @param [in] char Hexadecimal character
 * 
 * @return Integer value (0 - 15). 
 */
unsigned char
vktor_unicode_hex_to_int(unsigned char hex)
{
	unsigned char i = 0;
	
	assert((hex >= '0' && hex <= '9') || 
	       (hex >= 'a' && hex <= 'f') ||
		   (hex >= 'A' && hex <= 'F'));
		   
	if (hex >= '0' && hex <= '9') {
		i = hex - 48;
	} else if (hex >= 'A' && hex <= 'F') {
		i = hex - 55;
	} else if (hex >= 'a' && hex <= 'f') {
		i = hex - 87;
	}
	
	return i;
}

/**
 * @brief Encode a Unicode code point to a UTF-8 string
 * 
 * Encode a 32 bit number representing a Unicode code point (UCS-2) to 
 * a UTF-8 encoded string. 
 * 
 * Based on example from http://www.unicode.org/Public/PROGRAMS/CVTUTF/
 * 
 * @param [in]  cp    the unicode codepoint
 * @param [out] utf8  a pointer to a 5 byte long string (at least) that will 
 *   be populated with the UTF-8 encoded string
 * 
 * @return the length of the UTF-8 string (1 - 4 bytes) or 0 in case of error
 */
int
vktor_unicode_cp_to_utf8(unsigned short cp, unsigned char *utf8) 
{
	int len = 0;
	
	assert(sizeof(utf8) >= 4);
	
	if (cp <= 0x7f) {
		// 1 byte UTF-8, equivalent to ASCII
		utf8[0] = (unsigned char) cp;
		len  = 1;
		
	} else if (cp <= 0x7ff) {
		// 2 byte UTF-8
		utf8[0] = 0xc0 | (cp >> 6);
		utf8[1] = 0x80 | (cp & 0x3f);
		len = 2;
		
	} else {
		// 3 byte UTF-8
		utf8[0] = (unsigned char) 0xe0 | (cp >> 12);
		utf8[1] = (unsigned char) 0x80 | ((cp >> 6) & 0x3f);
		utf8[2] = (unsigned char) 0x80 | (cp & 0x3f);
		len = 3;	
	}
	
	utf8[len] = '\0';
	
	return len;
}
