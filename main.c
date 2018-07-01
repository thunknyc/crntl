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
#include "crntl.h"

void pad(FILE *out, int depth) {
  for(int i = 0; i < depth; i++) fputc('.', out);
}

#define DUMPPRIMITIVE(LABEL)                    \
  {                                             \
    fprintf(out, LABEL " value: ");             \
    fputws(v->content.token.wcs, out);          \
    fputs("\n", out);                           \
  }

#define DUMPSEQUENCE(LABEL)                                             \
  {                                                                     \
    fprintf(out, LABEL " value\n");                                     \
    for(struct ParserSequenceItem *elem = v->content.head_item;         \
        elem != NULL;                                                   \
        elem = elem->next) {                                            \
      dumpval1(out, &elem->value.i, depth + 1);                         \
    }                                                                   \
  }

#define DUMPBOX(LABEL)                                                  \
  {                                                                     \
    fprintf(out, LABEL " value\n");                                     \
    dumpval1(out, v->content.boxed_value, depth + 1);                   \
  }


#define DUMPDICT()                                                      \
  {                                                                     \
    fprintf(out, "Dictionary value\n");                                 \
    for(struct ParserSequenceItem *elem = v->content.head_item;         \
        elem != NULL;                                                   \
        elem = elem->next) {                                            \
      pad(out, depth);                                                  \
      fprintf(out, ".Entry:\n");                                        \
      dumpval1(out, &elem->value.key_entry.k, depth + 2);               \
      dumpval1(out, &elem->value.key_entry.v, depth + 2);               \
    }                                                                   \
  }

#define DUMPTAGGED()                                                    \
  {                                                                     \
    fprintf(out, "Tagged value\n");                                     \
    dumpval1(out, v->content.tagged.tag, depth + 1);                    \
    dumpval1(out, v->content.tagged.value, depth + 1);                  \
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
    case STRINGVAL: DUMPPRIMITIVE("String"); break;
    case CHARVAL: DUMPPRIMITIVE("Character"); break;

    default: DUMPPRIMITIVE("Unknown"); break;
    }
    break;

  case LIST_VALUE: DUMPSEQUENCE("List"); break;
  case SET_VALUE: DUMPSEQUENCE("Set"); break;
  case FUNC_VALUE: DUMPSEQUENCE("Function"); break;
  case VECT_VALUE: DUMPSEQUENCE("Vect"); break;

  case DICT_VALUE: DUMPDICT(); break;

  case TAGGED_VALUE: DUMPTAGGED(); break;

  case DEREF_VALUE: DUMPBOX("Deref"); break;
  case QUASIQUOTE_VALUE: DUMPBOX("Quasiquote"); break;
  case UNQUOTE_VALUE: DUMPBOX("Unquote"); break;
  case UNQUOTESPLICE_VALUE: DUMPBOX("Unquote splice"); break;
  case META_VALUE: DUMPBOX("Meta"); break;
  case VARQUOTE_VALUE: DUMPBOX("Varquote"); break;
  case QUOTE_VALUE: DUMPBOX("Quote"); break;

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
