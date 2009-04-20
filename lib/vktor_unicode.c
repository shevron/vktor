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
char
vktor_unicode_hex_to_int(char hex)
{
	char i = 0;
	
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
 * Encode a 32 bit number representing a Unicode code point (UCS-4 assumed) to 
 * a UTF-8 encoded string. 
 * 
 * Conversion method is according to http://www.ietf.org/rfc/rfc2279.txt
 * 
 * @param [in]  cp    the unicode codepoint
 * @param [out] utf8  a pointer to a 5 byte long string (at least) that will 
 *   be populated with the UTF-8 encoded string
 * 
 * @return the length of the UTF-8 string (1 - 4 bytes) or 0 in case of error
 */
int
vktor_unicode_cp_to_utf8(unsigned short cp, char *utf8) 
{
	int   len = 0;
	
	assert(sizeof(utf8) >= 4);
	
	if (cp <= 0x7f) {
		// 1 byte UTF-8, equivalent to ASCII
		utf8[0] = (char) cp;
		len  = 1;
		
	} else if (cp <= 0x7ff) {
		// 2 byte UTF-8
		utf8[1] = (char) cp & 0xbf;
		utf8[0] = (char) cp >> 4 & 0xdf;
		len = 2;
		
	} else if (cp <= 0xffff) {
		// 3 byte UTF-8
		utf8[0] = (char) cp >> 16 & 0xef;
		utf8[1] = (char) cp >> 8  & 0xbf;
		utf8[2] = (char) cp & 0xbf;
		len = 3;
		
	} else if (cp <= 0x1fffff) {
		// 4 byte UTF-8
		utf8[0] = (char) cp & 0xbf;
		utf8[1] = (char) cp >> 4  & 0xbf;
		utf8[2] = (char) cp >> 8  & 0xbf;
		utf8[3] = (char) cp >> 12 & 0xf7;
		len = 4;
		
	} else if (cp <= 0x3ffffff) {
		fprintf(stderr, "NOT YET SUPPORTED");
	} else if (cp <= 0x7fffffff) {
		fprintf(stderr, "NOT YET SUPPORTED");
	}
	
	utf8[len] = '\0';
	
	return len;
}
