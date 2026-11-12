# Verbum
An EBNF parser generator for iterative language development.

## About
Verbum is an LR(1) parser generator for iterative programming language development. It offers an easy to use interface for code generation and language interoperability.
It is not a parser generator for a particular language, rather it aims to be one for any programming language that allows interfacing with the C ABI.

### Why Verbum?
At first glance Verbum may seem like another parser generator in a sea of parser generators. However, Verbum can be set apart by others by analyzing the following goals:
- Embeddable: Verbum **must** produce simple, efficient, and dependency free code.*
- Ease of use: Verbum is ergonomic. Meaning that from language design to integration developers should have very little overhead when working with Verbum.
- Language agnostic: Verbum is designed to be used within any software stack. Bindings are to be provided.

Many parser generators are developed/engineered with a particular programming language in mind; however, this does not need to be the case.
C was deliberately chosen to develop Verbum as it is the lingua franca of software engineering. Many languages provide C interoperability (i.e. C++, Go, Rust).

*Currently Verbum relies on the standard library and some third party libraries for the code generation. Though these arguably maintain the dependency free characteristic.

## Getting Started
### Prerequisites
- [Forge](https://github.com/Ocpotus/forge) - Build system.

### Building
To build Verbum simply run the following:
```
$ forge build # debug
$ forge build --release # release
```
### Running
Running the following will produce a `verbum.c` file containing the generated code:
```
@ verbum generate <path>
```

### Accepted syntax
To generate code Verbum needs to follow a strict EBNF grammar. Verbum accepts the following syntax as EBNF:
- `()`: grouping
- `[]`: optional
- `{}`: repetition
- `''`, `""`: literals
- `,`: concatenation
- `|`: alternation
- `a-zA-Z_` + `a-zA-Z0-9_`: identifier
- `=`: assignment, a syntax rule
- `:=`: assignment, a lexical rule
- `;`: end of rule

Verbum expects a valid LR(1) grammar or at least a grammar with very little ambiguities. If the grammar is very ambiguous then Verbum will take _forever_ to run, so a valid or close to valid grammar should only take a few seconds to run. If you find yourself waiting a few minutes, then it is safe to assume the grammar is not LR(1) and will have to be redisgned.

## Current State
- Generates
- Currently Verbum can generate LR(0) and LR(1) automata.
- Code generation is not available.
- The testing suite is very weak.
## Roadmap
- [x] Lexer generation
- [x] Parser generation
- [x] AST generation
- [ ] Better CLI interface
- [ ] Generated code interface
- [ ] Error reporting
- [ ] Extended EBNF syntax (annotations)
- [ ] Context injection
- [ ] Matrix compression
- [ ] Multi-threading
- [ ] Testing suite
- [ ] Extended code generation (i.e. specifying types)
- [ ] Example bindings

## AI Generated Code Use Disclaimer
Some portions utilize a lot of AI generated code. Especially with complex algorithms (i.e. Lane Table). AI generated code must be understood and heavily scrutinized.
This project is also not too focused on algorithm development or research. Therefore contributors can focus more so on the software engineering aspect and less so on creating new cutting edge algorithms.

