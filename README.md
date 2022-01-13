[![CI](https://github.com/lf-lang/reactor-c/actions/workflows/ci.yml/badge.svg)](https://github.com/lf-lang/reactor-c/actions/workflows/ci.yml)
[![API docs](https://github.com/lf-lang/reactor-c/actions/workflows/api-docs.yml/badge.svg)](https://github.com/lf-lang/reactor-c/actions/workflows/api-docs.yml)

# Reactor-C: A reactor runtime implementation in C

## Documentation
To generate and view documentation, follow the following steps:
- Install `python3`, `pip3` and `doxygen`
- Install the required Python modules:
  - `pip3 install sphinx`
  - `pip3 install sphinx_sitemap`
  - `pip3 install sphinx-rtd-theme`
  - `pip3 install breathe`
  - `pip3 install exhale`
- Check out this repo and build the docs:
  - `git clone git@github.com:lf-lang/reactor-c.git`
  - `cd reactor-c/doc-sphinx`
  - `make html`
- Point your browser to the generated HTML page:
  - `firefox _build/html/index.html`

## Testing
To create a new test, write a C program with a file name ending in "test.c"
in a subdirectory of the `test` directory.
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

To define/undefine other preprocessor definitions such as `LOG_LEVEL`, pass them as
arguments to `cmake` in the same way as with `NUMBER_OF_WORKERS`, using the same
`-D`/`-U` prefixes.
