# Multithreading

OnLoad module allocates pool of workers, default 4 or depends on passed parameter;

When request arrives, module assigns work into pool queue;

All tasks processed in FIFO mode.

**TODO** Find Why RedisModueApi Not allows Block client in transaction scope. Throws segfault from Redis code.

## FsortAggregate
Client paused and Job assigned into worker pool queue

## Fsort
Client paused and Job assigned into worker pool queue
When result data being saved, we're checking whether same key already been created. If so, not writing data into result list and continue work.

**TODO** Think we need add some db level locks. To avoid overprocessing data for sorts and filters. E.g. if one task already started processing command, threads with same command should wait for result.
