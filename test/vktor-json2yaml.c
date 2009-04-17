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
 * @file vktortest-json2yaml.c 
 * 
 * A simple JSON to (kind of) YAML converter based on libvktor, used here for 
 * testing purposes.
 * 
 * This program reads a JSON stream from standard input, and writes the same
 * data back in YAML-like format to standard output. 
 * 
 * If the BUFFSIZE environment variable is set, it defines the read buffer size
 * for reading from STDIN (the default value is 64 bytes - this is very small
 * but is intentional for testing purposes)
 * 
 * Please note that this tool is not meant to produce valid YAML output - 
 * the purpose is only to generate some consistent output which could be used
 * to test the JSON parser. This is not meant to be a good example of a YAML 
 * writer.
 * 
 * The return code of the program should be 0 if all is ok. Otherwise, one of
 * the VKTOR_ERR codes as returned from the parser is returned in case of a 
 * parser error. 255 is retuned in case of an error unrelated to the parser.
 * 
 * You can use the code here as an example of how to write a simple JSON parser
 * using libvktor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vktor.h>

#define DEFAULT_BUFFSIZE 64
#define INDENT_STR "  "

#define print_indent(s) for (i = 0; i < indent; i++) { printf(s);	}

#define print_array_indent_dash(s)       \
	if (nest == VKTOR_CONTAINER_ARRAY) { \
		print_indent(s);                 \
		printf("- ");                    \
	}

int indent  = 0;
int is_root = 1;

static int
handle_token(vktor_parser *parser, vktor_container nest, vktor_error **error)
{
	char   *str;
	long    num;
	int     i;
	double  dbl;
	
	assert(indent >= 0);
	
	*error = NULL;
	
	switch(vktor_get_token_type(parser)) {
		case VKTOR_T_ARRAY_START:
			if (! is_root) {
				print_array_indent_dash(INDENT_STR);
				printf("\n");
				indent++;
			} else {
				is_root = 0;
			}
			break;
			
		case VKTOR_T_MAP_START:
			if (! is_root) {
				print_array_indent_dash(INDENT_STR);
				printf("\n");
				indent++;
			} else {
				is_root = 0;
			}
			
			break;
		
		case VKTOR_T_MAP_KEY:
			print_indent(INDENT_STR);
			vktor_get_value_str(parser, &str, error);
			if (*error != NULL) {
				return 0;
			}
			
			printf("\"%s\": ", str);
			break;
		
		case VKTOR_T_STRING:
			print_array_indent_dash(INDENT_STR);
			vktor_get_value_str(parser, &str, error);
			if (*error != NULL) {
				return 0;
			}
			
			printf("\"%s\"\n", str);
			break;
		
		case VKTOR_T_INT:
			num = vktor_get_value_long(parser, error);
			if (*error != NULL) {
				if ((*error)->code == VKTOR_ERR_OUT_OF_RANGE) {
					// Out of range, get value as string
					print_array_indent_dash(INDENT_STR);
					vktor_get_value_str(parser, &str, error);
					if (*error != NULL) {
						return 0;
					}
					printf("%s ## AS STRING ##\n", str);
					break;
				} else {
					return 0;
				}
			}
			
			print_array_indent_dash(INDENT_STR);
			printf("%ld\n", num);
			break;
			
		case VKTOR_T_FLOAT:
			dbl = vktor_get_value_double(parser, error);
			if (*error != NULL) {
				if ((*error)->code == VKTOR_ERR_OUT_OF_RANGE) {
					// Out of range, get value as string
					print_array_indent_dash(INDENT_STR);
					vktor_get_value_str(parser, &str, error);
					if (*error != NULL) {
						return 0;
					}
					printf("%s ## AS STRING ##\n", str);
					break;
				} else {
					return 0;
				}
			}
			
			print_array_indent_dash(INDENT_STR);
			printf("%.5f\n", dbl);
			break;
			
		case VKTOR_T_ARRAY_END:
			indent--;
			break;
		
		case VKTOR_T_MAP_END:
			indent--;
			break;
		
		case VKTOR_T_NULL:
			print_array_indent_dash(INDENT_STR);
			printf("null\n");
			break;
		
		case VKTOR_T_TRUE:
			print_array_indent_dash(INDENT_STR);
			printf("true\n");
			break;
		
		case VKTOR_T_FALSE:
			print_array_indent_dash(INDENT_STR);
			printf("false\n");
			break;
			
		default:  // not yet handled stuff
			print_array_indent_dash(INDENT_STR);
			printf("--- VKTOR UNHANDLED TOKEN: %d\n", 
				vktor_get_token_type(parser));
			break;
	}
	
	return 1;
}

int 
main(int argc, char *argv[], char *envp[]) 
{
	vktor_parser    *parser;
	vktor_status     status;
	vktor_error     *error;
	vktor_container  nest;
	char            *buffer;
	size_t           read_bytes;
	int              done = 0, ret = 0;
	char            *buffsize_c;
	int              buffsize = DEFAULT_BUFFSIZE;
	
	// Set buffer size from environment, if set
	if ((buffsize_c = getenv("BUFFSIZE")) != NULL) {
		buffsize = atoi(buffsize_c);
	}
	
	parser = vktor_parser_init(128);
	
	do {
		nest = vktor_get_current_container(parser);
		status = vktor_parse(parser, &error);
		
		switch (status) {
			
			case VKTOR_OK:
				// Print the token
				if (! handle_token(parser, nest, &error)) {
					fprintf(stderr, "Paser error [%d]: %s\n", error->code, 
						error->message);
					ret = error->code;
					done = 1;
				}
				break;
				
			case VKTOR_MORE_DATA:
				// We need to read more data
				buffer = malloc(sizeof(char) * buffsize);
				read_bytes = fread(buffer, sizeof(char), buffsize, stdin);
				if (read_bytes) {
					vktor_read_buffer(parser, buffer, read_bytes, &error);
					
				} else {
					// Nothing left to read
					done = 1;
					ret = 255;
					fprintf(stderr, "Error: premature end of stream\n");
				}
				break;
				
			case VKTOR_COMPLETE: 
				// Parser says we are done
				done = 1;
				break;
				
			case VKTOR_ERROR:
				// We have a parse error
				fprintf(stderr, "Paser error [%d]: %s\n", error->code, 
					error->message);
				ret = error->code;
				done = 1;
				break;
		}
		
	} while (! done);
	
	vktor_parser_free(parser);
	
	return ret;
}
