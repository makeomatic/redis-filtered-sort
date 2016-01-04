# redis-filtered-sort
Exports LUA script, which is able to perform multi filter operations, as well as sorts

## Installation

`npm i redis-filtered-sort -S`

## Usage

```js
const filteredSort = require('redis-filtered-sort');
const Redis = require('ioredis');
const redis = new Redis();

// adds redis.sortedFilteredList command to redis instance
filteredSort.attach(redis);

// raw Buffer of lua script
// filteredSort.script

const filter = {
  '#': 'mamba',
  priority: {
    gt: 10,
  },
};
const offset = 10;
const limit = 20;
const expiration = 30000; // ms

// perform op
redis
  .sortedFilteredList('set-of-ids', 'metadata*', 'priority', 'DESC', JSON.stringify(filter), offset, limit, expiration)
  .then(data => {
    // how many items in the list
    // rest of the data is ids from the 'set-of-ids'
    const length = parseInt(data.pop(), 10);

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
