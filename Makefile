# This file lets you format the code-base with a single command.
FILES := $(shell find . -name '*.c' -o -name '*.h')
.PHONY: format
format:
	clang-format -i -style=file $(FILES)