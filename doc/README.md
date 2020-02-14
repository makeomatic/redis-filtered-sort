# RedisFilterModule

Module basically replicates `http://redis.io/commands/sort` but with extra features and ability to run it in clustered mode with
hashed keys, which resolve to the same slot. Works in separate threads, so Redis db stays unblocked.

* See [HOWTO](./build.md) build module.
* API provided by module [API](./api.md)

## Load options
Module use 4 threads in it's own processing pool. Thread pool size can be changed in `redis.conf`, eg `loadmodule filter_module.so {POOL_SIZE}` or in Redis args `redis-server --loadmodule {filter_module.so} {POOL_SIZE}`  
