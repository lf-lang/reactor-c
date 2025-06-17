[![CI](https://github.com/lf-lang/reactor-c/actions/workflows/ci.yml/badge.svg)](https://github.com/lf-lang/reactor-c/actions/workflows/ci.yml)
[![API docs](https://github.com/lf-lang/reactor-c/actions/workflows/api-docs.yml/badge.svg)](https://github.com/lf-lang/reactor-c/actions/workflows/api-docs.yml)

# Reactor-C Documentation

## Automatically Generated Doc Files

The code in reactor-c is documented with Javadoc-style comments that are automatically processed and deployed when you push updates to the repo.  The latest docs can be found here:

- [reactor-c docs](https://www.lf-lang.org/reactor-c/)

## Building Doc Files Locally

To clone the repo and build the doc files locally, simply do this:

```
git clone git@github.com:lf-lang/reactor-c.git
cd reactor-c/docs
make docs
open docs/_build/html/index.html
```
