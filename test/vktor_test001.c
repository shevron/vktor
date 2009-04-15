#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vktor.h>

#define MAX_BUFFSIZE 64
#define INDENT_STR "  "

#define print_indent(s) int i; for (i = 0; i < indent; i++) { printf(s);	}

int indent      = 0;
int first_token = 1;

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
print_value(const char *value, vktor_container nest)
{
	if (nest != VKTOR_CONTAINER_OBJECT) { 
		if (indent > 0) {
			printf("%s\n", first_token ? "" : ",");
			print_indent(INDENT_STR);
		}		
	}
	
	printf("%s", value);
}

static void
handle_token(vktor_token type, void * value, int size, vktor_container nest) 
{
	char *str;
	
	switch(type) {
		case VKTOR_TOKEN_ARRAY_START:
			print_value("[", nest);
			indent++;
			first_token = 1;
			break;
			
		case VKTOR_TOKEN_MAP_START:
			print_value("{", nest);
			indent++;
			first_token = 1;
			break;
		
		case VKTOR_TOKEN_MAP_KEY:
			printf("%s\n", first_token ? "" : ",");
			print_indent(INDENT_STR);
			str = copy_string((char *) value, size);
			printf("%s => ", str);
			free(str);
			first_token = 0;
			break;
		
		case VKTOR_TOKEN_STRING:
			str = copy_string((char *) value, size);
			print_value("", nest);
			printf("\"%s\"", str);
			free(str);
			first_token = 0;
			break;
			
		case VKTOR_TOKEN_ARRAY_END:
			indent--;
			first_token = 1;
			print_value("]", nest);
			first_token = 0;
			break;
		
		case VKTOR_TOKEN_MAP_END:
			indent--;
			first_token = 1;
			print_value("}", VKTOR_CONTAINER_NONE);
			first_token = 0;
			break;
		
		case VKTOR_TOKEN_NULL:
			print_value("null", nest);
			first_token = 0;
			break;
		
		case VKTOR_TOKEN_TRUE:
			print_value("true", nest);
			first_token = 0;
			break;
		
		case VKTOR_TOKEN_FALSE:
			print_value("false", nest);
			first_token = 0;
			break;
			
		default:  // not yet handled stuff
			print_value(" -- unknown -- ", nest);
			first_token = 0;
			break;
	}
}

int 
main(int argc, char *argv[]) 
{
	vktor_parser    *parser;
	vktor_status     status;
	vktor_error     *error;
	vktor_container  nest;
	char            *buffer;
	size_t           read_bytes;
	int              done = 0;
		
	parser = vktor_parser_init(128);
	
	do {
		nest = vktor_get_current_container(parser);
		status = vktor_parse(parser, &error);
		
		switch (status) {
			
			case VKTOR_OK:
				// Print the token
				handle_token(parser->token_type, parser->token_value,
					parser->token_size, nest);
				break;
				
			case VKTOR_COMPLETE: 
				// Print the token
				handle_token(parser->token_type, parser->token_value, 
					parser->token_size, nest);
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
