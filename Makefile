all: hello

hello.o: hello.cc
	clang++ -std=c++14 -c hello.cc $(shell pkg-config libffi --cflags)

extern.o: extern.cc
	clang++ -std=c++14 -c extern.cc $(shell llvm-config --cflags)

hello: hello.o extern.o
	clang++ -rdynamic $(shell llvm-config --ldflags) -Wl,--whole-archive -ldl $(shell pkg-config libffi --libs) $(shell llvm-config --system-libs --libs) -Wl,--no-whole-archive hello.o extern.o -o hello

llvm-c-poc: llvm-c-poc.c
	clang++ llvm-c-poc.c $(shell llvm-config --cflags --ldflags --system-libs --libs) -o llvm-c-poc

out.ll: hello pint.pint helloworld.pint
	./hello pint.pint helloworld.pint

helloworld: out.ll
	clang out.ll -o helloworld

includer-hack.ll: includer-hack.pint hello pint.pint
	./hello pint.pint includer-hack.pint
	mv out.ll includer-hack.ll

includer-hack.s: includer-hack.ll
	llc includer-hack.ll

includer-hack: includer-hack.s
	clang includer-hack.s -o includer-hack

clean:
	rm -f hello hello.o extern.o llvm-c-poc out.ll helloworld includer-hack{,.s,.ll}

.PHONY: all clean

