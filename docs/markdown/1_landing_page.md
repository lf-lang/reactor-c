\mainpage

\note The reactor-c runtime system for Lingua Franca is the system used when you specify as your target `C`, `CCpp`, or `Python`. While it is theoretically possible to use reactor-c alone by writing applications in C, it is meant to be used via the [Lingua Franca coordination language](https://lf-lang.org) together with its code generator, `lfc`.

The [documentation here](https://lf-lang.org/reactor-c) is generated automatically from the C source code using [Doxygen](https://doxygen.nl).
Higher-level user documentation and getting-started information can be found on the
[lf-lang.org website](https://lf-lang.org/docs/next?target-languages=c),
with details about the C runtime on the
[target language details](https://lf-lang.org/docs/next/reference/target-language-details?target-languages=c)
page.

\note The most useful entry point to this documentation is likely the [topics page](topics.html). The [API](group__API.html) subpage describes the API used in writing reactors. The [Utilities](group__Utilities.html) subpage describes various utilities that may help with writing reactors.

## Overview

When you specify

```
target C
```
then a C compiler (such as `gcc`) will be used.
If you specify

```
target CCpp
```
then a C++ compiler (such as `g++`) will be used.
In this case, your reaction bodies can written in C++ and you can include and link to C++ code.
Unlike the Cpp target, you will still access the LF API through the C API documented here.

When you specify

```
target Python
```
then `lfc` will generate a Python program that uses this reactor-c runtime under the hood.

## Supported Platforms

The C, CCpp, and Python targets are tested nightly on Linux, macOS, and Windows (with WSL). The federated programs are tested on Linux and macOS only.

For embedded platforms, there is support for C on Arduino and Zephyr, plus bare-metal (unthreaded) execution on the Raspberry Pi RP2040, nRF52, FlexPRET, and Patmos. For bare-metal, Zephyr, and RIoT, consider using the [reactor-uc](https://github.com/lf-lang/reactor-uc) target instead.


