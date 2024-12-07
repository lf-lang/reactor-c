[![CI](https://github.com/lf-lang/reactor-c/actions/workflows/ci.yml/badge.svg)](https://github.com/lf-lang/reactor-c/actions/workflows/ci.yml)
[![API docs](https://github.com/lf-lang/reactor-c/actions/workflows/api-docs.yml/badge.svg)](https://github.com/lf-lang/reactor-c/actions/workflows/api-docs.yml)

# Reactor-C: A reactor runtime implementation in C

## Documentation

To generate and view documentation, see [docs/README.md](docs/README.md).

## Code-formatting

We use clang-format to format our codebase. To run the formatter on all source and header files in reactor-c:
```
make format
```
The CI will do a "dry-run" of the formatter to verify that all files are correctly formatted.

VSCode can be configured to run clang-format on files as they are saved. To achieve this set the following settings:
- editor.formatOnSave: true
- C_Cpp.formatting: clang-format
- C_Cpp.clang_format_style: file


## Testing

The Github Actions tests for this repo will automatically run all the C Lingua Franca tests with each of the available schedulers. The version of the lingua-franca repo that is used to do this is specified by the lingua-franca-ref.txt file in this directory.

To create a new unit test, write a C program with a file name ending in "test.c"
in a subdirectory of the `test` directory. That file should contain a main and should return 0 if the test succeeds.

* Tests in the `general` subdirectory will always be run.
* Tests in the `single-threaded` and `multithreaded` subdirectories will be run
depending on parameters passed to `cmake`.

To run tests for the single-threaded runtime, execute the following. Note that
`-U` is required to undefine a name that may be cached from a previous run.

- `cd build`
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
