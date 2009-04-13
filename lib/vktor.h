/**
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

#ifndef _VKTOR_H

/* type definitions */

/* parser status codes */
typedef enum {
	VKTOR_ERROR,
	VKTOR_OK,
	VKTOR_MORE_DATA
} vktor_status;

/* parser token types */
typedef enum {
	VKTOR_TOKEN_NONE,
	VKTOR_TOKEN_NULL,
	VKTOR_TOKEN_BOOL,
	VKTOR_TOKEN_INT,
	VKTOR_TOKEN_FLOAT,
	VKTOR_TOKEN_STRING,
	VKTOR_TOKEN_ARRAY_START,
	VKTOR_TOKEN_ARRAY_END,
	VKTOR_TOKEN_MAP_START,
	VKTOR_TOKEN_MAP_KEY,
	VKTOR_TOKEN_MAP_END
} vktor_token;

typedef struct _vktor_error_struct {
	unsigned short  code;
	char           *message;
} vktor_error;

typedef struct _vktor_buffer_struct {
	char                        *text;
	long                         size;
	long                         ptr;
	struct _vktor_buffer_struct *next_buff;	
} vktor_buffer;

typedef struct _vktor_parser_struct {
	vktor_buffer   *buffer;
	vktor_buffer   *last_buffer;
	vktor_token     token_type;
	void           *token_value;
	unsigned long   level;
	// Some configuration options?
} vktor_parser;

/* function prototypes */

vktor_parser* vktor_parser_init();
void          vktor_parser_free(vktor_parser *parser);
void          vktor_error_free(vktor_error *err);

#define _VKTOR_H
#endif /* VKTOR_H */
