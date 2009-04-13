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

#include "vktor.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#define VKTOR_ERR_STRLEN 1024

/**
 * vktor_parser_init:
 * 
 * Initialize and return a new parser struct. Will return NULL if memory can't 
 * be allocated.
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
 * vktor_buffer_free: 
 * @buffer Buffer to free
 * 
 * Free a vktor_buffer struct without following any next buffers in the chain. 
 * Call vktor_buffer_free_all() to free an entire chain of buffers.
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
 * vktor_buffer_fre_all:
 * @buffer First buffer in the list to free
 * 
 * Free an entire linked list of vktor buffers
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
 * vktor_parser_free:
 * @parser Parser struct to free 
 * 
 * Free parser and any associated memory
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

static void 
vktor_error_set(vktor_error **eptr, unsigned short code, const char *msg, ...)
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

void 
vktor_error_free(vktor_error *err)
{
	if (err->message != NULL) {
		free(err->message);
	}
	
	free(err);
}

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

vktor_status 
vktor_read_buffer(vktor_parser *parser, char *text, long text_len, 
                  vktor_error **err) 
{
	vktor_buffer *buffer;
	
	// Create buffer
	if ((buffer = vktor_buffer_init(text, text_len)) == NULL) {
		vktor_error_set(err, 1, "Unable to allocate memory buffer for %ld bytes", 
			text_len);
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
