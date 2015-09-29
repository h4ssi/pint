SHELL = /bin/bash
pint_deps = hello pint.pint std.pint parser.pint io.pint includer-hack build-pint

all: hello

hello.o: hello.cc
	clang++ -std=c++14 -c hello.cc $(shell pkg-config libffi --cflags)

extern.o: extern.cc
	clang++ -std=c++14 -c extern.cc $(shell llvm-config --cflags)

hello: hello.o extern.o
	clang++ -rdynamic $(shell llvm-config --ldflags) -Wl,--whole-archive -ldl $(shell pkg-config libffi --libs) $(shell llvm-config --system-libs --libs engine) -Wl,--no-whole-archive hello.o extern.o -o hello

llvm-c-poc: llvm-c-poc.c
	clang++ llvm-c-poc.c $(shell llvm-config --cflags --ldflags --system-libs --libs engine) -o llvm-c-poc

helloworld: helloworld.pint $(pint_deps)
	echo -n helloworld | ./build-pint

# special bootstrap for includer hack
includer-hack.ll: includer-hack.pint hello pint.pint std.pint io.pint parser.pint
	./hello <(cat io.pint std.pint parser.pint pint.pint | grep -v '(include ') includer-hack.pint
	mv out.ll includer-hack.ll

includer-hack.s: includer-hack.ll
	llc includer-hack.ll

includer-hack: includer-hack.s
	clang includer-hack.s -o includer-hack

build-pint.ll: build-pint.pint includer-hack hello pint.pint std.pint io.pint parser.pint
	./hello <(echo -n pint.pint | ./includer-hack) <(echo -n build-pint.pint | ./includer-hack)
	mv out.ll build-pint.ll

build-pint.s: build-pint.ll
	llc build-pint.ll

build-pint: build-pint.s
	clang build-pint.s -o build-pint

clean:
	rm -f hello hello.o extern.o llvm-c-poc helloworld{,.s,.ll} includer-hack{,.s,.ll} build-pint{,.s,.ll}

.PHONY: all clean

