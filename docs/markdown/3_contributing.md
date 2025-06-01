\page contributing Contributing

Contributions to `reactor-c` are welcome as long as they conform with the programming style, include tests, and can be released open-source with [BSD licence](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md).

\note This document contains guidelines specific to this repository. Please also read the general [contributing guidelines](https://github.com/lf-lang/.github/blob/main/CONTRIBUTING.md) of the lf-lang organization.

## Workflow
All code contributions must go through a [pull request (PR)](https://github.com/lf-lang/.github/blob/main/CONTRIBUTING.md#pull-requests), pass all tests run in CI, and get an approving review before it is merged. Pushing to the `main` branch is restricted. All code review is conducted using the Github review system on PRs. Before requesting a code review, ensure that you have:

- applied the \ref code-style-and-formatting "code formatter";
- \ref docs "documented" your code;
- written \ref tests "tests" that cover your code; and
- accompanied any remaining `TODO`s or `FIXME`s with a link to an active issue.

\anchor docs
## Documentation

This repository uses [Doxygen](https://doxygen.org) to generate online documentation from source files. It uses [Javadoc](https://en.wikipedia.org/wiki/Javadoc)-style documentation with [Markdown](https://en.wikipedia.org/wiki/Markdown) supported in the comment bodies.

To build the documentation on your local machine, in a clone of the [reactor-c repository](https://github.com/lf-lang/reactor-c), simply run

```
make docs
```
(Running `make` with no arguments will list available targets.)

The documentation conventions are:

1. Functions are documented in `.h` files. Documentation appears in `.c` files only if there is no declaration for the function in a `.h` file. Do not duplicate the documentation in both places.
2. Not all files are included in the Doxygen generation. Which files are included is defined by the `INPUT` variable in [docs/Doxyfile.in](https://github.com/lf-lang/reactor-c/blob/main/docs/Doxyfile.in).
3. Each source file include in the documentation should begin with documentation having the following form:
   
    ```
    /**
     * @file <filename>
     * @author <first and last name, no email>
     * @author <second author ...>
     * @brief <brief description of the file>
     *
     * <Longer, multi-line description of the file, after a blank line>
     */
    ```
4. Each function should be documented using the following pattern:
   
    ```
    /**
     * @brief <brief description of the function>
     * @ingroup <group name, optional, see below>
     *
     * <Longer, multi-line description of the function, after a blank line>
     * @param <param name> <description>
     * @param <param name> <description>
     * @return <description of return value, optional>
     */
    ```

\anchor groups
### Groups

When a function's doc includes an `@ingroup` tag, then it will be listed the "Topics" section of the documentation. Otherwise, it will only appear in the "Files" section (which is harder to navigate).  The defined topics are:

* **API**: For macros and functions that are used in reaction bodies and require no `#include`.
* **Types**: Basic type and struct definitions that may be used in reaction bodies.
* **Constants**: Macros for constants such as time values.
* **Utilities**: Utility functions and data types.
* **Platform**: Functions with platform-specific implementations.
* **Internal**: Functions used in the runtime system.
* **IntTypes**: Structs used in the runtime system.
* **Federated**: Functions used in federated execution.

The groups are defined in the file [docs/doxygen.h](https://github.com/lf-lang/reactor-c/blob/main/docs/doxygen.h).

\anchor tests
## Tests

### System Tests
System tests are Lingua Franca programs that either compile and run to completion (the test passes) or fail to compile or exit the run with an error condition (returning non-zero).
System tests are defined in the [test/C directory](https://github.com/lf-lang/lingua-franca/tree/master/test/C) of the lingua-franca repo.

The Github Actions tests for this repo will automatically run all the C Lingua Franca tests with each of the available schedulers and with both the C and CCpp target specified. The version of the lingua-franca repo that is used to do this is specified by the [lingua-franca-ref.txt](lingua-franca-ref.txt) file in this directory.
Normally, this should point to `master`, but to run tests temporarily with a branch of the code generator, just change [lingua-franca-ref.txt](lingua-franca-ref.txt) to point to your branch.

To run the system tests locally on your machine, check out the lingua-franca repository and replace the reactor-c submodule with your branch:

```sh
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
* Tests in the `scheduling` subdirectory will be run
only if the running platform is Linux and `NUMBER_OF_WORKERS` is set to something bigger than 1 (using `-DNUMBER_OF_WORKERS=2` as a `cmake` parameter). These tests require `sudo` permissions.

To run tests for the single-threaded runtime, execute the following. Note that
`-U` is required to undefine a name that may be cached from a previous run.

```sh
rm -rf build && mkdir build && cd build
cmake .. -UNUMBER_OF_WORKERS
cmake --build .
make test
```

To run the scheduling tests on a Linux platform, provide a nonzero number of workers when invoking `cmake` and use `sudo`. For example, in the `build` directory:

```sh
cmake .. -DNUMBER_OF_WORKERS=2
cmake --build .
sudo make test
```

One of the tests requires sudo because
it changes the scheduling policy and priorities.

To define/undefine other preprocessor definitions such as `LOG_LEVEL`, pass them as
arguments to `cmake` in the same way as with `NUMBER_OF_WORKERS`, using the same
`-D`/`-U` prefixes.


\anchor code-style-and-formatting
## Code style and formatting
We use clang-format to format our codebase. To run the formatter on all source and header files in reactor-c:

```sh
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
