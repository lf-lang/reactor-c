# This file lets you format the code-base with a single command.
FILES := $(shell find . -name '*.c' -o -name '*.h')

.PHONY: help
help:
	@echo "Available commands:"
	@echo "  make format        - Format all C source files using clang-format"
	@echo "  make format-check  - Check whether C source files are properly formatted"
	@echo "  make docs          - Generate documentation using Doxygen"
	@echo "  make clean         - Clean up build and documentation"
	@echo "  make unit-tests    - Compile and run unit tests (without scheduling tests)"
	
.PHONY: format
format:
	clang-format -i -style=file $(FILES)

.PHONY: format-check
format-check:
	clang-format --dry-run --Werror -style=file $(FILES)

.PHONY: docs
docs:
	cd docs && doxygen Doxyfile.in

.PHONY: clean
clean:
	rm -rf build docs/_build

.PHONY: unit-tests
unit-tests: clean
	# In case NUMBER_OF_WORKERS has been set, unset it.
	cmake -B build -UNUMBER_OF_WORKERS
	cmake --build build
	cd build && make test

# Set help as the default target
.DEFAULT_GOAL := help
