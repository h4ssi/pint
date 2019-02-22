pint_deps = hello pint.pint std.pint parser.pint io.pint includer-hack build-pint

docker_image = pint/build-env
docker = sudo docker run --rm -i --mount type=bind,source="$(CURDIR)",target=/pint -u $(shell id -u):$(shell id -g) --ulimit stack=-1 $(docker_image)

.PHONY: all test clean docker_image test-self-hosting

all: pint

docker_image:
	sudo docker build -t $(docker_image) docker

hello.o: hello.cc | docker_image
	$(docker) clang++ -std=c++14 -c $< $(shell $(docker) pkg-config libffi --cflags)

extern.o: extern.cc | docker_image
	$(docker) clang++ -std=c++14 -c $< $(shell $(docker) llvm-config --cxxflags)

hello: hello.o extern.o | docker_image
	$(docker) clang++ -rdynamic $(shell $(docker) llvm-config --ldflags) -Wl,--whole-archive -ldl $(shell $(docker) pkg-config libffi --libs) $(shell $(docker) llvm-config --system-libs --libs engine) -Wl,--no-whole-archive $+ -o $@

llvm-c-poc: llvm-c-poc.c | docker_image
	$(docker) clang++ $< $(shell $(docker) llvm-config --cflags --ldflags --system-libs --libs engine) -o $@

# special bootstrap for includer-hack (cannot use includer-hack)
includer-hack.ll: includer-hack.pint hello pint.pint std.pint io.pint parser.pint | docker_image
	cat io.pint std.pint parser.pint pint.pint | grep -v '(include ' > .pint.pint
	$(docker) ./hello .pint.pint $<
	mv out.ll $@

%.s: %.ll | docker_image
	$(docker) llc -relocation-model=pic $<

includer-hack: includer-hack.s | docker_image
	$(docker) clang $< -o $@

# special bootstrap for build-pint (cannot use build-pint)
build-pint.ll: build-pint.pint includer-hack hello pint.pint std.pint io.pint parser.pint | docker_image
	echo -n pint.pint | $(docker) ./includer-hack > .pint.pint || /bin/true
	echo -n build-pint.pint | $(docker) ./includer-hack > .build-pint.pint || /bin/true
	$(docker) ./hello .pint.pint .build-pint.pint
	mv out.ll $@

build-pint: build-pint.s | docker_image
	$(docker) clang $< -o $@

helloworld: helloworld.pint $(pint_deps) | docker_image
	echo -n helloworld | $(docker) ./build-pint

# special bootstrap for pint (no linker support in build-pint)
pint.ll: hello pint.pint std.pint parser.pint io.pint includer-hack | docker_image
	echo -n pint.pint | $(docker) ./includer-hack > .pint.pint || /bin/true
	$(docker) ./hello .pint.pint .pint.pint
	mv out.ll $@

pint: pint.s extern.o | docker_image
	$(docker) clang++ $+ $(shell $(docker) llvm-config --link-static --ldflags --system-libs --libs engine) -o $@

pint-from-compiler.ll: pint pint.pint std.pint parser.pint io.pint includer-hack | docker_image
	echo -n pint.pint | $(docker) ./includer-hack | $(docker) ./pint || /bin/true
	mv out.ll $@

test-self-hosting: pint.ll pint-from-compiler.ll
	diff $+

clean:
	rm -f hello hello.o extern.o llvm-c-poc helloworld{,.s,.ll} includer-hack{,.s,.ll} pint{,.s,.ll,-from-compiler.ll} build-pint{,.s,.ll} .{,build-}pint.pint

