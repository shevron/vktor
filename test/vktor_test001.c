#include <stdio.h>
#include <stdlib.h>
#include <vktor.h>

#define MAX_BUFFSIZE 64

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
				printf("Token: %d\n", parser->token_type);
				// Do something
				break;
				
			case VKTOR_COMPLETE: 
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
