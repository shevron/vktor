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

/**
 * Possible vktor parser status codes
 */
typedef enum {
	VKTOR_ERROR,     /** < An error has occured */
	VKTOR_OK,        /** < Everything is OK */
	VKTOR_MORE_DATA  /** < More data is required in order to continue parsing */
} vktor_status;

/**
 * JSON token types
 * 
 * Whenever a token is encountered during the parsing process, the value of 
 * vktor_parser#token_type is set to one of these values
 */
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

/**
 * Error structure, signifying the error code and error message
 * 
 * Error structs must be freed using vktor_error_free()
 */
typedef struct _vktor_error_struct {
	unsigned short  code;
	char           *message;
} vktor_error;

/**
 * Buffer struct, containing some text to parse along with an internal pointer
 * and a link to the next buffer.
 * 
 * vktor internally holds text to be parsed as a linked list of buffers pushed
 * by the user, so no memory reallocations are required. Whenever a buffer is 
 * completely parsed, the parser will advance to the next buffer pointed by 
 * #next_buff and will free the previous buffer
 * 
 * This is done internally by the parser
 */
typedef struct _vktor_buffer_struct {
	char                        *text;      /** < buffer text */
	long                         size;      /** < buffer size */
	long                         ptr;       /** < internal buffer position */
	struct _vktor_buffer_struct *next_buff;	/** < pointer to the next buffer */
} vktor_buffer;

/**
 * Parser struct - this is the main object used by the user to parse a JSON 
 * stream. 
 */
typedef struct _vktor_parser_struct {
	vktor_buffer   *buffer;      /** < the current buffer being parsed */
	vktor_buffer   *last_buffer; /** < a pointer to the last buffer */ 
	vktor_token     token_type;  /** < current token type */
	void           *token_value; /** < current token value, if any */
	unsigned long   level;       /** < current nesting level */
	// Some configuration options?
} vktor_parser;

/* function prototypes */

vktor_parser* vktor_parser_init();
void          vktor_parser_free(vktor_parser *parser);
void          vktor_error_free(vktor_error *err);

#define _VKTOR_H
#endif /* VKTOR_H */
