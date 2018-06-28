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
   INTVAL, FLOATVAL, SYMBOLVAL, KEYWORDVAL,

   ENDOFINPUT, OUTOFMEMORY, ERROR
  };

struct Token {
  wchar_t *wcs;
  size_t wcs_length;
  enum TokenType type;
  
};

enum TokenizerReadState
  {
   START, INOCTO, INKEYWORD, INTILDE,
   INNUMERICVAL, INEXPONENT, INEXPONENTDIGITS,
   INSYMBOL, INTENTATIVESYMBOL
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

#define ALPHA							\
  'A': case 'B': case 'C': case 'D': case 'E':			\
  case 'F': case 'G': case 'H': case 'I': case 'J':		\
  case 'K': case 'L': case 'M': case 'N': case 'O':		\
  case 'P': case 'Q': case 'R': case 'S': case 'T':		\
  case 'U': case 'V': case 'W': case 'X': case 'Y':		\
  case 'Z':							\
  case 'a': case 'b': case 'c': case 'd': case 'e':		\
  case 'f': case 'g': case 'h': case 'i': case 'j':		\
  case 'k': case 'l': case 'm': case 'n': case 'o':		\
  case 'p': case 'q': case 'r': case 's': case 't':		\
  case 'u': case 'v': case 'w': case 'x': case 'y':		\
  case 'z'

#define NUMERIC						\
  '0': case '1': case '2': case '3': case '4':		\
  case '5': case '6': case '7': case '8': case '9'


void edntok_read(FILE *in,
		 struct Token *token,
		 struct TokenizerState *tokenizer_state) {
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
      case '#':	state = INOCTO; break;
      case '~': state = INTILDE; break;
      case ':': state = INKEYWORD; break;

      case '\n': tokenizer_state->line++; tokenizer_state->column = -1;
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
	state = INEXPONENTDIGITS;
	INSERT(token, wc);
	break;
	
      case NUMERIC:
	state = INEXPONENTDIGITS;
	INSERT(token, wc);
	break;
	
      default:
	tokenizer_state->column--;
	ungetwc(wc, in);
	INSERT(token, '\0');
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

int main(int argc, char **argv) {
  setlocale(LC_ALL, "en_US.UTF-8");
  struct TokenizerState ts;
  struct Token t;

  edntok_state_init(&ts);

  int done = 0;
  
  while (!done) {
    edntok_read(stdin, &t, &ts);
    switch (t.type) {
    case ERROR:
      fprintf(stderr, "ERROR"); done = 1; break;
    case ENDOFINPUT:
      fprintf(stderr, "ENDOFINPUT"); done = 1; break;
    case OUTOFMEMORY:
      fprintf(stderr, "OUTOFMEMORY"); done = 1; break;

    case INTVAL:
      fprintf(stderr, "Integer: "); fputws(t.wcs, stderr); break;
    case FLOATVAL:
      fprintf(stderr, "Float: "); fputws(t.wcs, stderr); break;
    case SYMBOLVAL:
      fprintf(stderr, "Symbol: "); fputws(t.wcs, stderr); break;
    case KEYWORDVAL:
      fprintf(stderr, "Keyword: "); fputws(t.wcs, stderr); break;

    case STARTLIST:
      fprintf(stderr, "Start list"); break;
    case ENDLIST:
      fprintf(stderr, "End list or func shortcut"); break;
    case STARTDICT:
      fprintf(stderr, "Start dict"); break;
    case ENDDICT:
      fprintf(stderr, "End dict or set"); break;
    case STARTSET:
      fprintf(stderr, "Start set"); break;
    case STARTFUNC:
      fprintf(stderr, "Start func shortcut"); break;
    case STARTVEC:
      fprintf(stderr, "Start vector"); break;
    case ENDVEC:
      fprintf(stderr, "End vector"); break;
    case VARQUOTE:
      fprintf(stderr, "Var quote"); break;
    case DEREF:
      fprintf(stderr, "Deref"); break;
    case HAT:
      fprintf(stderr, "Meta"); break;
    case QUOTE:
      fprintf(stderr, "Quote"); break;
    case QUASIQUOTE:
      fprintf(stderr, "Quasiquote"); break;
    case UNQUOTE:
      fprintf(stderr, "Unquote"); break;
    case UNQUOTESPLICE:
      fprintf(stderr, "Unquote-splice"); break;

    default:
      fprintf(stderr, "Something else of type: %d", t.type); break;
    }
    fprintf(stderr, " at line %d, col %d\n", ts.line, ts.column);
    edntok_freetok(t);
  }
}
