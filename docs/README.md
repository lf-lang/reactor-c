[![CI](https://github.com/lf-lang/reactor-c/actions/workflows/ci.yml/badge.svg)](https://github.com/lf-lang/reactor-c/actions/workflows/ci.yml)
[![API docs](https://github.com/lf-lang/reactor-c/actions/workflows/api-docs.yml/badge.svg)](https://github.com/lf-lang/reactor-c/actions/workflows/api-docs.yml)

# Reactor-C Documentation

## Documentation

### Prerequisites

- Install `python3`, `pip3` and `doxygen`
- Install the required Python modules:
  - `pip3 install sphinx`
  - `pip3 install sphinx_sitemap`
  - `pip3 install sphinx-rtd-theme`
  - `pip3 install breathe`
  - `pip3 install exhale`

### Build Documentation Files

- Check out this repo and build the docs:
  - `git clone git@github.com:lf-lang/reactor-c.git`
  - `cd reactor-c/docs`
  - `make html`

### View Documentation Files

- Point your browser to the generated HTML page:
  - `firefox _build/html/index.html`

