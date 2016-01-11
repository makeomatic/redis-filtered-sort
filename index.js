const fs = require('fs');
const path = require('path');
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
