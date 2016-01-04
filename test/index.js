'use strict';

const Promise = require('bluebird');
const Redis = require('ioredis');
const { expect } = require('chai');
const faker = require('faker');
const ld = require('lodash');

describe('filtered sort suite', function suite() {
  const idSetKey = 'id-set';
  const redis = new Redis(process.env.REDIS_PORT_6379_TCP_PORT, process.env.REDIS_PORT_6379_TCP_ADDR);
  const mod = require('../index.js');
  const escape = mod.escape;
  const prepopulateDataLength = parseInt(process.env.PREPOPULAR || 1000, 10);
  const insertedIds = [];
  const invertedIds = [];
  mod.attach(redis, 'fsort');

  function metadataKey(id) {
    return `${id}-metadata`;
  }

  function generateUser(n) {
    const id = `id-${n}`;

    insertedIds.push(id);
    invertedIds.push(id);

    return Promise.join(
      redis.sadd(idSetKey, id),
      redis.hmset(metadataKey(id), {
        email: faker.internet.email(),
        name: faker.name.firstName(),
        surname: faker.name.lastName(),
        balance: faker.finance.amount(),
        favColor: faker.commerce.color(),
        country: faker.address.countryCode(),
      })
    );
  }

  function comparatorASC(a, b) {
    return a.localeCompare(b);
  }

  function comparatorDESC(a, b) {
    return b.localeCompare(a);
  }

  function sortedBy(comparator, listLength) {
    return function sorted(ids) {
      const length = ids.pop();
      if (typeof listLength !== 'undefined') {
        expect(parseInt(length, 10)).to.be.eq(listLength);
      }
      const copy = [].concat(ids);
      copy.sort(comparator);
      expect(copy).to.be.deep.eq(ids);
    };
  }

  before('populate data', function pretest() {
    this.timeout(5000);
    const promises = ld.times(prepopulateDataLength, generateUser);

    // alphanum sort, use locale based sorting
    insertedIds.sort(comparatorASC);
    invertedIds.sort(comparatorDESC);

    return Promise.all(promises);
  });

  describe('sort/pagination only', function sortSuite() {
    const offset = 10;
    const limit = 20;

    it('sorts: asc', function test() {
      return redis.fsort(idSetKey, null, null, 'ASC', '{}')
        .tap(sortedBy(comparatorASC, prepopulateDataLength));
    });

    it('sorts: desc', function test() {
      return redis.fsort(idSetKey, null, null, 'DESC', '{}')
        .tap(sortedBy(comparatorDESC, prepopulateDataLength));
    });

    it('pagination: asc', function test() {
      return redis.fsort(idSetKey, null, null, 'ASC', '{}', offset, limit)
        .tap(sortedBy(comparatorASC, prepopulateDataLength))
        .then(ids => {
          expect(ids).to.have.length.of(20);
          expect(ids).to.be.deep.eq(insertedIds.slice(offset, offset + limit));
        });
    });

    it('pagination: desc', function test() {
      return redis.fsort(idSetKey, null, null, 'DESC', '{}', offset, limit)
        .tap(sortedBy(comparatorDESC, prepopulateDataLength))
        .then(ids => {
          expect(ids).to.have.length.of(20);
          expect(ids).to.be.deep.eq(invertedIds.slice(offset, offset + limit));
        });
    });
  });

  describe('sort/id filter only', function sortFilterIdSuite() {
    const filterIdString = 'd-9';
    const jsFilter = a => a.indexOf(filterIdString) >= 0;
    const offset = 10;
    const limit = 20;
    const filter = JSON.stringify({ '#': escape(filterIdString) });
    let filteredIds;
    let invertedFilteredIds;
    let filteredLength;

    before(function pretest() {
      filteredIds = insertedIds.filter(jsFilter);
      invertedFilteredIds = invertedIds.filter(jsFilter);
      filteredLength = filteredIds.length;
    });

    it('sorts: asc', function test() {
      return redis.fsort(idSetKey, null, null, 'ASC', filter)
        .tap(sortedBy(comparatorASC, filteredLength));
    });

    it('sorts: desc', function test() {
      return redis.fsort(idSetKey, null, null, 'DESC', filter)
        .tap(sortedBy(comparatorDESC, filteredLength));
    });

    it('pagination: asc', function test() {
      return redis.fsort(idSetKey, null, null, 'ASC', filter, offset, limit)
        .tap(sortedBy(comparatorASC, filteredLength))
        .then(ids => {
          expect(ids).to.have.length.of(20);
          expect(ids).to.be.deep.eq(filteredIds.slice(offset, offset + limit));
        });
    });

    it('pagination: desc', function test() {
      return redis.fsort(idSetKey, null, null, 'DESC', filter, offset, limit)
        .tap(sortedBy(comparatorDESC, filteredLength))
        .then(ids => {
          expect(ids).to.have.length.of(20);
          expect(ids).to.be.deep.eq(invertedFilteredIds.slice(offset, offset + limit));
        });
    });
  });

  describe('sort + complex filter suite', function sortComplexFilterSuite() {

  });
});
