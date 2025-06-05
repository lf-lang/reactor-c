\page intro Introduction

\note These docs assumes familiarity with the Lingua Franca coordination langauge. Refer to
the [Lingua Franca Handbook](https://www.lf-lang.org/docs/) for an in-depth guide to
reactor-oriented programming.

## Getting started

First, [install Lingua Franca](https://www.lf-lang.org/docs/installation).
If you install this by cloning the [lingua-franca repository](https://github.com/lf-lang/lingua-franca), then the `reactor-c` repository will be installed as a git submodule at location
`core/src/main/resources/lib/c/reactor-c` within the `lingua-franca` clone.

The simplest possible C-target LF program looks like this:

```lf
target C
main reactor {
  reaction(startup) {=
    printf("Hello World!\n");
  =}
}
```

Put this in a file `HelloWorld.lf` in a directory called `src`.
Assuming that upon installing Lingua Franca, you arranged it so that `lfc` is in your `PATH`, then you can compile this program and run it as follows:

```shell
lfc src/HelloWorld.lf
bin/HelloWorld
```

When executed, the program should print `Hello World!` and terminate.
The `bin` directory is created automatically within the same directory that houses your `src` directory.

The `lfc` compiler will also create a directory `src-gen` that includes a standalone C program together with all required compiler infrastructure (cmake files, etc.).





