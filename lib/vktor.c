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
 * 
 * @todo Terminology: use 'object' for JSON object everywhere instead of map
 * @todo Terminology: use 'struct' for JSON container (array or object)
 * @todo Drop the 'vktor' prefix from static stuff
 */

#include "config.h"
#include "vktor.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

/**
 * Maximal error string length (mostly for internal use)
 */
#define VKTOR_ERR_STRLEN 1024

/**
 * Memory allocation chunk size used when reading strings
 */
#ifndef VKTOR_STR_MEMCHUNK
#define VKTOR_STR_MEMCHUNK 128
#endif

/**
 * Memory allocation chunk size used when reading numbers
 */
#ifndef VKTOR_NUM_MEMCHUNK
#define VKTOR_NUM_MEMCHUNK 32
#endif

/**
 * Convenience macro to check if we are at the end of a buffer
 */
#define eobuffer(b) (b->ptr >= b->size)

/**
 * Convenience macro to set an 'unexpected character' error
 */
#define vktor_error_set_unexpected_c(e, c)         \
	vktor_error_set(e, VKTOR_ERR_UNEXPECTED_INPUT, \
		"Unexpected character in input: %c", c)

/**
 * A bitmask representing any 'value' token 
 */
#define VKTOR_VALUE_TOKEN VKTOR_T_NULL        | \
                          VKTOR_T_FALSE       | \
						  VKTOR_T_TRUE        | \
						  VKTOR_T_INT         | \
						  VKTOR_T_FLOAT       | \
						  VKTOR_T_STRING      | \
						  VKTOR_T_ARRAY_START | \
						  VKTOR_T_MAP_START

/**
 * Convenience macro to check if we are in a specific type of JSON struct
 */
#define nest_stack_in(p, c) (p->nest_stack[p->nest_ptr] == c)

/**
 * Convenience macro to easily set the expected next token map after a value
 * token, taking current container struct (if any) into account.
 */
#define expect_next_value_token(p)                 \
	switch(p->nest_stack[p->nest_ptr]) {           \
		case VKTOR_CONTAINER_OBJECT:               \
			p->expected = VKTOR_C_COMMA |    \
							VKTOR_T_MAP_END;   \
			break;                                 \
												   \
		case VKTOR_CONTAINER_ARRAY:                \
			p->expected = VKTOR_C_COMMA |    \
							VKTOR_T_ARRAY_END; \
			break;                                 \
												   \
		default:                                   \
			p->expected = VKTOR_T_NONE;      \
			break;                                 \
	}

/**
 * Convenience macro to check, and reallocate if needed, the memory size for
 * reading a token
 */
#define check_reallocate_token_memory(cs)                               \
	if ((ptr + 1) >= maxlen) {                                          \
		maxlen = maxlen + cs;                                           \
		if ((token = realloc(token, maxlen * sizeof(char))) == NULL) {  \
			vktor_error_set(error, VKTOR_ERR_OUT_OF_MEMORY,             \
				"unable to allocate %d more bytes for string parsing",  \
				cs);                                                    \
			return VKTOR_ERROR;                                         \
		}                                                               \
	}

/**
 * Special JSON characters - these are not returned to the user as tokens 
 * but still have a meaning when parsing JSON.
 */
enum {
	VKTOR_C_COMMA  = 1 << 16, /**< ",", used as struct separator */
	VKTOR_C_COLON  = 1 << 17, /**< ":", used to separate object key:value */
	VKTOR_C_DOT    = 1 << 18, /**< ".", used in floating-point numbers */ 
	VKTOR_C_SIGNUM = 1 << 19, /**< "+" or "-" used in numbers */
	VKTOR_C_EXP    = 1 << 20  /**< "e" or "E" used for number exponent */
};

/**
 * @brief Initialize a new parser 
 * 
 * Initialize and return a new parser struct. Will return NULL if memory can't 
 * be allocated.
 * 
 * @param [in] max_nest maximal nesting level
 * 
 * @return a newly allocated parser
 */
vktor_parser*
vktor_parser_init(int max_nest)
{
	vktor_parser *parser;
	
	if ((parser = malloc(sizeof(vktor_parser))) == NULL) {
		return NULL;
	}
		
	parser->buffer       = NULL;
	parser->last_buffer  = NULL;
	parser->token_type   = VKTOR_T_NONE;
	parser->token_value  = NULL;
	parser->token_resume = 0;
	
	// set expectated tokens
	parser->expected   = VKTOR_VALUE_TOKEN;

	// set up nesting stack
	parser->nest_stack   = calloc(sizeof(vktor_container), max_nest);
	parser->nest_ptr     = 0;
	parser->max_nest     = max_nest;
			
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
	
	free(parser->nest_stack);
	
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
parser_advance_buffer(vktor_parser *parser)
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
 * @brief Set the current token just read by the parser
 * 
 * Set the current token just read by the parser. Called when a token is 
 * encountered, before returning from vktor_parse(). The user can then access
 * the token information. Will also take care of freeing any previous token
 * held by the parser.
 * 
 * @param [in,out] parser Parser object
 * @param [in]     token  New token type
 * @param [in]     value  New token value or NULL if no value
 */
static void
parser_set_token(vktor_parser *parser, vktor_token token, void *value)
{
	parser->token_type = token;
	if (parser->token_value != NULL) {
		free(parser->token_value);
	}
	parser->token_value = value;
}

/**
 * @brief add a nesting level to the nesting stack
 * 
 * Add a nesting level to the nesting stack when a new array or object is 
 * encountered. Will make sure that the maximal nesting level is not 
 * overflowed.
 * 
 * @param [in,out] parser    Parser object
 * @param [in]     nest_type nesting type - array or map
 * @param [out]    error     an error struct pointer pointer or NULL
 * 
 * @return Status code - VKTOR_OK or VKTOR_ERROR
 */
static vktor_status
nest_stack_add(vktor_parser *parser, vktor_container nest_type, 
	vktor_error **error)
{
	assert(parser != NULL);
	
	parser->nest_ptr++;
	if (parser->nest_ptr >= parser->max_nest) {
		vktor_error_set(error, VKTOR_ERR_MAX_NEST, 
			"maximal nesting level of %d reached", parser->max_nest);
		return VKTOR_ERROR;
	}
	
	parser->nest_stack[parser->nest_ptr] = nest_type;
	
	return VKTOR_OK;
}

/**
 * @brief pop a nesting level out of the nesting stack
 * 
 * Pop a nesting level out of the nesting stack when the end of an array or a
 * map is encountered. Will ensure there are no stack underflows.
 * 
 * @param [in,out] parser Parser object
 * @param [out]    error struct pointer pointer or NULL
 * 
 * @return Status code: VKTOR_OK or VKTOR_ERROR
 */
static vktor_status
nest_stack_pop(vktor_parser *parser, vktor_error **error)
{
	assert(parser != NULL);
	assert(parser->nest_stack[parser->nest_ptr]);
	
	parser->nest_ptr--;
	if (parser->nest_ptr < 0) {
		vktor_error_set(error, VKTOR_ERR_INTERNAL_ERR, 
			"internal parser error: nesting stack pointer underflow");
		return VKTOR_ERROR;
	}
	
	return VKTOR_OK;
}

/**
 * @brief Read a string token
 * 
 * Read a string token until the ending double-quote. Will decode any special
 * escaped characters found along the way, and will gracefully handle buffer 
 * replacement. 
 * 
 * Used by parser_read_string_token() and parser_read_objkey_token()
 * 
 * @param [in,out] parser Parser object
 * @param [out]    error  Error object pointer pointer or NULL
 * 
 * @return Status code
 */
static vktor_status
parser_read_string(vktor_parser *parser, vktor_error **error)
{
	char  c;
	char *token;
	int   ptr, maxlen;
	int   done = 0;
	
	assert(parser != NULL);
	
	if (parser->token_resume) {
		ptr = parser->token_size;
		if (ptr < VKTOR_STR_MEMCHUNK) {
			maxlen = VKTOR_STR_MEMCHUNK;
			token = (void *) parser->token_value;
			assert(token != NULL);
		} else {
			maxlen = ptr + VKTOR_STR_MEMCHUNK;
			token = realloc(parser->token_value, sizeof(char) * maxlen);
		}	
		
	} else {
		token  = malloc(VKTOR_STR_MEMCHUNK * sizeof(char));
		maxlen = VKTOR_STR_MEMCHUNK;
		ptr    = 0;
	}
	
	if (token == NULL) {
		vktor_error_set(error, VKTOR_ERR_OUT_OF_MEMORY, 
			"unable to allocate %d bytes for string parsing", 
			VKTOR_STR_MEMCHUNK);
		return VKTOR_ERROR;
	}
	
	while (parser->buffer != NULL) {
		while (! eobuffer(parser->buffer)) {
			c = parser->buffer->text[parser->buffer->ptr];
			
			switch (c) {
				case '"':
					// end of string;
					done = 1;
					break;
					
				case '\\':
					/** @todo Handle quoted special characters */
					// Some quoted character
					
				default:
					/** @todo Handle control characters and unicode */
					token[ptr++] = c;
					check_reallocate_token_memory(VKTOR_STR_MEMCHUNK);
					break;
			}
			
			parser->buffer->ptr++;
			if (done) break;
		}
		
		if (done) break;
		parser_advance_buffer(parser);
	}
	
	parser->token_value = (void *) token;
	parser->token_size  = ptr;
	
	// Check if we need more data
	if (! done) {
		parser->token_resume = 1;
		return VKTOR_MORE_DATA;
	} else {
		token[ptr] = '\0';
		parser->token_resume = 0;
		return VKTOR_OK;
	}
}

/**
 * @brief Read a string token
 * 
 * Read a string token using parser_read_string() and set the next expected 
 * token map accordingly
 * 
 * @param [in,out] parser Parser object
 * @param [out]    error  Error object pointer pointer or null
 * 
 * @return Status code
 */
static vktor_status
parser_read_string_token(vktor_parser *parser, vktor_error **error)
{
	vktor_status status;
	
	// Read string	
	parser->token_type = VKTOR_T_STRING;
	status = parser_read_string(parser, error);
	
	// Set next expected token
	if (status == VKTOR_OK) {
		expect_next_value_token(parser);
	}
	
	return status;
}

/**
 * @brief Read an object key token
 * 
 * Read an object key using parser_read_string() and set the next expected 
 * token map accordingly
 * 
 * @param [in,out] parser Parser object
 * @param [out]    error  Error object pointer pointer or null
 * 
 * @return Status code
 */
static vktor_status
parser_read_objkey_token(vktor_parser *parser, vktor_error **error)
{
	vktor_status status;
	
	assert(nest_stack_in(parser, VKTOR_CONTAINER_OBJECT));
	
	// Read string	
	parser->token_type = VKTOR_T_MAP_KEY;
	status = parser_read_string(parser, error);
	
	// Set next expected token
	if (status == VKTOR_OK) {
		parser->expected = VKTOR_C_COLON;
	}
	
	return status;
}

/**
 * @brief Read an "expected" token
 * 
 * Read an "expected" token - used when we guess in advance what the next token
 * should be and want to try reading it. 
 * 
 * Used internally to read true, false, and null tokens.
 * 
 * If during the parsing the input does not match the expected token, an error
 * is returned.
 * 
 * @param [in,out] parser Parser object
 * @param [in]     expect Expected token as string
 * @param [in]     explen Expected token length
 * @param [out]    error  Error object pointer pointer or NULL
 * 
 * @return Status code
 */
static vktor_status
parser_read_expectedstr(vktor_parser *parser, const char *expect, int explen, 
	vktor_error **error)
{
	char  c;
	//~ int   ptr;
	
	assert(parser != NULL);
	assert(expect != NULL);
	
	// Expected string should be "null", "true" or "false"
	assert(explen > 3 && explen < 6);
	
	if (! parser->token_resume) {
		parser->token_size = 0;
	}
	
	for (; parser->token_size < explen; parser->token_size++) {
		if (parser->buffer == NULL) {
			parser->token_resume = 1;
			return VKTOR_MORE_DATA;
		}
		
		if (eobuffer(parser->buffer)) {
			parser_advance_buffer(parser);
			if (parser->buffer == NULL) {
				parser->token_resume = 1;
				return VKTOR_MORE_DATA;
			}
		}
		
		c = parser->buffer->text[parser->buffer->ptr];
		if (expect[parser->token_size] != c) {
			vktor_error_set_unexpected_c(error, c);
			return VKTOR_ERROR;
		}
		
		parser->buffer->ptr++;
	}
	
	// if we made it here, it means we are good!
	parser->token_size = 0;
	parser->token_resume = 0;
	return VKTOR_OK;
}

/**
 * @brief Read an expected null token
 * 
 * Read an expected null token using parser_read_expectedstr(). Will also set 
 * the next expected token map.
 * 
 * @param [in,out] parser Parser object
 * @param [out]    error  Error object pointer pointer or NULL
 * 
 * @return Status code
 */
static vktor_status
parser_read_null(vktor_parser *parser, vktor_error **error)
{
	vktor_status st = parser_read_expectedstr(parser, "null", 4, error);
	
	if (st != VKTOR_ERROR) {
		parser_set_token(parser, VKTOR_T_NULL, NULL);
		if (st == VKTOR_OK) {
			// Set the next expected token
			expect_next_value_token(parser);
		}
	}
	
	return st;
}

/**
 * @brief Read an expected true token
 * 
 * Read an expected true token using parser_read_expectedstr(). Will also set 
 * the next expected token map.
 * 
 * @param [in,out] parser Parser object
 * @param [out]    error  Error object pointer pointer or NULL
 * 
 * @return Status code
 */
static vktor_status
parser_read_true(vktor_parser *parser, vktor_error **error)
{
	vktor_status st = parser_read_expectedstr(parser, "true", 4, error);
	
	if (st != VKTOR_ERROR) {
		parser_set_token(parser, VKTOR_T_TRUE, NULL);
		if (st == VKTOR_OK) {
			// Set the next expected token
			expect_next_value_token(parser);
		}
	}
	
	return st;
}

/**
 * @brief Read an expected false token
 * 
 * Read an expected false token using parser_read_expectedstr(). Will also set 
 * the next expected token map.
 * 
 * @param [in,out] parser Parser object
 * @param [out]    error  Error object pointer pointer or NULL
 * 
 * @return Status code
 */
static vktor_status
parser_read_false(vktor_parser *parser, vktor_error **error)
{
	vktor_status st = parser_read_expectedstr(parser, "false", 5, error);
	
	if (st != VKTOR_ERROR) {
		parser_set_token(parser, VKTOR_T_FALSE, NULL);
		if (st == VKTOR_OK) {
			// Set the next expected token
			expect_next_value_token(parser);
		}
	}
	
	return st;
}

/**
 * @brief Read a number token
 * 
 * Read a number token - this might be an integer or a floating point number.
 * Will set the token_type accordingly. 
 * 
 * @param [in,out] parser Parser object
 * @param [out]    error  Error object pointer pointer
 * 
 * @return Status code
 */
static vktor_status 
parser_read_number_token(vktor_parser *parser, vktor_error **error)
{
	char  c;
	char *token;
	int   ptr, maxlen;
	int   done = 0;
	
	assert(parser != NULL);
	
	if (parser->token_resume) {
		ptr = parser->token_size;
		if (ptr < VKTOR_NUM_MEMCHUNK) {
			maxlen = VKTOR_NUM_MEMCHUNK;
			token = (void *) parser->token_value;
			assert(token != NULL);
		} else {
			maxlen = ptr + VKTOR_NUM_MEMCHUNK;
			token = realloc(parser->token_value, sizeof(char) * maxlen);
		}
		
	} else {
		token  = malloc(VKTOR_NUM_MEMCHUNK * sizeof(char));
		maxlen = VKTOR_NUM_MEMCHUNK;
		ptr    = 0;
		
		// Reading a new token - set possible expected characters
		parser->expected = VKTOR_T_INT | 
		                   VKTOR_T_FLOAT | 
						   VKTOR_C_DOT | 
						   VKTOR_C_EXP | 
						   VKTOR_C_SIGNUM;
						   
		// Token type is INT until proven otherwise 
		parser->token_type = VKTOR_T_INT;
	}
	
	if (token == NULL) {
		vktor_error_set(error, VKTOR_ERR_OUT_OF_MEMORY, 
			"unable to allocate %d bytes for string parsing", 
			VKTOR_NUM_MEMCHUNK);
		return VKTOR_ERROR;
	}
	
	while (parser->buffer != NULL) {
		while (! eobuffer(parser->buffer)) {
			c = parser->buffer->text[parser->buffer->ptr];
			
			switch (c) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					// Digits are always allowed
					token[ptr++] = c;
					
					// Signum cannot come after a digit
					parser->expected = (parser->expected & ~VKTOR_C_SIGNUM);
					break;
					
				case '.':
					if (! (parser->expected & VKTOR_C_DOT && ptr > 0)) {
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					token[ptr++] = c;
					
					// Dots are no longer allowed
					parser->expected = parser->expected & ~VKTOR_C_DOT;
					
					// This is a floating point number
					parser->token_type = VKTOR_T_FLOAT;
					break;
					
				case '-':
				case '+':
					if (! (parser->expected & VKTOR_C_SIGNUM)) {
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					token[ptr++] = c;
				
					// Signum is no longer allowed
					parser->expected = parser->expected & ~VKTOR_C_SIGNUM;
					break;
					
				case 'e':
				case 'E':
					if (! (parser->expected & VKTOR_C_EXP && ptr > 0)) {
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					// Make sure the previous sign is a number
					switch(token[ptr - 1]) {
						case '.':
						case '+':
						case '-':
							vktor_error_set_unexpected_c(error, c);
							return VKTOR_ERROR;
							break;
					}
					
					// Exponent is no longer allowed 
					parser->expected = parser->expected & ~VKTOR_C_EXP;
					
					// Dot is no longer allowed
					parser->expected = parser->expected & ~VKTOR_C_DOT;
					
					// Signum is now allowed again
					parser->expected = parser->expected | VKTOR_C_SIGNUM;
					
					// This is a floating point number
					parser->token_type = VKTOR_T_FLOAT;
					
					token[ptr++] = 'e';
					break;
					
				default:
					// Check that we are not expecting more digits
					assert(ptr > 0);
					switch(token[ptr - 1]) {
						case 'e':
						case 'E':
						case '.':
						case '+':
						case '-':
							vktor_error_set_unexpected_c(error, c);
							return VKTOR_ERROR;
							break;
					}
					
					done = 1;
					break;	
			}
			
			if (done) break;
			parser->buffer->ptr++;
			check_reallocate_token_memory(VKTOR_NUM_MEMCHUNK);
		}
		
		if (done) break;
		parser_advance_buffer(parser);
	}
	
	parser->token_value = (void *) token;
	parser->token_size  = ptr;
	
	// Check if we need more data
	if (! done) {
		parser->token_resume = 1;
		return VKTOR_MORE_DATA;
	} else {
		parser->token_resume = 0;
		expect_next_value_token(parser);
		return VKTOR_OK;
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
 * @return status code:
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
		
		// Do we need to continue reading the previous token?
		if (parser->token_resume) {
						
		    switch (parser->token_type) {
		    	case VKTOR_T_MAP_KEY:
		    		return parser_read_objkey_token(parser, error);
		    		break;
		    		
		    	case VKTOR_T_STRING:
		    		return parser_read_string_token(parser, error);
		    		break;
		    	
				case VKTOR_T_NULL:
					return parser_read_null(parser, error);
					break;
					
				case VKTOR_T_TRUE:
					return parser_read_true(parser, error);
					break;
				
				case VKTOR_T_FALSE:
					return parser_read_false(parser, error);
					break;
				
				case VKTOR_T_INT:
				case VKTOR_T_FLOAT:
					return parser_read_number_token(parser, error);
					break;
					
		    	default:
		    		vktor_error_set(error, VKTOR_ERR_INTERNAL_ERR, 
		    			"token resume flag is set but token type %d is unexpected",
		    			parser->token_type);
		    		return VKTOR_ERROR;
		    		break;
		    }
		}
		
		while (! eobuffer(parser->buffer)) {
			c = parser->buffer->text[parser->buffer->ptr];
			
			switch (c) {
				case '{':
					if (! parser->expected & VKTOR_T_MAP_START) {
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					if (nest_stack_add(parser, VKTOR_CONTAINER_OBJECT, error) == VKTOR_ERROR) {
						return VKTOR_ERROR;
					}
					
					parser_set_token(parser, VKTOR_T_MAP_START, NULL);
					
					// Expecting: map key or map end
					parser->expected = VKTOR_T_MAP_KEY |
					                     VKTOR_T_MAP_END;
					
					done = 1;
					break;
					
				case '[':
					if (! parser->expected & VKTOR_T_ARRAY_START) {
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					if (nest_stack_add(parser, VKTOR_CONTAINER_ARRAY, error) == VKTOR_ERROR) {
						return VKTOR_ERROR;
					}
					
					parser_set_token(parser, VKTOR_T_ARRAY_START, NULL);
					
					// Expecting: any value or array end
					parser->expected = VKTOR_VALUE_TOKEN | 
					                     VKTOR_T_ARRAY_END;
					
					done = 1;
					break;
					
				case '"':
					if (! parser->expected & (VKTOR_T_STRING | 
					                            VKTOR_T_MAP_KEY)) {
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					parser->buffer->ptr++;	 
					
					if (parser->expected & VKTOR_T_MAP_KEY) {
						return parser_read_objkey_token(parser, error);
					} else {
						return parser_read_string_token(parser, error);
					}
					
					break;
				
				case ',':
					if (! parser->expected & VKTOR_C_COMMA) {
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					switch(parser->nest_stack[parser->nest_ptr]) {
						case VKTOR_CONTAINER_OBJECT:
							parser->expected = VKTOR_T_MAP_KEY;
							break;
							
						case VKTOR_CONTAINER_ARRAY:
							parser->expected = VKTOR_VALUE_TOKEN;
							break;
							
						default:
							vktor_error_set(error, VKTOR_ERR_INTERNAL_ERR, 
								"internal parser error: unexpected nesting stack member");
							return VKTOR_ERROR;
							break;
					}
					
					//~ printf(",");
					break;
				
				case ':':
					if (! parser->expected & VKTOR_C_COLON) {
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					// Colon is only expected inside maps
					assert(nest_stack_in(parser, VKTOR_CONTAINER_OBJECT));
					
					// Next we expected a value
					parser->expected = VKTOR_VALUE_TOKEN;
					break;
					
				case '}':
					if (! (parser->expected & VKTOR_T_MAP_END &&
					       nest_stack_in(parser, VKTOR_CONTAINER_OBJECT))) {
					
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					parser_set_token(parser, VKTOR_T_MAP_END, NULL);
					
					if (nest_stack_pop(parser, error) == VKTOR_ERROR) {
						return VKTOR_ERROR;
					} 
					
					if (parser->nest_ptr > 0) {
						// Next can be either a comma, or end of array / map
						parser->expected = VKTOR_C_COMMA | 
											 VKTOR_T_MAP_END | 
											 VKTOR_T_ARRAY_END;
					} else {
						// Next can be nothing
						parser->expected = VKTOR_T_NONE;
					}
					                     
					done = 1;
					break;
					
				case ']':
					if (! (parser->expected & VKTOR_T_ARRAY_END &&
					       nest_stack_in(parser, VKTOR_CONTAINER_ARRAY))) { 
					
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					parser_set_token(parser, VKTOR_T_ARRAY_END, NULL);
					
					if (nest_stack_pop(parser, error) == VKTOR_ERROR) {
						return VKTOR_ERROR;
					} 
					
					if (parser->nest_ptr > 0) {
						// Next can be either a comma, or end of array / map
						parser->expected = VKTOR_C_COMMA | 
											 VKTOR_T_MAP_END | 
											 VKTOR_T_ARRAY_END;
					} else {
						// Next can be nothing
						parser->expected = VKTOR_T_NONE;
					}
					                     
					done = 1;
					break;
					
				case ' ':
				case '\n':
				case '\r':
				case '\t':
					// Whitespace - do nothing!
					/** 
					 * @todo consinder: read all whitespace without looping? 
					 */
					break;
					
				case 't':
					// true?
					if (! parser->expected & VKTOR_T_TRUE) {
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					return parser_read_true(parser, error);
					break;
					
				case 'f':
					// false?
					if (! parser->expected & VKTOR_T_FALSE) {
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					return parser_read_false(parser, error);
					break;

				case 'n':
					// null?
					if (! parser->expected & VKTOR_T_NULL) {
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					return parser_read_null(parser, error);
					break;
									
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case '-':
				case '+':
					// Read a number
					if (! parser->expected & (VKTOR_T_INT | 
					                          VKTOR_T_FLOAT)) {
						vktor_error_set_unexpected_c(error, c);
						return VKTOR_ERROR;
					}
					
					return parser_read_number_token(parser, error);
					break;
					
				default:
					// Unexpected character
					vktor_error_set_unexpected_c(error, c);
					return VKTOR_ERROR;
					break;
			}
			
			parser->buffer->ptr++;
			if (done) break;
		}
		
		if (done) break;
		parser_advance_buffer(parser);	
	}
	
	if (parser->buffer == NULL) {
		return VKTOR_MORE_DATA;
	} else {
		if (parser->nest_ptr == 0) {
			return VKTOR_COMPLETE;
		} else {
			return VKTOR_OK;
		}
	}
}

/**
 * @brief Get the current nesting depth
 * 
 * Get the current array/object nesting depth of the current token the parser
 * is pointing to
 * 
 * @param [in] parser Parser object
 * 
 * @return nesting level - 0 means top level
 */
int
vktor_get_depth(vktor_parser *parser) 
{
	return parser->nest_ptr;	
}

/**
 * @brief Get the current container type
 * 
 * Get the container type (object, array or none) containing the current token
 * pointed to by the parser
 * 
 * @param [in] parser Parser object
 * 
 * @return A vktor_container value or VKTOR_CONTAINER_NONE if we are in the top 
 *   level
 */
vktor_container
vktor_get_current_container(vktor_parser *parser)
{
	assert(parser != NULL);
	return parser->nest_stack[parser->nest_ptr];
}

/**
 * @brief Get the current token type
 * 
 * Get the type of the current token pointed to by the parser
 * 
 * @param [in] parser Parser object
 * 
 * @return Token type (one of the VKTOR_T_* tokens)
 */
vktor_token
vktor_get_token_type(vktor_parser *parser)
{
	assert(parser != NULL);
	return parser->token_type;	
}

/**
 * @brief Get the token value as a long integer
 * 
 * Get the value of the current token as a long integer. Suitable for reading
 * the value of VKTOR_T_INT tokens, but can also be used to get the integer 
 * value of VKTOR_T_FLOAT tokens and even any numeric prefix of a VKTOR_T_STRING
 * token. 
 * 
 * If the value of a number token is larger than the system's maximal long, 
 * 0 is returned and #error will indicate overflow. In such cases, 
 * vktor_get_value_string() should be used to get the value as a string.
 * 
 * @param [in]  parser Parser object
 * @param [out] error  Error object pointer pointer or null
 * 
 * @return The numeric value of the current token as a long int, 
 * @retval 0 in case of error (although 0 might also be normal, so check the 
 *         value of #error)
 */
long 
vktor_get_value_long(vktor_parser *parser, vktor_error **error)
{
	long val;
	
	assert(parser != NULL);
	
	if (parser->token_value == NULL) {
		vktor_error_set(error, VKTOR_ERR_NO_VALUE, "token value is unknown");
		return 0;
	}
	
	errno = 0;
	val = strtol((char *) parser->token_value, NULL, 10);
	if (errno == ERANGE) {
		vktor_error_set(error, VKTOR_ERR_OUT_OF_RANGE,
			"integer value is overflows maximal long value");
		return 0;
	}
	
	return val;
}

/**
 * @brief Get the value of the token as a string
 * 
 * Get the value of the current token as a string, as well as the length of the
 * token. Suitable for getting the value of a VKTOR_T_STRING token, but also 
 * for reading numeric values as a string. 
 * 
 * Note that the string pointer populated into #val is owned by the parser and 
 * should not be freed by the user.
 * 
 * @param [in]  parser Parser object
 * @param [out] val    Pointer-pointer to be populated with the value
 * @param [out] error  Error object pointer pointer or NULL
 * 
 * @return The length of the string
 * @retval 0 in case of error (although 0 might also be normal, so check the 
 *         value of #error)
 */
int
vktor_get_value_str(vktor_parser *parser, char **val, vktor_error **error)
{
	assert(parser != NULL);
	
	if (parser->token_value == NULL) {
		vktor_error_set(error, VKTOR_ERR_NO_VALUE, "token value is unknown");
		return -1;
	}
	
	*val = (char *) parser->token_value;
	return parser->token_size;
}

/**
 * @brief Get the value of the token as a string
 * 
 * Similar to vktor_get_value_str(), only this function will provide a copy of 
 * the string, which will need to be freed by the user when it is no longer 
 * used. 
 * 
 * @param [in]  parser Parser object
 * @param [out] val    Pointer-pointer to be populated with the value
 * @param [out] error  Error object pointer pointer or NULL
 * 
 * @return The length of the string
 * @retval 0 in case of error (although 0 might also be normal, so check the 
 *         value of #error)
 */
int 
vktor_get_value_str_copy(vktor_parser *parser, char **val, vktor_error **error)
{
	char *str;
	
	assert(parser != NULL);
	
	if (parser->token_value == NULL) {
		vktor_error_set(error, VKTOR_ERR_NO_VALUE, "token value is unknown");
		return 0;
	}
	
	str = malloc(sizeof(char) * (parser->token_size + 1));
	str = memcpy(str, parser->token_value, parser->token_size);
	str[parser->token_size] = '\0';
	
	*val = str;
	return parser->token_size;
}
