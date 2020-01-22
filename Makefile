DOCKER_IMAGE="makeomatic/redis_filter_mod"

ifdef IMAGE_NAME
	DOCKER_IMAGE = $(IMAGE_NAME)
endif

all: build

docker-push:
	docker push $(DOCKER_IMAGE)

docker-build:
	docker build . -f ./Dockerfile -t $(DOCKER_IMAGE)
docker-rebuild: 
	docker build . --no-cache -f ./Dockerfile -t $(DOCKER_IMAGE)

