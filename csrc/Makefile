include variables.include

ifdef IMAGE_NAME
	DOCKER_IMAGE = $(IMAGE_NAME)
endif

all: clean build

docker-push:
	docker push $(DOCKER_IMAGE)

docker-build:
	docker build . -f ./docker/Dockerfile -t $(DOCKER_IMAGE)
docker-rebuild: 
	docker build . --no-cache -f ./docker/Dockerfile -t $(DOCKER_IMAGE)

build:
	cd redis-filtered-sort && $(MAKE) all

clean:
	cd redis-filtered-sort && $(MAKE) $@

deps: jansson


clean-deps:
	rm -rf $(JANSSON_LIBDIR)/*

jansson: jansson_src jansson_configure
	(cd $(JANSSON_LIBDIR) && make CFLAGS='$(CFLAGS)')

jansson_configure:
	(cd $(JANSSON_LIBDIR) && ./configure)

jansson_clean:
	(cd $(JANSSON_LIBDIR) && make clean)

jansson_src: $(JANSSON_LIBDIR)/src/jansson.h

$(JANSSON_LIBDIR)/src/jansson.h:
	mkdir -p $(JANSSON_LIBDIR) && wget -O- $(JANSSON_LINK) | tar xvz --directory=$(JANSSON_LIBDIR) --strip-components=1 jansson-$(JANSSON_VERSION)