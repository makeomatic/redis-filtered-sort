# redis-filtered-sort
Exports LUA script, which is able to perform multi filter operations, as well as sorts

This basically replicates `http://redis.io/commands/sort` but with extra features and ability to run it in clustered mode with
hashed keys, which resolve to the same slot

## Installation

`npm i redis-filtered-sort -S`

## Usage

```js
const { escape, attach } = require('redis-filtered-sort');
const Redis = require('ioredis');
const redis = new Redis();

// adds redis.sortedFilteredList command to redis instance
attach(redis, 'fsort');

// raw Buffer of lua script
// filteredSort.script

const filter = {
  // only ids with `!mamba%` in them will be presented. Internally it uses lua string.find, so regexp is possible. Escape special chars
  // with % or use escape helper for that
  '#': escape('!mamba%'),
  priority: {
    gte: 10, // only ids, which have priority greater or equal to 10 will be returned
  },
  name: 'love', // only ids, which have 'name' containing 'love' in their metadata will be returned
};
const offset = 10;
const limit = 20;
const sortBy = 'priority';
const expiration = 30000; // ms

// perform op
redis
  .fsort('set-of-ids', 'metadata*', sortBy, 'DESC', JSON.stringify(filter), offset, limit, expiration)
  .then(data => {
    // how many items in the complete list
    // rest of the data is ids from the 'set-of-ids'
    const sortedListLength = parseInt(data.pop(), 10);

    // at this point you might want to populate ids with actual data about them
    // for instance, like this:
    return Promise.map(data, function populateData(id) {
      return Promise.props({
        id,
        data: redis.hgetall('metadata' + id),
      });
    });
  });
```
