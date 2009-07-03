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
 * @file vktortest-validate.c 
 * 
 * A simple JSON validator, used here for testing purposes.
 * 
 * This program reads a JSON stream from standard input, and validates it as
 * it is read.
 * 
 * The return code of the program should be 0 if all is ok and the stream is
 * valid. Otherwise, one of the VKTOR_ERR codes as returned from the parser 
 * is returned in case of a parser error. 255 is retuned in case of an error 
 * unrelated to the parser.
 * 
 * You can use the code here as an example of how to write a simple JSON parser
 * using libvktor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vktor.h>

#define DEFAULT_BUFFSIZE 4096
#define DEFAULT_MAXDEPTH 32

int 
main(int argc, char *argv[], char *envp[]) 
{
	vktor_parser *parser;
	vktor_status  status;
	vktor_error  *error = NULL;
	char         *buffer;
	size_t        read_bytes;
	int           done = 0, ret = 0;
	char         *envvar;
	int           buffsize = DEFAULT_BUFFSIZE;
	int           maxdepth = DEFAULT_MAXDEPTH;
	
	/* Set buffer size from environment, if set */
	if ((envvar = getenv("BUFFSIZE")) != NULL) {
		buffsize = atoi(envvar);
	}
	
	/* Set max depth from environment, if set */
	if ((envvar = getenv("MAXDEPTH")) != NULL) {
		maxdepth = atoi(envvar);
	}

	parser = vktor_parser_init(maxdepth);
	
	do {
		status = vktor_parse(parser, &error);
		
		switch (status) {
			
			case VKTOR_OK:
				break;
				
			case VKTOR_MORE_DATA:
				// We need to read more data
				buffer = malloc(sizeof(char) * buffsize);
				read_bytes = fread(buffer, sizeof(char), buffsize, stdin);
				if (read_bytes) {
					vktor_feed(parser, buffer, read_bytes, 1, &error);
					
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
	
	if (error != NULL) {
		vktor_error_free(error);
	}
	
	vktor_parser_free(parser);
	
	return ret;
}
