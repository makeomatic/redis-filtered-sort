const {Pipeline} = require('ioredis');

// cached vars
const regexp = /[\^\$\(\)\%\.\[\]\*\+\-\?]/g;
const keys = Object.keys;
const stringify = JSON.stringify;
const BLACK_LIST_PROPS = ['eq', 'ne'];

// compat fix, cmod's variable hardcoded
exports.FSORT_TEMP_KEYSET = 'fsort_temp_keys';

/**
 * Creates new ioredis command
 * @param  {ioredis} redis
 * @param  {string} Command name
 * @returns {function} New ioredis nonbuffer command
 */
function createModuleCommand(redis, name) {
  let funcs = redis.createBuiltinCommand(name);
  redis[name] = funcs.string;
  redis[name + "Buffer"] = funcs.buffer;
}

/**
 * Sets argument transformer for redis command
 * @param {ioredis} redis 
 * @param {string} name 
 * @param {int} keyCount 
 */
function cmodAttachArgTransformer(redis, name, keyCount) {
  let remapFunc = redis[name];
  remapFunc = remapFunc.bind(redis);
  
  redis[name] = function (...args) {
    var keyPrefix = redis.options.keyPrefix;
    if (keyPrefix) {
      for (let i = 0; i < keyCount; i++) {
        if (args[i] !== null) {
          args[i] = keyPrefix + args[i];
        }
      }
    }
    return remapFunc(...args);
  }
}

/**
 * C-Module exported functions
 * We have to rebind them manually. IOredis hardcoded on `redis-commands` package.
 * Think it's incorrect to change ioredis deps.
 */
const cmodFunctions = {
  fsort: 2,
  fsortBust: 1,
  fsortAggregate: 2 
}

function cmodWrapCommands(obj) {
  Object.keys(cmodFunctions).forEach((fName) => {
    createModuleCommand(obj, fName);
    cmodAttachArgTransformer(obj, fName, cmodFunctions[fName]);
  })
}

function cmodWrapPipeline(redis) {
  const rc = redis.constructor;
  const pipeFunc = rc.prototype.pipeline.bind(redis);
  rc.prototype.pipeline = function () {
    let newPipe = pipeFunc(...arguments);
    cmodWrapCommands(newPipe);
    return newPipe;
  };
}

/**
 * Currently unsuported. Causes module failure
 * @param {ioredis} redis 
 */
function cmodWrapMulti(redis) {
  const rc = redis.constructor;
  const multiFunc = rc.prototype.multi.bind(redis);
  rc.prototype.multi = function () {
    let newMulti = multiFunc(...arguments);
    cmodWrapCommands(newMulti);
    return newMulti;
  };
}


exports.attach = function attachToRedis(redis, _name, useSnakeCase = false) {
  //Let this vars be here, for compatibility with previous versions
  let _useSnakeCase = useSnakeCase;
  let __name = _name;
  cmodWrapCommands(redis);
  cmodWrapPipeline(redis);
  //cmodWrapMulti(redis);
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

