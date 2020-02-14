# FilterSortModule redis commands
Module exports this Redis commands

**Please use `json Objects` instead of `json Arrays`**

## Redis Cluster NOTE
In cluster environment all keys used by this command must have same *fix with `{}` in key names

## fsort 
Sorts, filters and paginates provided SET `idSetKey` using data from `metaKeyPattern`.`hashKey` using `sortOrder`,`filter` and stores processed data for `expire` seconds

**Usage**:
```console
127.0.0.1:6379:> cfsort idSetKey metaKeyPattern hashKey sortOrder filter(jsonFormattedString) curTime [offset:int] [limit:int] [expire] [keyOnly]
```
- **idSetKey**: Redis SET containing id values
- **metaKeyPattern**: Pattern to find HASHes of record, "*" is replaced by id from `idSetKey`, eg: _*-metakey_ produce  _id-231-metakey_
- **hashKey**: field name from HASHkey formed from `metakeyPattern`
- **filter**: Json string with filters
- **curTime**: Current time in milliseconds
- **expire** : TTL for generated cacheds in milliseconds(default 30 sec)
- **keyOnly**: Forces command to return Redis `key`, containing result data. Usable for [`fsortBust`](#sortbust) method


## fsortBust :
Checking caches for specified `key` that was created in result of [fsort](#fsort) method. Allows forcing cache invalidation.

**Method usage**: 
```
fsortbust key time [expire]
```
- **key**: Redis LIST containing cached sorted or filtered values. You can obtain it by passing `keyOnly` parameter into [fsort](#fsort) command
- **time**: Current or desired time in milliseconds
- **expire** : TTL  in milliseconds(default 30 sec)


## fsortAggregate:
Groups elements by `aggregateParam` from set `idSet` using meta available under `metaKeyPattern` and reports count of each type of criteria.

Currently supports only `sum`;

* **Method usage**:
```
cfsortAggregate idSetKey metaKeyPattern aggregateParam`
```
- **idSetKey**: Redis SET containing id values
- **metaKeyPattern**: Pattern to find HASHes of record, "*" is replaced by id from `idSetKey`, eg: _*-metakey_ produce  _id-231-metakey_
- **aggregateParam** : JsonString containing aggregate criteria.
