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

/**
 * @file vktor.c 
 * Main vktor library file
 */

#include "config.h"
#include "vktor.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

/**
 * Maximal error string length (mostly for internal use)
 */
#define VKTOR_ERR_STRLEN 1024

/**
 * Convenience macro to check if we are at the end of a buffer
 */
#define eobuffer(b) (b->ptr >= b->size)

/**
 * @brief Initialize a new parser 
 * 
 * Initialize and return a new parser struct. Will return NULL if memory can't 
 * be allocated.
 * 
 * @return a newly allocated parser
 */
vktor_parser*
vktor_parser_init()
{
	vktor_parser *parser;
	
	if ((parser = malloc(sizeof(vktor_parser))) == NULL) {
		return NULL;
	}
		
	parser->buffer      = NULL;
	parser->last_buffer = NULL;
	parser->token_type  = VKTOR_TOKEN_NONE;
	parser->token_value = NULL;
	parser->level       = 0;
	
	return parser;
}

/**
 * @brief Free a vktor_buffer struct
 * 
 * Free a vktor_buffer struct without following any next buffers in the chain. 
 * Call vktor_buffer_free_all() to free an entire chain of buffers.
 * 
 * @param[in,out] buffer the buffer to free
 */
static void 
vktor_buffer_free(vktor_buffer *buffer)
{
	assert(buffer != NULL);
	assert(buffer->text != NULL);
	
	free(buffer->text);
	free(buffer);
}

/**
 * @brief Free an entire linked list of vktor buffers
 * 
 * Free an entire linked list of vktor buffers. Will usually be called by 
 * vktor_parser_free() to free all buffers attached to a parser. 
 * 
 * @param[in,out] buffer the first buffer in the list to free
 */
static void 
vktor_buffer_free_all(vktor_buffer *buffer)
{
	vktor_buffer *next;
	
	while (buffer != NULL) {
		next = buffer->next_buff;
		vktor_buffer_free(buffer);
		buffer = next;
	}
}

/**
 * @brief Free a parser and any associated memory
 * 
 * Free a parser and any associated memory structures, including any linked
 * buffers
 * 
 * @param [in,out] parser parser struct to free
 */
void
vktor_parser_free(vktor_parser *parser)
{
	assert(parser != NULL);
	
	if (parser->buffer != NULL) {
		vktor_buffer_free_all(parser->buffer);
	}
	
	if (parser->token_value != NULL) {
		free(parser->token_value);
	}
	
	free(parser);
}

/**
 * @brief Initialize and populate new error struct
 *
 * If eptr is NULL, will do nothing. Otherwise, will initialize a new error
 * struct with an error message and code, and set eptr to point to it. 
 * 
 * The error message is passed as an sprintf-style format and an arbitrary set
 * of parameters, just like sprintf() would be used. 
 * 
 * Used internally to pass error messages back to the user. 
 * 
 * @param [in,out] eptr error struct pointer-pointer to populate or NULL
 * @param [in]     code error code
 * @param [in]     msg  error message (sprintf-style format)
 */
static void 
vktor_error_set(vktor_error **eptr, vktor_errcode code, const char *msg, ...)
{
	vktor_error *err;
	
	if (eptr == NULL) {
		return;
	}
	
	if ((err = malloc(sizeof(vktor_error))) == NULL) {
		return;
	}
	
	err->code = code;
	err->message = malloc(VKTOR_ERR_STRLEN * sizeof(char));
	if (err->message != NULL) {
		va_list ap;
		      
		va_start(ap, msg);
		vsnprintf(err->message, VKTOR_ERR_STRLEN, msg, ap);
		va_end(ap);
	}
	
	*eptr = err;
}

/**
 * @brief Free an error struct
 * 
 * Free an error struct
 * 
 * @param [in,out] err Error struct to free
 */
void 
vktor_error_free(vktor_error *err)
{
	if (err->message != NULL) {
		free(err->message);
	}
	
	free(err);
}

/**
 * @brief Initialize a vktor buffer struct
 * 
 * Initialize a vktor buffer struct and set it's associated text and other 
 * properties
 * 
 * @param [in] text buffer contents
 * @param [in] text_len the length of the buffer
 * 
 * @return A newly-allocated buffer struct
 */
static vktor_buffer*
vktor_buffer_init(char *text, long text_len)
{
	vktor_buffer *buffer;
	
	if ((buffer = malloc(sizeof(vktor_buffer))) == NULL) {
		return NULL;
	}
	
	buffer->text      = text;
	buffer->size      = text_len;
	buffer->ptr       = 0;
	buffer->next_buff = NULL;
	
	return buffer;
}

/**
 * @brief Read and store JSON text in the internal buffer
 * 
 * Read and store JSON text in the internal buffer, to be used later when 
 * parsing. This function should be called before starting to parse at least
 * once (to feed the parser), and again whenever new data is available and the
 * VKTOR_MORE_DATA status is returned from vktor_parse().
 * 
 * @param [in] parser   parser object
 * @param [in] text     text to add to buffer
 * @param [in] text_len length of text to add to buffer
 * @param [in,out] err  pointer to an unallocated error struct to return any 
 *                      errors, or NULL if there is no need for error handling
 * 
 * @return vktor status code (OK on success, ERROR otherwise)
 */
vktor_status 
vktor_read_buffer(vktor_parser *parser, char *text, long text_len, 
                  vktor_error **err) 
{
	vktor_buffer *buffer;
	
	// Create buffer
	if ((buffer = vktor_buffer_init(text, text_len)) == NULL) {
		vktor_error_set(err, VKTOR_ERR_OUT_OF_MEMORY, 
			"Unable to allocate memory buffer for %ld bytes", text_len);
		return VKTOR_ERROR;
	}
	
	// Link buffer to end of parser buffer chain
	if (parser->last_buffer == NULL) {
		assert(parser->buffer == NULL);
		parser->buffer = buffer;
		parser->last_buffer = buffer;
	} else {
		parser->last_buffer->next_buff = buffer;
		parser->last_buffer = buffer;
	}
	
	return VKTOR_OK;
}

/**
 * @brief Advance the parser to the next buffer
 * 
 * Advance the parser to the next buffer in the parser's buffer list. Called 
 * when the end of the current buffer is reached, and more data is required. 
 * 
 * If no further buffers are available, will set vktor_parser->buffer and 
 * vktor_parser->last_buffer to NULL.
 * 
 * @param [in,out] parser The parser we are working with
 */
static void 
vktor_parser_advance_buffer(vktor_parser *parser)
{
	vktor_buffer *next;
	
	assert(parser->buffer != NULL);
	assert(eobuffer(parser->buffer));
	
	next = parser->buffer->next_buff;
	vktor_buffer_free(parser->buffer);
	parser->buffer = next;
	
	if (parser->buffer == NULL) {
		parser->last_buffer = NULL;
	}
}

/**
 * @brief Parse some JSON text and return on the next token
 * 
 * Parse the text buffer until the next JSON token is encountered
 * 
 * In case of error, if error is not NULL, it will be populated with error 
 * information, and VKTOR_ERROR will be returned
 * 
 * @param [in,out] parser The parser object to work with
 * @param [out]    error  A vktor_error pointer pointer, or NULL
 * 
 * @return Status code:
 *  - VKTOR_OK        if a token was encountered
 *  - VKTOR_ERROR     if an error has occured
 *  - VKTOR_MORE_DATA if we need more data in order to continue parsing
 *  - VKTOR_COMPLETE  if parsing is complete and no further data is expected
 */
vktor_status 
vktor_parse(vktor_parser *parser, vktor_error **error)
{
	char c;
	int  done;
	
	assert(parser != NULL);
	
	// Do we have a buffer to work with?
	while (parser->buffer != NULL) {
		done = 0;
		
		while (! eobuffer(parser->buffer)) {
			c = parser->buffer->text[parser->buffer->ptr];
		
			switch (c) {
				case '{':
					done = 1;
					printf("{");
					break;
					
				case '(':
					done = 1;
					printf("(");
					break;
					
				case '"':
					done = 1;
					printf("\"");
					break;
				
				case ',':
					done = 1;
					printf(",");
					break;
					
				case '}':
					done = 1;
					printf("}");
					break;
					
				case ')':
					done = 1;
					printf(")");
					break;
					
				case ' ':
				case '\n':
				case '\r':
				case '\t':
					printf(" ");
					break;
					
				default:
					printf(".");
					// Check for digits
					break;
			}
			
			parser->buffer->ptr++;
			if (done) break;
		}
		
		if (done) break;
		vktor_parser_advance_buffer(parser);	
	}
	
	if (parser->buffer == NULL) {
		return VKTOR_MORE_DATA;
	} else {
		return VKTOR_OK;
	}	
}
