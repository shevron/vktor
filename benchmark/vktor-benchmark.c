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
 * @file vktortest-benchmark.c 
 * 
 * A simple program used to benchmark vktor.
 *
 * Takes a JSON file name as a parameter. Will parse this file and keep
 * a counter of the different JSON tokens in this file. 
 *
 * Additionally, will print the time it took to parse the entire file.
 *
 * The return code of the program should be 0 if all is ok and the JSON file
 * was successfully parsed. Otherwise, one of the VKTOR_ERR codes as returned 
 * from the parser is returned in case of a parser error. 255 is retuned in 
 * case of an error unrelated to the parser.
 * 
 * You can use the code here as an example of how to write a simple JSON parser
 * using libvktor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/errno.h>
#include <malloc/malloc.h>

#include <vktor.h>

#define DEFAULT_BUFFSIZE 4096
#define DEFAULT_MAXDEPTH 32

static unsigned int mallocs  = 0;
static unsigned int reallocs = 0;
static unsigned int frees    = 0;

void *my_malloc(size_t size);

void *my_realloc(void *pointer, size_t size);

void  my_free(void *pointer);

int 
main(int argc, char *argv[], char *envp[]) 
{
	vktor_parser   *parser;
	vktor_status    status;
	vktor_error    *error = NULL;
	char           *buffer;
	size_t          read_bytes;
	int             done = 0, ret = 0;
	char           *envvar;
	int             buffsize = DEFAULT_BUFFSIZE;
	int             maxdepth = DEFAULT_MAXDEPTH;
	FILE           *infile;
	clock_t         runtime; 
	char            memtest = 0;

	/* Counters */
	int c_nulls = 0, c_falses = 0, c_trues = 0, 
	    c_ints = 0, c_floats = 0, c_strings = 0,
	    c_arrays = 0, c_objects = 0, c_obj_keys = 0;

	/* Set buffer size from environment, if set */
	if ((envvar = getenv("BUFFSIZE")) != NULL) {
		buffsize = atoi(envvar);
	}
	
	/* Set max depth from environment, if set */
	if ((envvar = getenv("MAXDEPTH")) != NULL) {
		maxdepth = atoi(envvar);
	}

	/* Open input file */
	if (argc > 1) {
		infile = fopen(argv[1], "r");
	} else {
		infile = stdin;
	}

	if (infile == NULL) {
		perror("Error opening input file");
		exit(255);
	}

	/* Set memory handlers */
	if (getenv("MEMTEST") != NULL) {
		vktor_set_memory_handlers(my_malloc, my_realloc, my_free);
		memtest = 1;
	}

	runtime = clock();

	parser = vktor_parser_init(maxdepth);
	
	do {
		status = vktor_parse(parser, &error);
		
		switch (status) {
			
			case VKTOR_OK:
				switch(vktor_get_token_type(parser)) {
					case VKTOR_T_NULL: 
						c_nulls++;
						break;

					case VKTOR_T_FALSE:
						c_falses++;
						break;

					case VKTOR_T_TRUE:
						c_trues++;
						break;

					case VKTOR_T_INT:
						c_ints++;
						break;

					case VKTOR_T_FLOAT:
						c_floats++;
						break;

					case VKTOR_T_STRING:
						c_strings++;
						break;

					case VKTOR_T_ARRAY_START: 
						c_arrays++;
						break;

					case VKTOR_T_OBJECT_START:
						c_objects++;
						break;

					case VKTOR_T_OBJECT_KEY:
						c_obj_keys++;
						break;

					default:
						/* do nothing */
						break;
				}
				break;
				
			case VKTOR_MORE_DATA:
				// We need to read more data
				buffer = malloc(sizeof(char) * buffsize);
				read_bytes = fread(buffer, sizeof(char), buffsize, infile);
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

	/* Calculate parsing time */
	runtime = clock() - runtime;

	if (error != NULL) {
		vktor_error_free(error);
	}
	
	vktor_parser_free(parser);
	
	printf("------------------------------------------------------------------------\n"
	       "Finished parsing %s\n\n"
	       
	       "Sum of JSON tokens encountered:\n"
	       "  null:       %d\n"
	       "  false:      %d\n"
	       "  true:       %d\n"
	       "  integer:    %d\n"
	       "  float:      %d\n"
	       "  array:      %d\n"
	       "  object:     %d\n"
	       "  object key: %d\n\n",
	       
	       (argc > 1 ? argv[1] : "data from STDIN"), 
	       c_nulls, c_falses, c_trues, c_ints, c_floats, c_arrays, c_objects, c_obj_keys);

	if (memtest) {
		printf("malloc()  calls: %u\n"
		       "realloc() calls: %u\n"
		       "free()    calls: %u\n\n", mallocs, reallocs, frees); 
	}
	 
	printf("Total parsing time: %f seconds\n"
	       "------------------------------------------------------------------------\n", 
	       (double) runtime / CLOCKS_PER_SEC);

	return ret;
}

/* Wrapping malloc(), adding a counter of calls */
void *my_malloc(size_t size)
{
	mallocs++;
	return malloc(size);
}

/* Wrapping realloc, adding a counter of calls */
void *my_realloc(void *pointer, size_t size)
{
	reallocs++;
	return realloc(pointer, size);
}

/* Wrapping free, adding a counter of calls */
void  my_free(void *pointer)
{
	frees++;
	free(pointer);
}

