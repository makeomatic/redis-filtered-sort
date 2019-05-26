const { FSORT_TEMP_KEYSET } = require('./constants');

function getIndexTempKeys(index) {
  return `${index}::${exports.FSORT_TEMP_KEYSET}`;
}

async function fsort(...args) {
  const {
    redis,
    dlock,
  } = this;

  const [
    idSet,
    metadataKey,
    hashKey,
    order,
    filter,
    curTime,
    offset = 0,
    limit = 10,
    expiration = 30000,
    returnKeyOnly = false,
  ] = args;

  const tempKeysSet = getIndexTempKeys(idSet);
  const jsonFilter = JSON.parse(filter);

  const [FFLKey, PSSKey] = prepareKeys({
    metadataKey,
    hashKey,
    jsonFilter,
  });
}

function prepareKeys(opts) {
  const {
    metadataKey,
    hashKey,
    jsonFilter,
  }

  const finalFilteredListKeys = [idSet, order];
  const preSortedSetKeys = [idSet, order];
  const totalFilters = Object.keys(jsonFilter).length;

  if (metadataKey) {
    if (hashKey) {
      finalFilteredListKeys.push(metadataKey, hashKey);
      preSortedSetKeys.push(metadataKey, hashKey);
    }

    if (totalFilters > 0) {
      finalFilteredListKeys.push(filter);
    }
  } else if (totalFilters >= 1 && typeof jsonFilter['#'] === 'string') {
    finalFilteredListKeys.push(`{"#":"${jsonFilter['#']}"}`);
  }

  return [
    finalFilteredListKeys.join(':'),
    preSortedSetKeys.join(':'),
  ];
}

function FilteredSort(redis, dlock) {
  this.dlock = dlock;
  this.redis = redis;
  this.fsort = fsort.bind(this);
}

module.exports = dlock => new FilteredSort(dlock);
