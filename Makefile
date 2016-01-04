SHELL = /bin/bash
pint_deps = pint includer-hack build-pint

all: pint-dev

pint: bootstrap.sh
	./bootstrap.sh

helloworld: helloworld.pint $(pint_deps)
	echo -n helloworld | ./build-pint

# special bootstrap for includer hack
includer-hack.ll: pint includer-hack.pint
	cat includer-hack.pint | ./pint
	mv out.ll includer-hack.ll

includer-hack.s: includer-hack.ll
	llc includer-hack.ll

includer-hack: includer-hack.s
	clang includer-hack.s -o includer-hack

pint-dev.ll: pint pint.pint std.pint parser.pint io.pint includer-hack
	echo -n pint.pint | ./includer-hack | ./pint
	mv out.ll pint-dev.ll

pint-dev.s: pint-dev.ll
	llc pint-dev.ll

pint-dev: pint-dev.s
	clang++ pint-dev.s $(shell llvm-config --ldflags --system-libs --libs engine) -o pint-dev

build-pint.ll: pint build-pint.pint includer-hack pint.pint std.pint io.pint
	echo -n build-pint.pint | ./includer-hack | ./pint
	mv out.ll build-pint.ll

build-pint.s: build-pint.ll
	llc build-pint.ll

build-pint: build-pint.s
	clang build-pint.s -o build-pint

poc.ll: pint includer-hack poc.pint
	echo -n poc.pint | ./includer-hack | ./pint
	mv out.ll poc.ll

poc.s: poc.ll
	llc poc.ll

poc: poc.s
	clang++ poc.s $(shell llvm-config --ldflags --system-libs --libs engine) -o poc

clean:
	rm -f helloworld{,.s,.ll} includer-hack{,.s,.ll} pint-dev{,.s,.ll} build-pint{,.s,.ll} poc{,.s,.ll}

clean-bootstrap: clean
	rm -f pint

.PHONY: all clean clean-bootstrap

