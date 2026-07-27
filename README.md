# Verbum
An EBNF-to-C parser generator.

## About
The current vision for Verbum is to generate a bottom-up parser from a given EBNF grammar. As of now the focus is to generate a LALR parser;
other parsers may be experimented with in the future.

## Getting Started
### Prerequisites
- [Forge](https://github.com/Ocpotus/forge) - Build system.

### Building
To build Verbum simply run the following:
```
$ forge build
```
### Running
To generate code Verbum needs a valid EBNF grammar. Verbum accepts the following syntax as EBNF:
- `()`: grouping
- `[]`: optional
- `{}`: repeition
- `''`, `""`: literals
- `,`: concatenation
- `|`: alternation
- `a-zA-Z_` + `a-zA-Z0-9_`: identifier

### Testing
In it's current state simply run the binary. To test on different grammars modify the path in `src/main.c`.

## Current State
Currently Verbum can generate LR0 grammars, code generation has yet to be written.
