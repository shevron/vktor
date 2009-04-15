#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vktor.h>

#define MAX_BUFFSIZE 64
#define INDENT_STR "  "

#define print_indent for (i = 0; i < indent; i++) { printf(INDENT_STR);	}

int indent = 0;

static char*
copy_string(char *src, int src_len)
{
	char *dest;
	
	dest = malloc(sizeof(char) * (src_len + 1));
	dest = memcpy(dest, src, src_len);
	dest[src_len] = '\0';
	
	return dest;	
}

static void
handle_token(vktor_token type, void * value, int size) 
{
	int   i;
	char *str;
	
	switch(type) {
		case VKTOR_TOKEN_ARRAY_START:
			print_indent;
			printf("(\n");
			indent++;
			break;
			
		case VKTOR_TOKEN_MAP_START:
			print_indent;
			printf("{\n");
			indent++;
			break;
		
		case VKTOR_TOKEN_MAP_KEY:
			print_indent;
			str = copy_string((char *) value, size);
			printf("%s:\n", str);
			free(str);
			break;
		
		case VKTOR_TOKEN_STRING:
			print_indent;
			str = copy_string((char *) value, size);
			printf("\"%s\"\n", str);
			free(str);
			break;
			
		case VKTOR_TOKEN_ARRAY_END:
			indent--;
			print_indent;
			printf(")\n");
			break;
		
		case VKTOR_TOKEN_MAP_END:
			indent--;
			print_indent;
			printf("}\n");
			break;
		
		default:  // not yet handled stuff
			print_indent;
			printf(" -- some value (%d) -- \n", type);
			break;
	}
}

int 
main(int argc, char *argv[]) 
{
	vktor_parser *parser;
	vktor_status  status;
	vktor_error  *error;
	char         *buffer;
	size_t        read_bytes;
	int           done = 0;
		
	parser = vktor_parser_init(128);
	
	do {
		status = vktor_parse(parser, &error);
		
		switch (status) {
			
			case VKTOR_OK:
				// Print the token
				handle_token(parser->token_type, parser->token_value, 
					parser->token_size);
				break;
				
			case VKTOR_COMPLETE: 
				// Print the token
				handle_token(parser->token_type, parser->token_value, 
					parser->token_size);
				printf("\nDone.\n");
				done = 1;
				break;
				
			case VKTOR_ERROR:
				// We have a parse error
				fprintf(stderr, "Paser error #%d: %s\n", error->code, 
					error->message);
					
				done = 1;
				break;
				
			case VKTOR_MORE_DATA:
				// We need to read more data
				buffer = malloc(sizeof(char) * MAX_BUFFSIZE);
				read_bytes = fread(buffer, sizeof(char), MAX_BUFFSIZE, stdin);
				if (read_bytes) {
					vktor_read_buffer(parser, buffer, read_bytes, &error);
					
				} else {
					// Nothing left to read
					done = 1;
					fprintf(stderr, "Error: premature end of stream\n");
				}
				break;
		}
		
	} while (! done);
	
	vktor_parser_free(parser);
	
	return 0;
}
