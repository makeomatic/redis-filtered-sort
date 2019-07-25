FROM alpine AS build-mod
RUN apk add --no-cache --virtual .build-deps \
		coreutils \
		gcc \
		linux-headers \
		make \
		musl-dev \
    && apk add \
        bash
RUN mkdir /src
WORKDIR /src

ADD . /src/
RUN make deps && make

FROM redis:5.0.5-alpine
COPY --from=build-mod /src/redis-filtered-sort/filter_module.so /usr/local/lib/redis_filtered_sort.so
ENTRYPOINT ["docker-entrypoint.sh"]
CMD ["redis-server", "--loadmodule", "/usr/local/lib/redis_filtered_sort.so"]
EXPOSE 6379
