[![CI](https://github.com/lf-lang/reactor-c/actions/workflows/ci.yml/badge.svg)](https://github.com/lf-lang/reactor-c/actions/workflows/ci.yml)
[![API docs](https://github.com/lf-lang/reactor-c/actions/workflows/api-docs.yml/badge.svg)](https://github.com/lf-lang/reactor-c/actions/workflows/api-docs.yml)

# README

The reactor-c runtime system for Lingua Franca is the system used when you specify as your target `C` or `CCpp`.
The [documentation here](https://lf-lang.org/reactor-c) is generated automatically from the C source code using [Doxygen](https://doxygen.nl).
Higher-level user documentation and getting-started information can be found on the
[lf-lang.org website](https://lf-lang.org/docs/next?target-languages=c),
with details about the C runtime on the
[target language details](https://lf-lang.org/docs/next/reference/target-language-details?target-languages=c)
page.

When you specify

```
target C
```
then a C compiler (such as `gcc`) will be used.
If you specify

```
target CCpp
```
Then a C++ compiler (such as `g++`) will be used.
In this case, your reaction bodies can written in C++ and you can include and link to C++ code.
Unlike the Cpp target, you will still access the LF API through the C API documented here.


