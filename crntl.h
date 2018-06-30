#ifndef __CRNTL__
#define __CRNTL__

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>

enum TokenType
  {
   STARTLIST, ENDLIST, STARTDICT, ENDDICT,
   STARTSET, STARTFUNC, STARTVEC, ENDVEC,
   VARQUOTE, DEREF, HAT,
   QUOTE, QUASIQUOTE, UNQUOTE, UNQUOTESPLICE,
   INTVAL, FLOATVAL, SYMBOLVAL, KEYWORDVAL,

   ENDOFINPUT, OUTOFMEMORY, ERROR
  };

struct Token {
  wchar_t *wcs;
  size_t wcs_length;
  enum TokenType type;
};

struct TokenizerState {
  int line;
  int column;
  int is_token_saved;
  struct Token token;
};

enum ParserValueType
  {
   PRIMITIVE_VALUE, LIST_VALUE, SET_VALUE, VECT_VALUE,
   DICT_VALUE, DEREF_VALUE, QUOTE_VALUE,
   QUASIQUOTE_VALUE, UNQUOTE_VALUE, UNQUOTESPLICE_VALUE,
   META_VALUE, VARQUOTE_VALUE, FUNC_VALUE,
   ERROR_VALUE, END_VALUE
  };

struct ParserValue {
  enum ParserValueType type;
  union {
    struct Token token;
    struct ParserValue *boxed_value;
    struct ParserSequenceItem *head_item;
    char *error_string;
  } content;
};

struct ParserSequenceItem {
  union {
    struct ParserValue i;
    struct {
      struct ParserValue k;
      struct ParserValue v;
    } key_entry;
  } value;
  struct ParserSequenceItem *next;
};

void crntl_state_init(struct TokenizerState *tokenizer_state);
void crntl_freetok(struct Token token);

void crntl_read(FILE *in, struct ParserValue *v,
		struct TokenizerState *ts);

void crntl_freevalue(struct ParserValue *v);

#endif /* __CRNTL__ */
