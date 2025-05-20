# CONTRIBUTING

> [!IMPORTANT]
> This document contains guidelines specific to this repository. Please also read the general [contributing guidelines](https://github.com/lf-lang/.github/blob/main/CONTRIBUTING.md) of the lf-lang organization.

This repository hosts the source code for the C runtime system for Lingua Franca, which is the runtime system you get when you specify C or CCpp as the target.
The C runtime system is tightly integrated with the Lingua Franca code generator, which is in the [lingua-franca](https://github.com/lf-lang/lingua-franca) repository.

## Workflow
All code contributions must go through a [pull request (PR)](https://github.com/lf-lang/.github/blob/main/CONTRIBUTING.md#pull-requests), pass all tests run in CI, and get an approving review before it is merged. Pushing to the `main` branch is restricted. All code review is conducted using the Github review system on PRs. Before requesting a code review, ensure that you have:

- applied the [code formatter](#code-style-and-formatting);
- [documented](#code-style-and-formatting) your code;
- written [tests](#tests) that cover your code; and
- accompanied any remaining `TODO`s or `FIXME`s with a link to an active issue.

## Tests

### System Tests
System tests are Lingua Franca programs that either compile and run to completion (the test passes) or fail to compile or exit the run with an error condition (returning non-zero).
System tests are defined in the [test/C directory](https://github.com/lf-lang/lingua-franca/tree/master/test/C) of the lingua-franca repo.

The Github Actions tests for this repo will automatically run all the C Lingua Franca tests with each of the available schedulers and with both the C and CCpp target specified. The version of the lingua-franca repo that is used to do this is specified by the [lingua-franca-ref.txt](lingua-franca-ref.txt) file in this directory.
Normally, this should point to `master`, but to run tests temporarily with a branch of the code generator, just change [lingua-franca-ref.txt](lingua-franca-ref.txt) to point to your branch.

To run the system tests locally on your machine, check out the lingua-franca repository and replace the reactor-c submodule with your branch:

```
git clone git@github.com:lf-lang/lingua-franca.git
git submodule update --init --recursive
cd lingua-franca/core/src/main/resources/lib/c/reactor-c/
git checkout <your-branch-name>
```

Then follow the [lingua-franca instructions for running tests](https://github.com/lf-lang/lingua-franca/blob/master/CONTRIBUTING.md).

### Unit Tests
To create a new unit test, write a C program with a file name ending in "test.c"
in a subdirectory of the `test` directory. That file should contain a main and should return 0 if the test succeeds.

* Tests in the `general` subdirectory will always be run.
* Tests in the `single-threaded` and `multithreaded` subdirectories will be run
depending on parameters passed to `cmake`.

To run tests for the single-threaded runtime, execute the following. Note that
`-U` is required to undefine a name that may be cached from a previous run.

- `rm -rf build && mkdir build && cd build`
- `cmake .. -UNUMBER_OF_WORKERS`
- `cmake --build .`
- `make test`

To run tests for the multithreaded runtime, provide a nonzero number of workers
when invoking `cmake`. For example:

- `cmake .. -DNUMBER_OF_WORKERS=2`
- `cmake --build .`
- `sudo make test`

Note that one of the tests in the multithreaded test suite requires sudo because
it changes the scheduling policy and priorities.

To define/undefine other preprocessor definitions such as `LOG_LEVEL`, pass them as
arguments to `cmake` in the same way as with `NUMBER_OF_WORKERS`, using the same
`-D`/`-U` prefixes.


## Code style and formatting
We use clang-format to format our codebase. To run the formatter on all source and header files in reactor-c:

```
make format
```
The CI will do a "dry-run" of the formatter to verify that all files are correctly formatted and will indicate errors if not.

VSCode can be configured to run clang-format on files as they are saved. To achieve this set the following settings:

- editor.formatOnSave: true
- C_Cpp.formatting: clang-format
- C_Cpp.clang_format_style: file

### General guidelines
- _Do not copy-paste code._ If you want to reuse code, factor it out into a method and call it.
- _Keep functions concise._ As a rule of thumb, a function should fit on your screen so that it can be read without scrolling. We impose no hard limit on method length, but anything above 40 lines should be considered for breaking up.
- _Do not leave FIXMEs_. Unaddressed `FIXME`s should not be allowed to pass code review unless they are accompanied with a link to a GitHub issue.

_Comments_

Please adhere to the following principles when writing documentation for your code:

- Use [Doxygen-style comments](https://www.doxygen.nl/manual/docblocks.html) with [Markdown](https://www.doxygen.nl/manual/markdown.html) in the comment bodies.
- Write descriptions in English.
- Do not use contractions like "aren't" or "isn't".
- Use imperative in the description of a function; i.e., write "Compute the shortest path," not "Computes the shortest path" (the latter is not a complete sentence).
- In `@param` JavaDoc tag descriptions it is OK to use incomplete sentences in the interest of brevity.
