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

#ifndef _VKTOR_H

/* type definitions */

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
 * Possible vktor parser status codes
 */
typedef enum {
	VKTOR_ERROR,     /**< An error has occured */
	VKTOR_OK,        /**< Everything is OK */
	VKTOR_MORE_DATA, /**< More data is required in order to continue parsing */
	VKTOR_COMPLETE   /**< Parsing is complete, no further data is expected */
} vktor_status;

/**
 * Possible error codes used in vktor_error->code 
 */
typedef enum {
	VKTOR_ERR_OUT_OF_MEMORY,    /**< can't allocate memory */
	VKTOR_ERR_UNEXPECTED_INPUT, /**< unexpected characters in input buffer */
	VKTOR_ERR_INCOMPLETE_DATA   /**< can't finish parsing without more data */
} vktor_errcode;

/**
 * Error structure, signifying the error code and error message
 * 
 * Error structs must be freed using vktor_error_free()
 */
typedef struct _vktor_error_struct {
	vktor_errcode  code;    /**< error code */
	char          *message; /**< error message */
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
	char                        *text;      /**< buffer text */
	long                         size;      /**< buffer size */
	long                         ptr;       /**< internal buffer position */
	struct _vktor_buffer_struct *next_buff;	/**< pointer to the next buffer */
} vktor_buffer;

/**
 * Parser struct - this is the main object used by the user to parse a JSON 
 * stream. 
 */
typedef struct _vktor_parser_struct {
	vktor_buffer   *buffer;      /**< the current buffer being parsed */
	vktor_buffer   *last_buffer; /**< a pointer to the last buffer */ 
	vktor_token     token_type;  /**< current token type */
	void           *token_value; /**< current token value, if any */
	unsigned long   level;       /**< current nesting level */
	// Some configuration options?
} vktor_parser;

/* function prototypes */

/**
 * @brief Initialize a new parser 
 * 
 * Initialize and return a new parser struct. Will return NULL if memory can't 
 * be allocated.
 * 
 * @return a newly allocated parser
 */
vktor_parser* vktor_parser_init();

/**
 * @brief Free a parser and any associated memory
 * 
 * Free a parser and any associated memory structures, including any linked
 * buffers
 * 
 * @param [in,out] parser parser struct to free
 */
void vktor_parser_free(vktor_parser *parser);

/**
 * @brief Free an error struct
 * 
 * Free an error struct
 * 
 * @param [in,out] err Error struct to free
 */
void vktor_error_free(vktor_error *err);

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
vktor_status vktor_read_buffer(vktor_parser *parser, char *text, long text_len, 
                               vktor_error **err);

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
vktor_status vktor_parse(vktor_parser *parser, vktor_error **error);
		  

#define _VKTOR_H
#endif /* VKTOR_H */
