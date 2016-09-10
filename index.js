const fs = require('fs');
const path = require('path');
const calcSlot = require('cluster-key-slot');
const lua = fs.readFileSync(path.join(__dirname, 'sorted-filtered-list.lua'));

// cached vars
const regexp = /[\^\$\(\)\%\.\[\]\*\+\-\?]/g;
const keys = Object.keys;
const stringify = JSON.stringify;
const BLACK_LIST_PROPS = ['eq', 'ne'];

/**
 * Attached .sortedFilteredList function to ioredis instance
 * @param  {ioredis} redis
 */
exports.attach = function attachToRedis(redis, _name) {
  const name = _name || 'sortedFilteredList';
  redis.defineCommand(name, { numberOfKeys: 2, lua: lua.toString('utf-8') });
};

/**
 * Performs escape on a filter clause
 * ^ $ ( ) % . [ ] * + - ? are prefixed with %
 *
 * @param  {String} filter
 * @return {String}
 */
function escape(filter) {
  return filter.replace(regexp, '%$&');
};
exports.escape = escape;

/**
 * Walks over object and escapes string, does not touch #multi.fields
 * @param  {Object} obj
 * @return {Object}
 */
function iterateOverObject(obj) {
  const props = keys(obj);
  const output = {};
  props.forEach(function iterateOverProps(prop) {
    const val = obj[prop];
    const type = typeof val;
    if (type === 'string' && BLACK_LIST_PROPS.indexOf(prop) === -1) {
      output[prop] = escape(val);
    } else if (type === 'object') {
      if (prop !== '#multi') {
        output[prop] = iterateOverObject(val);
      } else {
        output[prop] = val;
        val.match = escape(val.match);
      }
    } else {
      output[prop] = val;
    }
  });

  return output;
}

/**
 * Goes over filter and escapes each string
 * @param  {Object} obj
 * @return {String}
 */
exports.filter = function filter(obj) {
  return stringify(iterateOverObject(obj));
};

/**
 * Exports raw script
 * @type {Buffer}
 */
exports.script = lua;

/**
 * Invalidates cached filtered list
 */
exports.invalidate = function invalidate(redis, keyPrefix, _index) {
  const keyPrefixLength = keyPrefix.length;
  const index = `${keyPrefix}${_index}`;
  const cacheKeys = [];
  const slot = calcSlot(index);

  // this has possibility of throwing, but not likely to since previous operations
  // would've been rejected already, in a promise this will result in a rejection
  const nodeKeys = redis.slots[slot];
  const masterNode = nodeKeys.reduce((node, key) => node || redis.connectionPool.nodes.master[key], null);
  const transform = (keys, prefixLength) => keys.map(key => key.slice(prefixLength));

  function scan(node, cursor = '0') {
    return node
      .scan(cursor, 'MATCH', `${index}:*`, 'COUNT', 50)
      .then(response => {
        const [next, keys] = response;

        if (keys.length > 0) {
          cacheKeys.push(...transform(keys, keyPrefixLength));
        }

        if (next === '0') {
          if (cacheKeys.length === 0) {
            return Promise.resolve(0);
          }

          return redis.del(cacheKeys);
        }

        return scan(node, next);
      });
  }

  return scan(masterNode);
}
