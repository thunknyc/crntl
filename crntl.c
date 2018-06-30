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

#include <assert.h>
#include <string.h>

#include "crntl.h"

enum TokenizerReadState
  {
   START, INOCTO, INKEYWORD, INTILDE,
   INNUMERICVAL, INEXPONENT, INEXPONENTFIRSTDIGIT,
   INEXPONENTDIGITS, INSYMBOL, INTENTATIVESYMBOL,
   INSTRING, INSTRINGESCAPE, INCHAR, INCHAR2
  };

void crntl_state_init(struct TokenizerState *tokenizer_state) {
  tokenizer_state->line = 0;
  tokenizer_state->column = -1;
}

void crntl_freetok(struct Token token) {
  free(token.wcs);
  token.wcs = NULL;
  token.wcs_length = 0;
}

#define INSERT(TOKPTR, WC)						\
  { TOKPTR->wcs = realloc(TOKPTR->wcs,					\
			  sizeof(wchar_t) * (TOKPTR->wcs_length + 1));	\
    if (TOKPTR->wcs == NULL) {						\
      TOKPTR->type = OUTOFMEMORY;					\
      TOKPTR->wcs = 0;							\
      return;								\
    }									\
    TOKPTR->wcs[TOKPTR->wcs_length++] = WC;				\
  }

#define WHITESPACE					\
  ' ': case '\t': case '\n': case '\f': case ','

#define ALPHA						\
  'A': case 'B': case 'C': case 'D': case 'E':		\
  case 'F': case 'G': case 'H': case 'I': case 'J':	\
  case 'K': case 'L': case 'M': case 'N': case 'O':	\
  case 'P': case 'Q': case 'R': case 'S': case 'T':	\
  case 'U': case 'V': case 'W': case 'X': case 'Y':	\
  case 'Z':						\
  case 'a': case 'b': case 'c': case 'd': case 'e':	\
  case 'f': case 'g': case 'h': case 'i': case 'j':	\
  case 'k': case 'l': case 'm': case 'n': case 'o':	\
  case 'p': case 'q': case 'r': case 's': case 't':	\
  case 'u': case 'v': case 'w': case 'x': case 'y':	\
  case 'z'

#define NUMERIC						\
  '0': case '1': case '2': case '3': case '4':			\
  case '5': case '6': case '7': case '8': case '9'


void crntl_ungettok(struct Token *token,
		    struct TokenizerState *tokenizer_state) {
  assert(!tokenizer_state->is_token_saved);
  tokenizer_state->is_token_saved = 1;
  tokenizer_state->token = *token;
  token->type = ERROR;
  token->wcs = NULL;
  token->wcs_length = 0;
}

void crntl_gettok(FILE *in,
		  struct Token *token,
		  struct TokenizerState *tokenizer_state) {

  if (tokenizer_state->is_token_saved) {
    tokenizer_state->is_token_saved = 0;
    *token = tokenizer_state->token;
    return;
  }

  wint_t wc;
  enum TokenizerReadState state = START;

  token->type = ERROR;
  token->wcs = NULL;
  token->wcs_length = 0;

  while(1) {
    wc = getwc(in);
    tokenizer_state->column++;
    switch (state) {
    case START:
      switch (wc) {
      case WEOF: token->type = ENDOFINPUT; return;
      case '\\': state = INCHAR; token->type = CHARVAL; break;
      case '"': state = INSTRING; token->type = STRINGVAL; break;
      case '#': state = INOCTO; break;
      case '~': state = INTILDE; break;
      case ':': state = INKEYWORD; break;

      case '\n': case '\f':
	tokenizer_state->line++; tokenizer_state->column = -1;
	// fall through and...
      case ' ': case '\t': case ',': break; // ..eat whitespace

      case '(': token->type = STARTLIST; return;
      case ')': token->type = ENDLIST; return;
      case '{': token->type = STARTDICT; return;
      case '}': token->type = ENDDICT; return;
      case '[': token->type = STARTVEC; return;
      case ']': token->type = ENDVEC; return;
      case '@': token->type = DEREF; return;
      case '\'': token->type = QUOTE; return;
      case '`': token->type = QUASIQUOTE; return;
      case '^': token->type = HAT; return;

      case ALPHA:
      case '*': case '!': case '_': case '?': case '$':
      case '%': case '&': case '=': case '<': case '>':
	token->type = SYMBOLVAL;
	state = INSYMBOL;
	INSERT(token, wc);
	break;

      case '+': case '-': case '.':
	token->type = SYMBOLVAL;
	INSERT(token, wc);
	state = INTENTATIVESYMBOL;
	break;

      case NUMERIC:
	token->type = INTVAL;
	state = INNUMERICVAL;
	INSERT(token, wc);
	break;

      default:
	token->type = ERROR;
	return;
      }
      break;

    case INOCTO:
      switch (wc) {
      case '{': token->type = STARTSET; return;
      case '(':	token->type = STARTFUNC; return;
      case '\'': token->type = VARQUOTE; return;
      default: token->type = ERROR; return;
      }

    case INTILDE:
      switch (wc) {
      case '@': token->type = UNQUOTESPLICE; return;
      default:
	tokenizer_state->column--;
	ungetwc(wc, in);
	token->type = UNQUOTE;
	return;
      }
      break;

    case INSTRING:
      switch (wc) {
      case '"':
	INSERT(token, '\0');
	return;
      case '\\':
	state = INSTRINGESCAPE;
	INSERT(token, '\\');
	break;
      default:
	INSERT(token, wc);
	break;
      }
      break;

    case INSTRINGESCAPE:
      switch (wc) {
      case '"': case '\\':
	state = INSTRING;
	INSERT(token, wc);
	break;
      default:
	// Need to sort out legal from illegal escape chars.
	state = INSTRING;
	INSERT(token, wc);
	break;
      }
      break;

    case INKEYWORD:
      switch (wc) {
      case ALPHA:
      case '*': case '!': case '_': case '?': case '$':
      case '%': case '&': case '=': case '<': case '>':
	token->type = KEYWORDVAL;
	state = INSYMBOL;
	INSERT(token, wc);
	break;

      case '+': case '-': case '.':
	token->type = KEYWORDVAL;
	state = INTENTATIVESYMBOL;
	INSERT(token, wc);
	break;
      }
      break;

    case INNUMERICVAL:
      switch (wc) {
      case NUMERIC: INSERT(token, wc); break;

      case '.':
	// Promote to float
	token->type = FLOATVAL;
	INSERT(token, wc);
	break;

      case 'e': case 'E':
	// Promote to float
	token->type = FLOATVAL;
	state = INEXPONENT;
	INSERT(token, wc);
	break;

      default:
	tokenizer_state->column--;
	ungetwc(wc, in);
	INSERT(token, '\0');
	return;
      }
      break;

    case INEXPONENT:
      switch (wc) {
      case '-': case '+':
	state = INEXPONENTFIRSTDIGIT;
	INSERT(token, wc);
	break;

      case NUMERIC:
	state = INEXPONENTDIGITS;
	INSERT(token, wc);
	break;

      default:
	token->type = ERROR;
	free(token->wcs);
	token->wcs = NULL;
	token->wcs_length = 0;
	return;
      }
      break;

    case INEXPONENTFIRSTDIGIT:
      switch (wc) {
      case NUMERIC:
	state = INEXPONENTDIGITS;
	INSERT(token, wc);
	break;
      default:
	token->type = ERROR;
	free(token->wcs);
	token->wcs = NULL;
	token->wcs_length = 0;
	return;
      }
      break;

    case INEXPONENTDIGITS:
      switch (wc) {
      case NUMERIC:
	INSERT(token, wc);
	break;

      default:
	tokenizer_state->column--;
	ungetwc(wc, in);
	INSERT(token, '\0');
	return;
      }
      break;

    case INCHAR:
      switch (wc) {
      case ' ': case '\t': case '\n': case '\f':
	token->type = ERROR;
	free(token->wcs);
	token->wcs = NULL;
	token->wcs_length = 0;
	return;
      default:
	state = INCHAR2;
	INSERT(token, wc);
	break;
      }
      break;

    case INCHAR2:
      switch (wc) {
      case WHITESPACE:
	tokenizer_state->column--;
	ungetwc(wc, in);
	INSERT(token, '\0');
	return;
      case ALPHA:
	INSERT(token, wc);
	break;
      default:
	tokenizer_state->column--;
	ungetwc(wc, in);
	INSERT(token, '\0');
	return;
      }
      break;

    case INSYMBOL:
      switch (wc) {
      case ALPHA: case NUMERIC:
      case '*': case '!': case '_': case '?': case '$':
      case '%': case '&': case '=': case '<': case '>':
      case '+': case '-': case '.': case ':':
	INSERT(token, wc);
	break;

      default:
	tokenizer_state->column--;
	ungetwc(wc, in);
	INSERT(token, '\0');
	return;
      }
      break;
    case INTENTATIVESYMBOL:
      switch (wc) {
      case ALPHA:
      case '*': case '!': case '_': case '?': case '$':
      case '%': case '&': case '=': case '<': case '>':
      case '+': case '-': case '.': case ':':
	state = INSYMBOL;
	INSERT(token, wc);
	break;

      case NUMERIC:
	// Oops, no, we're really in a number
	if (token->type == KEYWORDVAL) {
	  // If we're in a keyword we can't change
	  // horses mid-stream.
	  token->type = ERROR;
	  free(token->wcs);
	  token->wcs = NULL;
	  token->wcs_length = 0;
	  return;
	}

	token->type = token->wcs[0] == '.' ? FLOATVAL : INTVAL;
	state = INNUMERICVAL;
	INSERT(token, wc);
	break;

      default:
	tokenizer_state->column--;
	ungetwc(wc, in);
	INSERT(token, '\0');
	return;
      }
    }
  }
}

#define PARSE_ERROR(V, S)			\
  {						\
    V->type = ERROR_VALUE;			\
    V->content.error_string = S;		\
  }


void crntl_freevalue(struct ParserValue *v) {
  switch (v->type) {
  case ERROR_VALUE:
    // free(v->content.error_string); All error strings are static.
    break;

  case PRIMITIVE_VALUE:
    crntl_freetok(v->content.token);
    break;

  case LIST_VALUE: case VECT_VALUE: case SET_VALUE: case FUNC_VALUE:
    {
      struct ParserSequenceItem *head = v->content.head_item;
      while (head != NULL) {
	crntl_freevalue(&head->value.i);
	head = head->next;
      }
    }
    break;

  case DICT_VALUE:
    {
      struct ParserSequenceItem *head = v->content.head_item;
      while (head != NULL) {
	crntl_freevalue(&head->value.key_entry.k);
	crntl_freevalue(&head->value.key_entry.v);
	head = head->next;
      }
    }
    break;

  case DEREF_VALUE: case QUOTE_VALUE: case QUASIQUOTE_VALUE:
  case UNQUOTE_VALUE: case UNQUOTESPLICE_VALUE: case META_VALUE:
  case VARQUOTE_VALUE:
    crntl_freevalue(v->content.boxed_value);
    free(v->content.boxed_value);
    break;

  case END_VALUE:
    break;
  }
}

void crntl_read_list(FILE *in,
		     struct ParserValue *v,
		     struct TokenizerState *ts,
		     enum TokenType end_type) {

  struct ParserSequenceItem *parent = NULL;
  struct ParserSequenceItem *tail = malloc(sizeof(struct ParserSequenceItem));

  v->content.head_item = tail;

  struct Token t;
  while (1) {

    if (tail == NULL) {
      crntl_freevalue(v);
      v->type = ERROR_VALUE;
      v->content.error_string = "Parser out of memory";
      return;
    }

    tail->next = NULL;

    crntl_read(in, &tail->value.i, ts);

    if (tail->value.i.type == ERROR_VALUE) {
      crntl_gettok(in, &t, ts);
      if (t.type != end_type) {
	crntl_freevalue(v);
	v->type = ERROR_VALUE;
	v->content.error_string = "Unexpected token";
      } else if (parent != NULL) {
	free(parent->next);
	parent->next = NULL;
      } else {
	free(v->content.head_item);
	v->content.head_item = NULL;
      }
      return;
    }

    tail->next = malloc(sizeof(struct ParserSequenceItem));
    parent = tail;
    tail = tail->next;
  }
}

void crntl_read_dict(FILE *in,
		     struct ParserValue *v,
		     struct TokenizerState *ts) {

  struct ParserSequenceItem *parent = NULL;
  struct ParserSequenceItem *tail = malloc(sizeof(struct ParserSequenceItem));
  v->content.head_item = tail;

  struct Token t;
  while (1) {

    if (tail == NULL) {
      crntl_freevalue(v);
      v->type = ERROR_VALUE;
      v->content.error_string = "Parser out of memory";
      return;
    }

    tail->next = NULL;

    crntl_read(in, &tail->value.key_entry.k, ts);

    if (tail->value.key_entry.k.type == ERROR_VALUE) {
      crntl_gettok(in, &t, ts);
      if (t.type != ENDDICT) {
	crntl_freevalue(v);
	v->type = ERROR_VALUE;
	v->content.error_string = "Unexpected token while looking for key";
      } else if (parent != NULL) {
	free(parent->next);
	parent->next = NULL;
      } else {
	free(v->content.head_item);
	v->content.head_item = NULL;
      }
      return;
    }

    crntl_read(in, &tail->value.key_entry.v, ts);

    if (tail->value.key_entry.v.type == ERROR_VALUE) {
      crntl_freevalue(v);
      v->type = ERROR_VALUE;
      v->content.error_string = "Unexpected token while looking for value";
      return;
    }

    tail->next = malloc(sizeof(struct ParserSequenceItem));
    parent = tail;
    tail = tail->next;
  }
}

void crntl_read_box(FILE *in,
		    struct ParserValue *v,
		    struct TokenizerState *ts) {
  v->content.boxed_value = malloc(sizeof(struct ParserValue));
  if (v->content.boxed_value == NULL) {
      v->type = ERROR_VALUE;
      v->content.error_string = "Parser out of memory";
      return;
  }

  crntl_read(in, v->content.boxed_value, ts);

  if (v->content.boxed_value->type == ERROR_VALUE) {
    free(v->content.boxed_value);
    v->type = ERROR_VALUE;
    v->content.error_string = "Unexpected token";
  }
}

void crntl_read(FILE *in,
		struct ParserValue *v,
		struct TokenizerState *ts) {

  struct Token t;
  struct ParserSequenceItem **current = &(v->content.head_item);

  v->content.head_item = NULL;

  crntl_gettok(in, &t, ts);

  switch (t.type) {

  case ERROR:
    PARSE_ERROR(v, "Lexer error"); break;
  case OUTOFMEMORY:
    PARSE_ERROR(v, "Lexer out of memory"); break;

  case ENDOFINPUT:
    v->type = END_VALUE; break;

  case INTVAL: case FLOATVAL: case SYMBOLVAL: case KEYWORDVAL:
  case STRINGVAL: case CHARVAL:
    v->type = PRIMITIVE_VALUE;
    v->content.token = t;
    break;

  case STARTLIST:
    v->type = LIST_VALUE;
    crntl_read_list(in, v, ts, ENDLIST);
    break;
  case STARTSET:
    v->type = SET_VALUE;
    crntl_read_list(in, v, ts, ENDDICT);
    break;
  case STARTVEC:
    v->type = VECT_VALUE;
    crntl_read_list(in, v, ts, ENDVEC);
    break;
  case STARTFUNC:
    crntl_read_list(in, v, ts, ENDLIST);
    v->type = FUNC_VALUE;
    break;

  case STARTDICT:
    v->type = DICT_VALUE;
    crntl_read_dict(in, v, ts);
    break;

  case VARQUOTE:
    v->type = VARQUOTE_VALUE;
    crntl_read_box(in, v, ts);
    break;
  case DEREF:
    v->type = DEREF_VALUE;
    crntl_read_box(in, v, ts);
    break;
  case HAT:
    v->type = META_VALUE;
    crntl_read_box(in, v, ts);
    break;
  case QUOTE:
    v->type = QUOTE_VALUE;
    crntl_read_box(in, v, ts);
    break;
  case QUASIQUOTE:
    v->type = QUASIQUOTE_VALUE;
    crntl_read_box(in, v, ts);
    break;
  case UNQUOTE:
    v->type = UNQUOTE_VALUE;
    crntl_read_box(in, v, ts);
    break;
  case UNQUOTESPLICE:
    v->type = UNQUOTESPLICE_VALUE;
    crntl_read_box(in, v, ts);
    break;

  default:
    crntl_ungettok(&t, ts);
    PARSE_ERROR(v, "Unexpected token"); break;
  }
}
