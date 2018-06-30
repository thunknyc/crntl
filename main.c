#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include "crntl.h"

int main(int argc, char **argv) {
  setlocale(LC_ALL, "en_US.UTF-8");
  struct TokenizerState ts;
  struct ParserValue value;
  int done = 0;
  int exit_status = 1;
  
  crntl_state_init(&ts);

  while(!done) {
    crntl_read(stdin, &value, &ts);
    switch (value.type) {

    case PRIMITIVE_VALUE:
      switch (value.content.token.type) {
      default:
	fprintf(stderr, "Primitive value\n");
	break;	
      }
      break;

    case LIST_VALUE: case SET_VALUE: case FUNC_VALUE: case VECT_VALUE:
      fprintf(stderr, "Sequence value\n");
      break;      

    case DICT_VALUE:
      fprintf(stderr, "Dict sequence value\n");
      break;      
      
    case DEREF_VALUE: case QUASIQUOTE_VALUE:
    case UNQUOTE_VALUE: case UNQUOTESPLICE_VALUE:
    case META_VALUE: case VARQUOTE_VALUE: case QUOTE_VALUE: 
      fprintf(stderr, "Boxed value\n");
      break;      

    case ERROR_VALUE:
      fprintf(stderr, "Error: %s\n", value.content.error_string);
      done = 1;
      break;

    case END_VALUE:
      fprintf(stderr, "End of input gracefully reached\n");
      done = 1;
      exit_status = 0;
      break;
    }
    crntl_freevalue(&value);
  }
  return exit_status;
}
