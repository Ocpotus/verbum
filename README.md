# Verbum
An EBNF-to-C parser generator.

## About
Verbum is an LR(1) parser generator aimed for rapid programming language development. It offers an easy to use interface for code generation and language interoperability.
It is not a parser generator for a particular language, rather it aims to be one for any programming language that allows interfacing with the C ABI.

## Getting Started
### Prerequisites
- [Forge](https://github.com/Ocpotus/forge) - Build system.

### Building
To build Verbum simply run the following:
```
$ forge build
```
### Running
To generate code Verbum needs to follow a strict EBNF grammar. Verbum accepts the following syntax as EBNF:
- `()`: grouping
- `[]`: optional
- `{}`: repeition
- `''`, `""`: literals
- `,`: concatenation
- `|`: alternation
- `a-zA-Z_` + `a-zA-Z0-9_`: identifier

Verbum expects a valid LR(1) grammar or at least a grammar with very little ambiguities. If the grammar is very ambiguous then Verbum will take _forever_ to run, so a valid or close to valid grammar should only take a few seconds to run. If you find yourself waiting a few minutes, then it is safe to assume the grammar will not pass.

## Current State
- Currently Verbum can generate LR(0) and LR(1) automata.
- Code generation is not available.
- The testing suite is very weak.

## Roadmap
- Early conflict checking to prevent long processing times. Essentially abort at _first_ conflict.
- Parse table construction.
- Matrix compression.
- Code generation.
- Testing suite.

## AI Generated Code Use Disclaimer
Some portions utilize a lot of AI generated code. Especially with complex algorithms (i.e. Lane Table). AI generated code must be understood and heavily scrutinized.
This project is also not too focused on algorithm development or research. Therefore contributors can focus more so on the software engineering aspect and less so on creating new cutting edge algorithms.

