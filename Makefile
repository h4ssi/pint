all: hello

hello: hello.cc
	clang++ -std=c++14 hello.cc $(shell pkg-config libffi --cflags --libs) -ldl -o hello

llvm-c-poc: llvm-c-poc.c
	clang++ llvm-c-poc.c $(shell llvm-config --cflags --ldflags --system-libs --libs) -o llvm-c-poc

clean:
	rm -f hello

.PHONY: all clean

