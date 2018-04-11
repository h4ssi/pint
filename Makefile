pint_deps = pint-dev includer-hack-dev build-pint-dev

docker_image = pint/build-env
docker = sudo docker run --rm -i --mount type=bind,source="$(CURDIR)",target=/pint -w /pint --ulimit stack=-1 $(docker_image)
chown = sudo chown $(shell id -u):$(shell id -g)

ROOTDIR ?= $(CURDIR)
export ROOTDIR

define devified
$(<:.pint=-dev.pint)
endef
define devify =
sed -E 's/\.\/(includer-hack|pint)\b/&-dev/g' $< > $(devified)
endef

define undevify =
rm -f $(devified)
endef

.PHONY: all bootstrap test clean clean-bootstrap docker_image test-self-hosting self

all: pint-dev

# build-pint uses interpreter, so cannot be used from previous stage

prev_step = bootstrap/step-2
bootstrapped = pint includer-hack build-pint

bootstrap: $(bootstrapped)
$(bootstrapped):
	rm -rf "$(ROOTDIR)/$(prev_step)"
	git -C "$(ROOTDIR)" archive "$(shell git -C $(ROOTDIR) for-each-ref --count 1 --format '%(refname:short)' '**/$(prev_step)')" --prefix "$(prev_step)/" | tar -C $(ROOTDIR) -x
	$(MAKE) -C "$(ROOTDIR)/$(prev_step)" self
	cp $(bootstrapped:%=$(ROOTDIR)/$(prev_step)/%) $(CURDIR)

docker_image:
	sudo docker build -t $(docker_image) docker

includer-hack-dev: includer-hack.pint $(bootstrapped) | docker_image
	$(devify)
	echo -n $@ | $(docker) ./build-pint
	$(undevify)
	$(chown) $@

%.s: %.ll | docker_image
	$(docker) llc -relocation-model=pic $<
	$(chown) $@

build-pint-dev: build-pint.pint std.pint io.pint $(bootstrapped) | docker_image
	$(devify)
	echo -n $@ | $(docker) ./build-pint
	$(undevify)
	$(chown) $@

helloworld: helloworld.pint $(pint_deps) | docker_image
	echo -n $@ | $(docker) ./build-pint-dev
	$(chown) $@

# special bootstrap for pint (no linker support in build-pint)
pint-dev.ll: pint.pint std.pint parser.pint io.pint $(bootstrapped) | docker_image
	echo -n $< | $(docker) ./includer-hack | $(docker) ./pint
	$(chown) out.ll
	mv out.ll $@

pint-dev: pint-dev.s | docker_image
	$(docker) clang++ $< $(shell llvm-config --link-static --ldflags --system-libs --libs engine) -o $@
	$(chown) $@

pint-from-dev.ll: pint.pint $(pint_deps) | docker_image
	echo -n $< | $(docker) ./includer-hack-dev | $(docker) ./pint-dev
	$(chown) out.ll
	mv out.ll $@

test-self-hosting: pint-dev.ll pint-from-dev.ll
	diff $+

self: $(pint_deps) | docker_image
	cp pint-dev pint # cannot use build-pint (see above)
	echo -n includer-hack | $(docker) ./build-pint-dev
	$(chown) includer-hack
	echo -n build-pint | $(docker) ./build-pint-dev
	$(chown) build-pint

poc.ll: poc.pint $(bootstrapped) | docker_image
	echo -n $< | $(docker) ./includer-hack | $(docker) ./pint
	$(chown) out.ll
	mv out.ll $@

poc: poc.s | docker_image
	$(docker) clang++ $< $(shell llvm-config --link-static --ldflags --system-libs --libs engine) -o $@
	$(chown) $@

clean:
	rm -f {poc,helloworld,pint-dev,includer-hack-dev,build-pint-dev}{,.s,.ll} pint-dev-from-dev.ll

clean-bootstrap: clean
	rm -rf pint includer-hack bootstrap
