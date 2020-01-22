FROM alpine AS build-mod
RUN apk add --no-cache --virtual .build-deps \
		coreutils \
		g++ \
		make \
		cmake \
		boost-static \
		boost-dev \
    && apk add \
        bash
RUN mkdir /src
WORKDIR /src

ADD . /src/
RUN mkdir /src/build && cmake -B./build -S. && cd build && make -j 4

FROM redis:5.0.5-alpine
COPY --from=build-mod /src/build/libredis_filtered_sort.so /usr/local/lib/libredis_filtered_sort.so
ENTRYPOINT ["docker-entrypoint.sh"]
CMD ["redis-server", "--loadmodule", "/usr/local/lib/libredis_filtered_sort.so"]
EXPOSE 6379
