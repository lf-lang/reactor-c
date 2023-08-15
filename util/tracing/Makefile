# Makefile for utilities that convert Lingua Franca trace files
# into other formats.
# @author: Edward A. Lee
REACTOR_C=../../
CC=gcc
CFLAGS=	-I$(REACTOR_C)/include/core/ \
		-I$(REACTOR_C)/include/core/modal_models \
		-I$(REACTOR_C)/include/core/platform \
		-I$(REACTOR_C)/include/core/utils \
		-DLF_UNTHREADED=1 \
		-Wall
DEPS=
LIBS=-lcurl

INSTALL_PREFIX ?= /usr/local/bin

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

trace_to_csv: trace_to_csv.o trace_util.o
	$(CC) -o trace_to_csv trace_to_csv.o trace_util.o
	
trace_to_chrome: trace_to_chrome.o trace_util.o
	$(CC) -o trace_to_chrome trace_to_chrome.o trace_util.o

trace_to_influxdb: trace_to_influxdb.o trace_util.o
	$(CC) -o trace_to_influxdb trace_to_influxdb.o trace_util.o $(LIBS)

install: trace_to_csv trace_to_chrome trace_to_influxdb
	mv trace_to_csv $(INSTALL_PREFIX)
	mv trace_to_chrome $(INSTALL_PREFIX)
	mv trace_to_influxdb $(INSTALL_PREFIX)
	ln -f -s launch-fedsd.sh $(INSTALL_PREFIX)/fedsd
	chmod +x launch-fedsd.sh
	
clean:
	rm -f *.o
