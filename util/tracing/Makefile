# Makefile for utilities that convert Lingua Franca trace files
# into other formats.
# @author: Edward A. Lee
REACTOR_C=../..
CURRENT_PATH=$(shell pwd)
CC=gcc
CFLAGS=	-I$(REACTOR_C)/include/core/ \
		-I$(REACTOR_C)/include/core/modal_models \
		-I$(REACTOR_C)/include/core/platform \
		-I$(REACTOR_C)/include/core/utils \
		-DLF_UNTHREADED=1 \
		-Wall
DEPS=
LIBS=-lcurl

INSTALL_PREFIX ?= /usr/local
BIN_INSTALL_PATH = $(INSTALL_PREFIX)/bin

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

trace_to_csv: trace_to_csv.o trace_util.o
	$(CC) -o trace_to_csv trace_to_csv.o trace_util.o
	
trace_to_chrome: trace_to_chrome.o trace_util.o
	$(CC) -o trace_to_chrome trace_to_chrome.o trace_util.o

trace_to_influxdb: trace_to_influxdb.o trace_util.o
	$(CC) -o trace_to_influxdb trace_to_influxdb.o trace_util.o $(LIBS)

install: trace_to_csv trace_to_chrome trace_to_influxdb
	cp trace_to_csv $(BIN_INSTALL_PATH)
	cp trace_to_chrome $(BIN_INSTALL_PATH)
	cp trace_to_influxdb $(BIN_INSTALL_PATH)
	cp ./visualization/fedsd.py $(BIN_INSTALL_PATH)
	ln -f -s $(BIN_INSTALL_PATH)/fedsd.py $(BIN_INSTALL_PATH)/fedsd
	chmod +x $(BIN_INSTALL_PATH)/fedsd
	
clean:
	rm -f *.o
