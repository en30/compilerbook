# docker run -it --rm -v $(PWD):/9cc -w /9cc compilerbook make
CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.exe)

9cc: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJS): 9cc.h

test/%.exe: 9cc test/common test/%.c
	$(CC) -E -P -C test/$*.c -o test/tmp
	./9cc test/tmp 1> test/$*.s
	$(CC) -o $@ test/$*.s -xc test/common

test: $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done

clean:
	rm -f 9cc *.o *~ tmp* test/*.{s,exs}

prepare:
	docker build -t compilerbook https://www.sigbus.info/compilerbook/Dockerfile

.PHONY: test clean
