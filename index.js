const fs = require('fs');
const path = require('path');

const camelCase = require('lodash/camelCase');
const snakeCase = require('lodash/snakeCase');

const lua = fs.readFileSync(path.join(__dirname, 'sorted-filtered-list.lua'));
const fsortBust = fs.readFileSync(path.join(__dirname, 'filtered-list-bust.lua'));

// cached vars
const regexp = /[\^\$\(\)\%\.\[\]\*\+\-\?]/g;
const keys = Object.keys;
const stringify = JSON.stringify;
const BLACK_LIST_PROPS = ['eq', 'ne'];

exports.FSORT_TEMP_KEYSET = 'fsort_temp_keys';

const luaWrapper = (script) => `
---

local function getIndexTempKeys(index)
  return "${exports.FSORT_TEMP_KEYSET}:" .. index;
end

local curTime = ${Date.now()};

---

${script.toString('utf-8')}
`;

/**
 * Attached .sortedFilteredList function to ioredis instance
 * @param  {ioredis} redis
 */
const fsortScript = luaWrapper(lua);
const fsortBustScript = luaWrapper(fsortBust);

exports.attach = function attachToRedis(redis, _name, useSnakeCase = false) {
  const name = _name || 'sortedFilteredList';
  const bustName = (useSnakeCase ? snakeCase : camelCase)(`${name}Bust`);

  redis.defineCommand(name, { numberOfKeys: 2, lua: fsortScript });
  redis.defineCommand(bustName, { numberOfKeys: 1, lua: fsortBustScript });
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

