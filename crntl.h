#ifndef __CRNTL__
#define __CRNTL__

/*

  Copyright 2018 Thunk NYC Corp.

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

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
   STRINGVAL, CHARVAL, TAGVAL, IGNOREDVAL,

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
   PRIMITIVE_VALUE,

   LIST_VALUE, SET_VALUE, VECT_VALUE, DICT_VALUE, FUNC_VALUE, 

   DEREF_VALUE, QUOTE_VALUE, QUASIQUOTE_VALUE, UNQUOTE_VALUE,
   UNQUOTESPLICE_VALUE, META_VALUE, VARQUOTE_VALUE,

   TAGGED_VALUE,

   ERROR_VALUE, END_VALUE
  };

struct ParserValue {
  enum ParserValueType type;
  union {
    struct Token token;
    struct ParserValue *boxed_value;
    struct ParserSequenceItem *head_item;
    struct error {
      wchar_t *string;
      int needs_free;
    } error;
    struct {
      struct ParserValue *tag;
      struct ParserValue *value;
    } tagged;
  } content;
  struct TokenizerState state;
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
void crntl_freetok(struct Token *token);

void crntl_read(FILE *in, struct ParserValue *v,
                struct TokenizerState *ts);

void crntl_freevalue(struct ParserValue *v);

#endif /* __CRNTL__ */
