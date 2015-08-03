all: hello

hello: hello.cc
	clang++ -std=c++14 hello.cc $(shell pkg-config libffi --cflags --libs) -ldl -o hello

clean:
	rm -f hello

.PHONY: all clean

