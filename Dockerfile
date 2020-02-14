# Alpine image should be used same as the base image of the `redis:${ver}-alpine`
FROM alpine:${ALPINE_VERSION} AS build-base
RUN apk --no-cache add \
		coreutils \
		g++ \
		make \
		cmake \
		boost-dev \
    && apk add \
        bash
RUN mkdir /src
WORKDIR /src

FROM redis:${REDIS_VERSION}-alpine as base-redis
RUN apk --no-cache add \
		libgcc \
		libstdc++ \
		boost

FROM build-base as build-fsort
ADD . /src/
RUN mkdir /src/build && cmake -B./build -S. && cd build && make -j 4

FROM base-redis
LABEL module_version=${MODULE_VERSION}
COPY --from=build-fsort /src/build/libredis_filtered_sort.so /usr/local/lib/libredis_filtered_sort.so

ENTRYPOINT ["docker-entrypoint.sh", "--loadmodule", "/usr/local/lib/libredis_filtered_sort.so"]

EXPOSE 6379
