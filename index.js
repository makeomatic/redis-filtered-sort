const fs = require('fs');
const path = require('path');
const lua = fs.readFileSync(path.join(__dirname, 'sorted-filtered-list.lua'));

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
const regexp = /[\^\$\(\)\%\.\[\]\*\+\-\?]/g;
exports.escape = function escape(filter) {
  return filter.replace(regexp, '%$&');
};

/**
 * Exports raw script
 * @type {Buffer}
 */
exports.script = lua;
