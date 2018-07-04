# CRNTL: C Reader for the Next Thousand Lisps

## Current status

* No dependencies outside of C standard libraries
* Assumes UTF-8, newline line endings; `wchar_t` is used throughout
* Symbols and keywords must currently be ASCII
* Comments are parsed (and ignored)
* Tagged literals and discard-next-value (`#_`) are supported
* Each parsed value contains tokenizer state including line and column
* Parser produces (AFAIK) correct output on well-formed input
* Pretty decent error reporting
* A sample usage of the parser is in `main.c`; Makefile builds it as `crntl`
* On macOS, `crntl.o` builds to just under 20K (apologies for the bloat)
* There exists an [iOS framework](https://github.com/thunknyc/crntl-ios) for using CRNTL in Swift. It is currently being used in Thunk NYC Corp.'s computational notebook app:

<img src="https://user-images.githubusercontent.com/85875/42279473-e007ad36-7f6b-11e8-9930-6cf04b547178.jpg" width="321.714">

## Background

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
computing. I am focused on writing iOS apps at the moment so I will
eventually be putting effort into making sure that this project plays
nicely with Swift.

Finally, I do not intend to enshrine the limitations of the current
Clojure reader. I am open to evolving this project beyond of the
Clojure reader, assuming the changes are worthwhile and mostly
backwards compatible. At present, I am most open to a HEREDOC or
multiline string extension.

## Example

Given a file `test.txt` with contents

```
(def hilbert-rules {:L [:+ :R :F :- :L :F :L :- :F :R :+]
                    :R [:- :L :F :+ :R :F :R :+ :F :L :-]})

(defn produce-steps [rules start-steps iters]
  (loop [steps start-steps
         i iters]
    (if (zero? i)
      (filter #{:- :+ :F} steps)
      (recur (flatten (for [sym steps] (sym rules sym)))
             (dec i)))))
```

from [this
gist](https://gist.github.com/edw/ecc94abfef6cf50e161d0e1d639e34e4)
the command `./crntl < test.txt` produces the ouput

```
List value
.Symbol value: def
.Symbol value: hilbert-rules
.Dictionary value
..Entry:
...Keyword value: L
...Vect value
....Keyword value: +
....Keyword value: R
....Keyword value: F
....Keyword value: -
....Keyword value: L
....Keyword value: F
....Keyword value: L
....Keyword value: -
....Keyword value: F
....Keyword value: R
....Keyword value: +
..Entry:
...Keyword value: R
...Vect value
....Keyword value: -
....Keyword value: L
....Keyword value: F
....Keyword value: +
....Keyword value: R
....Keyword value: F
....Keyword value: R
....Keyword value: +
....Keyword value: F
....Keyword value: L
....Keyword value: -
List value
.Symbol value: defn
.Symbol value: produce-steps
.Vect value
..Symbol value: rules
..Symbol value: start-steps
..Symbol value: iters
.List value
..Symbol value: loop
..Vect value
...Symbol value: steps
...Symbol value: start-steps
...Symbol value: i
...Symbol value: iters
..List value
...Symbol value: if
...List value
....Symbol value: zero?
....Symbol value: i
...List value
....Symbol value: filter
....Set value
.....Keyword value: -
.....Keyword value: +
.....Keyword value: F
....Symbol value: steps
...List value
....Symbol value: recur
....List value
.....Symbol value: flatten
.....List value
......Symbol value: for
......Vect value
.......Symbol value: sym
.......Symbol value: steps
......List value
.......Symbol value: sym
.......Symbol value: rules
.......Symbol value: sym
....List value
.....Symbol value: dec
.....Symbol value: i
End of input
Gracefully exiting
```

.

