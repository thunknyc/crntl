# CRNTL: C Reader for the Next Thousand Lisps

## Introduction

My attraction to Clojure back in '09 was based on several
things. First, there were some killer apps i.e. Incanter and
Cascalog. Second, I appreciated the pragmatism of making peace with
the JVM in order to benefit from its ecosystem. And third, there was
the Clojure reader.

As a long-time Schemer, I had a profound appreciation of the
language's support for list, vector, set, and dictionary literals --
and the way in which the language itself took advantage of its
enriched reader to improve the expressiveness and concision of its syntax.

There are other reasons to appreciate Clojure vis a vis other Lisps,
of course. There is its commitment to immutability and its wilingness
to cast off some of the historical absurdities of Lisp e.g. by
replacing `car` and `cdr` with `first` and `rest`.

But Clojure was not the first Lisp and it won't be the last Lisp, and,
embracing that spirit, I think it's important to capture the benefits
of the Clojure reader in a form that people will be able to build upon
as they move on from the Clojure ecosystem. I know I myself stayed too
long.

The goal of this project is to produce a parser not just for EDN but
for Clojure, meaning a parser that can understand syntax quoting and
metadata and derefencing and so forth. Ideally it will be able to
preserve comments and other surface syntax features, but I consider
such things "stretch goals."

CRNTL is written in C, still the lingua franc of contemporary
computing, and I will be writing the lexer by hand and (probably) using
Lemon to create the parser. I am focused on writing iOS apps at the
moment so I will eventually be putting effort into making sure that
this project plays nicely with Swift.

Finally, I do not intend to enshrine the limitations of the current
Clojure reader. I am open to evolving this project beyond of the
Clojure reader, assuming the changes are worthwhile and mostly
backwards compatible. At present, I am most open to a HEREDOC or
multiline string extension.

## Current status

Lexer works with limitations -- see issues.

INPUT: ```(def x ^:dynamic 42)```

Sample program output:
```
Start list at line 0, col 0
Symbol: def at line 0, col 3
Symbol: x at line 0, col 5
Meta at line 0, col 7
Keyword: dynamic at line 0, col 15
Integer: 42 at line 0, col 18
End list or func shortcut at line 0, col 19
```
