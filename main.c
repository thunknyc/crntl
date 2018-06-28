#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <locale.h>

enum TokenType
  {
   STARTLIST, ENDLIST, STARTDICT, ENDDICT,
   STARTSET, STARTFUNC, STARTVEC, ENDVEC,
   VARQUOTE, DEREF, HAT,
   QUOTE, QUASIQUOTE, UNQUOTE, UNQUOTESPLICE,
   INTVAL, SYMVAL, KEYWORDVAL, ENDOFINPUT, ERROR
  };

struct Token {
  wint_t *wcs;
  enum TokenType type;
};

enum TokenizerReadState
  {
   START, OCTO, INKEYWORD, TILDE
  };

struct TokenizerState {
  int line;
  int column;
};

void edntok_state_init(struct TokenizerState *tokenizer_state) {
  tokenizer_state->line = 0;
  tokenizer_state->column = -1;
}

void edntok_freetok(struct Token token) {
  free(token.wcs);
}

struct Token edntok_read(FILE *in, struct TokenizerState *tokenizer_state) {
  wint_t wc;
  enum TokenizerReadState state = START;
  struct Token token = { NULL, ERROR };
  
  while(1) {
    wc = getwc(in);
    tokenizer_state->column++;
    switch (state) {
    case START:
      switch (wc) {
      case '\n':
	tokenizer_state->line++;
	tokenizer_state->column = -1;
      case ' ': case '\t':
	// eat whitespace
	break;
      case '(':
	token.type = STARTLIST;
	return token;
      case ')':
	token.type = ENDLIST;
	return token;
      case '{':
	token.type = STARTDICT;
	return token;
      case '}':
	token.type = ENDDICT;
	return token;
      case '#':
	state = OCTO;
	break;
      case '[':
	token.type = STARTVEC;
	return token;
      case ']':
	token.type = ENDVEC;
	return token;
      case '@':
	token.type = DEREF;
	return token;
      case '\'':
	token.type = QUOTE;
	return token;
      case '`':
	token.type = QUASIQUOTE;
	return token;
      case '~':
	state = TILDE;
	break;
      case '^':
	token.type = HAT;
	return token;
      case ':':
	state = INKEYWORD;
	break;
      case WEOF:
	token.type = ENDOFINPUT;
	return token;
      }
      break;
    case OCTO:
      token.type = ERROR;
      return token;
    case INKEYWORD:
      token.type = ERROR;
      return token;
    case TILDE:
      token.type = ERROR;
      return token;
    }
  }
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, "en_US.UTF-8");
  struct TokenizerState ts;
  struct Token t;

  edntok_state_init(&ts);

  while (1) {
    t = edntok_read(stdin, &ts);
    fprintf(stderr, "t.type: %d ", t.type);
    fprintf(stderr, "at line %d, col %d\n", ts.line, ts.column);
    if (t.type == ERROR || t.type == ENDOFINPUT) {
      exit(1);
    }
  }
}
