#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include "crntl.h"

void pad(FILE *out, int depth) {
  for(int i = 0; i < depth; i++) fputc(' ', out);
}

#define DUMPPRIMITIVE(LABEL)			\
  {						\
    fprintf(out, LABEL " value: ");		\
    fputws(v->content.token.wcs, out);		\
    fputs("\n", out);				\
  }  

#define DUMPSEQUENCE(LABEL)						\
  {									\
    fprintf(out, LABEL " value\n");					\
    for(struct ParserSequenceItem *elem = v->content.head_item;		\
	elem != NULL;							\
	elem = elem->next) {						\
      dumpval1(out, &elem->value.i, depth + 1);				\
    }									\
  }

#define DUMPBOX(LABEL)							\
  {									\
    fprintf(out, LABEL " value\n");					\
    for(struct ParserSequenceItem *elem = v->content.head_item;		\
	elem != NULL;							\
	elem = elem->next) {						\
      dumpval1(out, &elem->value.i, depth + 1);				\
    }									\
  }


#define DUMPDICT()							\
  {									\
    fprintf(out, "Dictionary value\n");					\
    for(struct ParserSequenceItem *elem = v->content.head_item;		\
	elem != NULL;							\
	elem = elem->next) {						\
      pad(out, depth);							\
      fprintf(out, " Entry:\n");					\
      dumpval1(out, &elem->value.key_entry.k, depth + 2);		\
      pad(out, depth);							\
      dumpval1(out, &elem->value.key_entry.v, depth + 2);		\
    }									\
 }									


void dumpval1(FILE *out, struct ParserValue *v, int depth) {
  pad(out, depth);
  switch (v->type) {
    
  case PRIMITIVE_VALUE:
    switch (v->content.token.type) {
    case INTVAL: DUMPPRIMITIVE("Integer"); break;
    case FLOATVAL: DUMPPRIMITIVE("Float"); break;
    case SYMBOLVAL: DUMPPRIMITIVE("Symbol"); break;
    case KEYWORDVAL: DUMPPRIMITIVE("Keyword"); break;
      
    default: DUMPPRIMITIVE("Unknown"); break;	
    }
    break;
    
  case LIST_VALUE: DUMPSEQUENCE("List"); break;
  case SET_VALUE: DUMPSEQUENCE("Set"); break;
  case FUNC_VALUE: DUMPSEQUENCE("Function"); break;
  case VECT_VALUE: DUMPSEQUENCE("Vect"); break;
    
  case DICT_VALUE: DUMPDICT(); break;      
    
  case DEREF_VALUE: case QUASIQUOTE_VALUE:
  case UNQUOTE_VALUE: case UNQUOTESPLICE_VALUE:
  case META_VALUE: case VARQUOTE_VALUE: case QUOTE_VALUE: 
    fprintf(out, "Boxed value\n");
    break;      
    
  case ERROR_VALUE:
    fprintf(out, "Error: %s\n", v->content.error_string);
    break;
    
  case END_VALUE:
    fprintf(out, "End of input\n");
      break;
  }
}

void dumpval(FILE *out, struct ParserValue *v) {
  dumpval1(out, v, 0);
}
  
int main(int argc, char **argv) {
  setlocale(LC_ALL, "en_US.UTF-8");
  struct TokenizerState ts;
  struct ParserValue value;
  int done = 0;
  int exit_status = 1;
  
  crntl_state_init(&ts);

  while(!done) {
    crntl_read(stdin, &value, &ts);

    dumpval(stderr, &value);
    
    switch (value.type) {

    case ERROR_VALUE:
      done = 1;
      break;

    case END_VALUE:
      fprintf(stderr, "Gracefully exiting\n");
      done = 1;
      exit_status = 0;
      break;

    default:
      break;
    }
    crntl_freevalue(&value);
  }
  return exit_status;
}
