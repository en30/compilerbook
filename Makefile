CC=docker run -it --rm -v $(PWD):/9cc -w /9cc compilerbook cc
CFLAGS=-std=c11 -g -static
SRCS=$(filter-out tmp.c, $(wildcard *.c))
OBJS=$(SRCS:.c=.o)

9cc: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJS): 9cc.h

test: 9cc
	docker run -it --rm -v $(PWD):/9cc -w /9cc compilerbook ./test.sh

clean:
	rm -f 9cc *.o *~ tmp*

prepare:
	docker build -t compilerbook https://www.sigbus.info/compilerbook/Dockerfile

.PHONY: test clean
