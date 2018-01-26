'use strict';

const Promise = require('bluebird');
const Redis = require('ioredis');
const { expect } = require('chai');
const faker = require('faker');
const ld = require('lodash');

describe('filtered sort suite', function suite() {
  const idSetKey = 'id-set';
  const metaKeyPattern = '*-metadata';
  const keyPrefix = 'fsort-test:';
  const redis = new Redis({
    port: process.env.REDIS_PORT_6379_TCP_PORT,
    host: process.env.REDIS_PORT_6379_TCP_ADDR,
    keyPrefix,
  });
  const monitor = redis.duplicate();
  const mod = require('../index.js');
  const prepopulateDataLength = parseInt(process.env.PREPOPULAR || 1000, 10);
  const metadata = {};
  const insertedIds = [];
  const invertedIds = [];
  mod.attach(redis, 'fsort');

  function metadataKey(id) {
    return metaKeyPattern.replace('*', id);
  }

  function generateUser(n) {
    const id = `id-${n}`;
    const user = metadata[id] = {
      email: faker.internet.email(),
      name: faker.name.firstName(),
      surname: faker.name.lastName(),
      balance: faker.finance.amount(),
      favColor: faker.commerce.color(),
      country: faker.address.countryCode(),
      age: ld.random(10, 60),
    };

    if (ld.random(1, 2) === 2) {
      user.fieldExists = 1;
    }

    insertedIds.push(id);
    invertedIds.push(id);

    return Promise.join(
      redis.sadd(idSetKey, id),
      redis.hmset(metadataKey(id), user)
    );
  }

  function comparatorASC(a, b) {
    return a.localeCompare(b);
  }

  function comparatorDESC(a, b) {
    return b.localeCompare(a);
  }

  function sortedBy(comparator, listLength, field) {
    return function sorted(ids) {
      const length = ids.pop();
      if (typeof listLength !== 'undefined') {
        expect(parseInt(length, 10)).to.be.eq(listLength);
      }
      const copy = [].concat(ids);
      copy.sort(comparator);

      if (field) {
        // because lua is -1/+1 and js is -1/0/+1 ids can be sorted differently
        // therefore we compare sort by derivative
        const map = id => metadata[id][field];
        expect(copy.map(map)).to.be.deep.eq(ids.map(map));
      } else {
        expect(copy).to.be.deep.eq(ids);
      }
    };
  }

  before('cleanup', function cleanup() {
    return redis.flushdb();
  });

  before('populate data', function pretest() {
    this.timeout(5000);
    const promises = ld.times(prepopulateDataLength, generateUser);

    // alphanum sort, use locale based sorting
    insertedIds.sort(comparatorASC);
    invertedIds.sort(comparatorDESC);

    return Promise.all(promises);
  });

  if (process.env.DEBUG === '1') {
    before('monitor', function pretest() {
      const { inspect } = require('util');
      return monitor.monitor().then(mon => {
        mon.on('monitor', function (time, args) {
          console.info(time + ": " + inspect(args));
        });
      });
    });

    after('monitor', function aftertest() {
      redis.quit();
      monitor.quit();
    });
  }

  it('filter escaping', function test() {
    expect(mod.filter({
      '#': 'mamba-%',
      '#multi': {
        fields: ['duramba!%'],
        match: 'caramba^',
      },
      walkie: {
        gte: 10,
        ne: 'bore-dom',
        eq: 'karamba%',
      },
      tom: '20',
      berlin: '!werthechampions0-9',
    })).to.be.eq(JSON.stringify({
      '#': 'mamba%-%%',
      '#multi': {
        fields: ['duramba!%'],
        match: 'caramba%^'
      },
      walkie: {
        gte: 10,
        ne: 'bore-dom',
        eq: 'karamba%',
      },
      tom: '20',
      berlin: '!werthechampions0%-9'
    }));
  });

  describe('cache invalidation', function invalidationSuite() {
    it('should invalidate cache: sort', function test() {

      return redis.fsort(idSetKey, null, null, 'ASC', '{}', Date.now())
        .then(() => redis.zrangebyscore(`${idSetKey}::${mod.FSORT_TEMP_KEYSET}`, '-inf', '+inf'))
        .tap(keys => expect(keys.length).to.be.eq(1))
        .tap(() => redis.fsortBust(idSetKey, Date.now()))
        .then(keys => Promise.join(
           redis.zcard(`${idSetKey}::${mod.FSORT_TEMP_KEYSET}`),
           redis.exists(keys)
        ))
        .spread((cardinality, keys) => {
           expect(cardinality).to.be.eq(0);
           expect(keys).to.be.eq(0);
        });
    });

    it('should invalidate cache: sort + filter', function test() {
       const fieldName = 'fieldExists';
       const filter = mod.filter({
        [fieldName]: { exists: '1' }
       });

      return redis.fsort(idSetKey, metaKeyPattern, null, 'ASC', filter, Date.now())
        .then(() => redis.zrangebyscore(`${idSetKey}::${mod.FSORT_TEMP_KEYSET}`, '-inf', '+inf'))
        .tap(keys => expect(keys.length).to.be.eq(2))
        .tap(() => redis.fsortBust(idSetKey, Date.now()))
        .then(keys => Promise.join(
           redis.zcard(`${idSetKey}::${mod.FSORT_TEMP_KEYSET}`),
           redis.exists(keys)
        ))
        .spread((cardinality, keys) => {
           expect(cardinality).to.be.eq(0);
           expect(keys).to.be.eq(0);
        });
    });
  });

  describe('sort/pagination only', function sortSuite() {
    const offset = 10;
    const limit = 20;

    it('sorts: asc', function test() {
      return redis.fsort(idSetKey, null, null, 'ASC', '{}', Date.now())
        .tap(sortedBy(comparatorASC, prepopulateDataLength));
    });

    it('sorts: desc', function test() {
      return redis.fsort(idSetKey, null, null, 'DESC', '{}', Date.now())
        .tap(sortedBy(comparatorDESC, prepopulateDataLength));
    });

    it('pagination: asc', function test() {
      return redis.fsort(idSetKey, null, null, 'ASC', '{}', Date.now(), offset, limit)
        .tap(sortedBy(comparatorASC, prepopulateDataLength))
        .then(ids => {
          expect(ids).to.have.length.of(20);
          expect(ids).to.be.deep.eq(insertedIds.slice(offset, offset + limit));
        });
    });

    it('pagination: desc', function test() {
      return redis.fsort(idSetKey, null, null, 'DESC', '{}', Date.now(), offset, limit)
        .tap(sortedBy(comparatorDESC, prepopulateDataLength))
        .then(ids => {
          expect(ids).to.have.length.of(20);
          expect(ids).to.be.deep.eq(invertedIds.slice(offset, offset + limit));
        });
    });
  });

  describe('sort/pagination only, external numeric field', function sortSuite() {
    const offset = 10;
    const limit = 20;
    const field = 'age';
    const map = id => metadata[id][field];

    function comparatorAge(direction) {
      return function compare(a, b) {
        const result = map(a) - map(b);
        return direction * result;
      };
    };

    let externallySortedIdsASC;
    let externallySortedIdsDESC;
    before(function pretest() {
      externallySortedIdsASC = [].concat(insertedIds).sort(comparatorAge(1));;
      externallySortedIdsDESC = [].concat(insertedIds).sort(comparatorAge(-1));
    });

    it('sorts: asc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, field, 'ASC', '{}', Date.now())
        .tap(sortedBy(comparatorAge(1), prepopulateDataLength, field));
    });

    it('sorts: desc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, field, 'DESC', '{}', Date.now())
        .tap(sortedBy(comparatorAge(-1), prepopulateDataLength, field));
    });

    it('pagination: asc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, field, 'ASC', '{}', Date.now(), offset, limit)
        .tap(sortedBy(comparatorAge(1), prepopulateDataLength, field))
        .then(ids => {
          expect(ids).to.have.length.of(20);
          expect(ids.map(map)).to.be.deep.eq(externallySortedIdsASC.slice(offset, offset + limit).map(map));
        });
    });

    it('pagination: desc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, field, 'DESC', '{}', Date.now(), offset, limit)
        .tap(sortedBy(comparatorAge(-1), prepopulateDataLength, field))
        .then(ids => {
          expect(ids).to.have.length.of(20);
          expect(ids.map(map)).to.be.deep.eq(externallySortedIdsDESC.slice(offset, offset + limit).map(map));
        });
    });
  });

  describe('sort/pagination only, external string field', function sortSuite() {
    const offset = 10;
    const limit = 20;
    const field = 'favColor';
    const map = id => metadata[id][field];

    function comparatorAlphanum(direction) {
      return function compare(a, b) {
        const result = map(a).localeCompare(map(b));
        return direction * result;
      };
    };

    let externallySortedIdsASC;
    let externallySortedIdsDESC;
    before(function pretest() {
      externallySortedIdsASC = [].concat(insertedIds).sort(comparatorAlphanum(1));
      externallySortedIdsDESC = [].concat(insertedIds).sort(comparatorAlphanum(-1));
    });

    it('sorts: asc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, field, 'ASC', '{}', Date.now())
        .tap(sortedBy(comparatorAlphanum(1), prepopulateDataLength, field));
    });

    it('sorts: desc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, field, 'DESC', '{}', Date.now())
        .tap(sortedBy(comparatorAlphanum(-1), prepopulateDataLength, field));
    });

    it('pagination: asc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, field, 'ASC', '{}', Date.now(), offset, limit)
        .tap(sortedBy(comparatorAlphanum(1), prepopulateDataLength, field))
        .then(ids => {
          expect(ids).to.have.length.of(20);
          expect(ids.map(map)).to.be.deep.eq(externallySortedIdsASC.slice(offset, offset + limit).map(map));
        });
    });

    it('pagination: desc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, field, 'DESC', '{}', Date.now(), offset, limit)
        .tap(sortedBy(comparatorAlphanum(-1), prepopulateDataLength, field))
        .then(ids => {
          expect(ids).to.have.length.of(20);
          expect(ids.map(map)).to.be.deep.eq(externallySortedIdsDESC.slice(offset, offset + limit).map(map));
        });
    });
  });

  describe('filter/exists', function sortFilterExistsEmptySuite() {
    const fieldName = 'fieldExists';
    const hasOwnProperty = Object.prototype.hasOwnProperty;
    const jsFilter = id => hasOwnProperty.call(metadata[id], fieldName);
    const offset = 0;
    const limit = 10;
    const filter = mod.filter({
      [fieldName]: { exists: '1' }
    });

    let filteredIds;
    let invertedFilteredIds;
    let filteredLength;

    before(function pretest() {
      filteredIds = insertedIds.filter(jsFilter);
      invertedFilteredIds = invertedIds.filter(jsFilter);
      filteredLength = filteredIds.length;
    });

    it('sorts: asc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'ASC', filter, Date.now())
        .tap(sortedBy(comparatorASC, filteredLength));
    });

    it('sorts: desc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'DESC', filter, Date.now())
        .tap(sortedBy(comparatorDESC, filteredLength));
    });

    it('pagination: asc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'ASC', filter, Date.now(), offset, limit)
        .tap(sortedBy(comparatorASC, filteredLength))
        .then(ids => {
          expect(ids).to.have.length.of(limit);
          expect(ids).to.be.deep.eq(filteredIds.slice(offset, offset + limit));
        });
    });

    it('pagination: desc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'DESC', filter, Date.now(), offset, limit)
        .tap(sortedBy(comparatorDESC, filteredLength))
        .then(ids => {
          expect(ids).to.have.length.of(limit);
          expect(ids).to.be.deep.eq(invertedFilteredIds.slice(offset, offset + limit));
        });
    });
  });

  describe('filter/isempty', function sortFilterExistsEmptySuite() {
    const fieldName = 'fieldExists';
    const hasOwnProperty = Object.prototype.hasOwnProperty;
    const jsFilter = id => !hasOwnProperty.call(metadata[id], fieldName);
    const offset = 0;
    const limit = 10;
    const filter = mod.filter({
      [fieldName]: { isempty: '1' }
    });

    let filteredIds;
    let invertedFilteredIds;
    let filteredLength;

    before(function pretest() {
      filteredIds = insertedIds.filter(jsFilter);
      invertedFilteredIds = invertedIds.filter(jsFilter);
      filteredLength = filteredIds.length;
    });

    it('sorts: asc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'ASC', filter, Date.now())
        .tap(sortedBy(comparatorASC, filteredLength));
    });

    it('sorts: desc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'DESC', filter, Date.now())
        .tap(sortedBy(comparatorDESC, filteredLength));
    });

    it('pagination: asc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'ASC', filter, Date.now(), offset, limit)
        .tap(sortedBy(comparatorASC, filteredLength))
        .then(ids => {
          expect(ids).to.have.length.of(limit);
          expect(ids).to.be.deep.eq(filteredIds.slice(offset, offset + limit));
        });
    });

    it('pagination: desc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'DESC', filter, Date.now(), offset, limit)
        .tap(sortedBy(comparatorDESC, filteredLength))
        .then(ids => {
          expect(ids).to.have.length.of(limit);
          expect(ids).to.be.deep.eq(invertedFilteredIds.slice(offset, offset + limit));
        });
    });
  });

  describe('filter/any', function sortFilterExistsEmptySuite() {
    const fieldName = 'age';
    const hasOwnProperty = Object.prototype.hasOwnProperty;
    const jsFilter = id => {
      const userAge = metadata[id][fieldName];
      return ((userAge >= 10 && userAge <= 18) ||
        (userAge >= 35 && userAge <= 45));
    };
    const offset = 0;
    const limit = 10;
    const filter = mod.filter({
      [fieldName]: { any: [
        { gte: 10, lte: 18 },
        { gte: 35, lte: 45 },
      ]}
    });

    let filteredIds;
    let invertedFilteredIds;
    let filteredLength;

    before(function pretest() {
      filteredIds = insertedIds.filter(jsFilter);
      invertedFilteredIds = invertedIds.filter(jsFilter);
      filteredLength = filteredIds.length;
    });

    it('sorts: asc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'ASC', filter, Date.now())
        .tap(sortedBy(comparatorASC, filteredLength));
    });

    it('sorts: desc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'DESC', filter, Date.now())
        .tap(sortedBy(comparatorDESC, filteredLength));
    });

    it('pagination: asc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'ASC', filter, Date.now(), offset, limit)
        .tap(sortedBy(comparatorASC, filteredLength))
        .then(ids => {
          expect(ids).to.have.length.of(limit);
          expect(ids).to.be.deep.eq(filteredIds.slice(offset, offset + limit));
        });
    });

    it('pagination: desc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'DESC', filter, Date.now(), offset, limit)
        .tap(sortedBy(comparatorDESC, filteredLength))
        .then(ids => {
          expect(ids).to.have.length.of(limit);
          expect(ids).to.be.deep.eq(invertedFilteredIds.slice(offset, offset + limit));
        });
    });
  });

  describe('sort/id filter only', function sortFilterIdSuite() {
    const filterIdString = 'd-9';
    const jsFilter = a => a.indexOf(filterIdString) >= 0;
    const offset = 10;
    const limit = 20;
    const filter = mod.filter({ '#': filterIdString });
    let filteredIds;
    let invertedFilteredIds;
    let filteredLength;

    before(function pretest() {
      filteredIds = insertedIds.filter(jsFilter);
      invertedFilteredIds = invertedIds.filter(jsFilter);
      filteredLength = filteredIds.length;
    });

    it('sorts: asc', function test() {
      return redis.fsort(idSetKey, null, null, 'ASC', filter, Date.now())
        .tap(sortedBy(comparatorASC, filteredLength));
    });

    it('sorts: desc', function test() {
      return redis.fsort(idSetKey, null, null, 'DESC', filter, Date.now())
        .tap(sortedBy(comparatorDESC, filteredLength));
    });

    it('pagination: asc', function test() {
      return redis.fsort(idSetKey, null, null, 'ASC', filter, Date.now(), offset, limit)
        .tap(sortedBy(comparatorASC, filteredLength))
        .then(ids => {
          expect(ids).to.have.length.of(20);
          expect(ids).to.be.deep.eq(filteredIds.slice(offset, offset + limit));
        });
    });

    it('pagination: desc', function test() {
      return redis.fsort(idSetKey, null, null, 'DESC', filter, Date.now(), offset, limit)
        .tap(sortedBy(comparatorDESC, filteredLength))
        .then(ids => {
          expect(ids).to.have.length.of(20);
          expect(ids).to.be.deep.eq(invertedFilteredIds.slice(offset, offset + limit));
        });
    });
  });

  describe('sort + complex filter suite', function sortComplexFilterSuite() {
    const filterIdString = 'id-';
    const name = 'an';
    const age = 15;
    const fields = ['surname', 'favColor', 'email'];
    const color = 'ue';
    const jsFilter = a => {
      const meta = metadata[a];
      return a.indexOf(filterIdString) >= 0 &&
        meta.name.toLowerCase().indexOf(name) >= 0 &&
        meta.age >= age &&
        meta.age <= age * 3 &&
        ld.some(ld.values(ld.pick(meta, fields)), it => it.indexOf(color) >= 0)
    };

    const offset = 10;
    const limit = 20;
    const filter = mod.filter({
      '#': filterIdString,
      '#multi': { fields, match: color },
      name,
      age: { gte: age, lte: age * 3 },
    });
    let filteredIds;
    let invertedFilteredIds;
    let filteredLength;

    before(function pretest() {
      filteredIds = insertedIds.filter(jsFilter);
      invertedFilteredIds = invertedIds.filter(jsFilter);
      filteredLength = filteredIds.length;
    });

    it('sorts: asc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'ASC', filter, Date.now())
        .tap(sortedBy(comparatorASC, filteredLength));
    });

    it('sorts: desc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'DESC', filter, Date.now())
        .tap(sortedBy(comparatorDESC, filteredLength));
    });

    it('pagination: asc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'ASC', filter, Date.now(), offset, limit)
        .tap(sortedBy(comparatorASC, filteredLength))
        .then(ids => {
          expect(ids).to.be.deep.eq(filteredIds.slice(offset, offset + limit));
        });
    });

    it('pagination: desc', function test() {
      return redis.fsort(idSetKey, metaKeyPattern, null, 'DESC', filter, Date.now(), offset, limit)
        .tap(sortedBy(comparatorDESC, filteredLength))
        .then(ids => {
          expect(ids).to.be.deep.eq(invertedFilteredIds.slice(offset, offset + limit));
        });
    });
  });

  describe('aggregate filter', function suite() {
    it('sums up age and returns key', function test() {
      const filter = mod.filter({
        age: { gte: 10, lte: 40 },
      });

      return redis
        .fsort(idSetKey, metaKeyPattern, 'age', 'DESC', filter, Date.now(), 0, 0, 30000, 1)
        .then((response) => {
          // key where the resulting data is stored
          expect(response).to.be.equal('fsort-test:id-set:DESC:fsort-test:*-metadata:age:{"age":{"gte":10,"lte":40}}');
          this.response = response;
        });
    });

    it('returns aggregate', function test() {
      const aggregate = mod.filter({
        age: 'sum',
      });

      return redis
        .fsortAggregate(this.response.slice(keyPrefix.length), metaKeyPattern, aggregate)
        .then(JSON.parse)
        .then((response) => {
          // this would average to 15000+ due to random ranges
          expect(response.age).to.be.gt(15000);
        });
    });
  });
});
