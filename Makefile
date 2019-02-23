pint_deps = pint-dev includer-hack-dev build-pint-dev

docker_image = pint/build-env
docker = sudo docker run --rm -i --mount type=bind,source="$(CURDIR)",target=/pint -u $(shell id -u):$(shell id -g) --ulimit stack=-1 $(docker_image)

ROOTDIR ?= $(CURDIR)
export ROOTDIR

define devified
$(<:.pint=-dev.pint)
endef
define devify =
sed -E "s/\\(pathed '(includer-hack|pint)'\\)/(pathed '\1-dev')/g" $< > $(devified)
endef

define undevify =
rm -f $(devified)
endef

.PHONY: all bootstrap test clean clean-bootstrap docker_image test-self-hosting self

all: pint-dev

# build-pint uses interpreter, so cannot be used from previous stage

prev_step = bootstrap/step-1
bootstrapped = pint includer-hack

bootstrap: $(bootstrapped)
$(bootstrapped):
	rm -rf "$(ROOTDIR)/$(prev_step)"
	git -C "$(ROOTDIR)" archive "$(shell git -C $(ROOTDIR) for-each-ref --count 1 --format '%(refname:short)' '**/$(prev_step)')" --prefix "$(prev_step)/" | tar -C $(ROOTDIR) -x
	$(MAKE) -C "$(ROOTDIR)/$(prev_step)" test-self-hosting $(bootstrapped)
	cp $(bootstrapped:%=$(ROOTDIR)/$(prev_step)/%) $(CURDIR)

docker_image:
	sudo docker build -t $(docker_image) docker

extern.o: extern.cc | docker_image
	$(docker) clang++ -std=c++14 -c $< $(shell $(docker) llvm-config --cxxflags)

# special bootstrap for includer-hack (cannot use build-pint)
includer-hack-dev.ll: includer-hack.pint pint | docker_image
	$(docker) ./pint < $<
	mv out.ll $@

%.s: %.ll | docker_image
	$(docker) llc -relocation-model=pic $<

includer-hack-dev: includer-hack-dev.s | docker_image
	$(docker) clang $< -o $@

# special bootstrap for build-pint (cannot use build-pint)
build-pint-dev.ll: build-pint.pint std.pint io.pint $(bootstrapped) | docker_image
	$(devify)
	echo -n $(devified) | $(docker) ./includer-hack | $(docker) ./pint
	$(undevify)
	mv out.ll $@

build-pint-dev: build-pint-dev.s | docker_image
	$(docker) clang $< -o $@

helloworld: helloworld.pint $(pint_deps) | docker_image
	echo -n $@ | $(docker) ./build-pint-dev

# special bootstrap for pint (no linker support in build-pint)
pint-dev.ll: pint.pint std.pint parser.pint io.pint $(bootstrapped) | docker_image
	echo -n $< | $(docker) ./includer-hack | $(docker) ./pint
	mv out.ll $@

pint-dev: pint-dev.s extern.o | docker_image
	$(docker) clang++ $+ $(shell $(docker) llvm-config --link-static --ldflags --system-libs --libs engine) -o $@

pint-from-dev.ll: pint.pint $(pint_deps) | docker_image
	echo -n $< | $(docker) ./includer-hack-dev | $(docker) ./pint-dev
	mv out.ll $@

test-self-hosting: pint-dev.ll pint-from-dev.ll
	diff $+

self: test-self-hosting $(pint_deps) | docker_image
	cp pint-dev pint # cannot use build-pint (see above)
	echo -n includer-hack | $(docker) ./build-pint-dev
	echo -n build-pint | $(docker) ./build-pint-dev

clean:
	rm -f extern.o helloworld{,.s,.ll} includer-hack-dev{,.s,.ll} pint-dev{,.s,.ll,-from-dev.ll} build-pint-dev{,.s,.ll}

clean-bootstrap: clean
	rm -rf pint includer-hack bootstrap
