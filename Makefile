CFLAGS=-std=c11 -g -static

9cc: 9cc.c
	docker run -it --rm -v $(PWD):/9cc -w /9cc compilerbook $(CC) -o $@ $? $(LDFLAGS)

test: 9cc
	docker run -it --rm -v $(PWD):/9cc -w /9cc compilerbook ./test.sh

clean:
	rm -f 9cc *.o *~ tmp*

prepare:
	docker build -t compilerbook https://www.sigbus.info/compilerbook/Dockerfile

.PHONY: test clean
