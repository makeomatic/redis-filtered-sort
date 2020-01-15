include variables.include

ifdef IMAGE_NAME
	DOCKER_IMAGE = $(IMAGE_NAME)
endif

all: build

docker-push:
	docker push $(DOCKER_IMAGE)

docker-build:
	docker build . -f ./docker/Dockerfile -t $(DOCKER_IMAGE)
docker-rebuild: 
	docker build . --no-cache -f ./docker/Dockerfile -t $(DOCKER_IMAGE)

build:
	cd redis-filtered-sort && $(MAKE)

clean:
	cd redis-filtered-sort && $(MAKE) $@
