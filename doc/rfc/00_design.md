# Proposal
As described in Redis Module API modules have more benefints than general LUA scripts. In current environment, lua script locking db for a long period. The main reason of this module: provide non db-locking access for time-consuming operations.

# Design
* Projects consists from one redis module `filtered_sort` and node.js wrapper `redis-filtered-sort`.

* By default, module going to have 2 build options: Docker based build for redis:5.0-alpine and local system `gcc`

* Exported functions for dev purpose prefixed with "c" prefix

* Current module behaviour copied from existing scripts for compat with lua.

* in Redis Cluster environment: all keys and sets MUST use {prefix} notation, to be stored on same node. Otherwise module won't work correctly.


## Expected API and behaviour
Module will export into Redis command namespace these commands:
#### cfsort 
Sorts, filters and paginates provided SET `idSetKey` using data from `metaKeyPattern`.`hashKey` using `sortOrder`,`filter` and stores processed data for `expire` seconds

* **Method usage**: `cfsort idSetKey metaKeyPattern hashKey sortOrder filter(jsonFormattedString) curTime:unixtime [offset:int] [limit:int] [expire:int - ttl in seconds] [keyOnly]`;
    - **idSetKey**: Redis SET containing id values
    - **metaKeyPattern**: Pattern to find HASHes of record, "*" is replaced by id from `idSetKey`, eg: _*-metakey_ produce  _id-231-metakey_
    - **hashKey**: field name from HASHkey formed from `metakeyPattern`
    - **filter**: Json string with filters
* **General behaviour**: During work process function going to store postprocessed data into 2 temporary sets(sorted and filterd) in format `{idSetKey}:{order}:{metaKey}:{hasKey}:{filterString}` and reuse them while `ttl` not past. 

    Creates index of this sets in ZSET `tempKeysSet`.
    > _Definitely I don't understand why this needed, but we have cacheBuster_ in lua. #SeeLater

    - If `metaKeyPattern` and `hashKey` not provided, function going to use `idSetKey` values for sorting, otherwise fetch data from `metaKeyPattern.gsub("*", idSetMemberValue).providedHashKey` and use them for sort.
    
    - Same as prev behaviour going to be used in filtering process.

    - If more than one filter provided: `filterString` will contain only "#" filter;


### cfsortBust :
Checking caches for specified `idSetKey` that was created in result of `cfsort` method work. Deletes outdated tempKeys from ZSET `tempKeysSet`

**Method usage**: `cfsort idSetKey curTime:unixtime [expire:int]`

### cfsortAggregate:
Groups elements by `aggregateParam` from set `idSet` using meta available under `metaKeyPattern` and reports count of each type of criteria.

Currently supports only `sum`;

* **Method usage**: `cfsortAggregate idSetKey metaKeyPattern aggregateParam`
    - **idSetKey**: Redis SET containing id values
    - **metaKeyPattern**: Pattern to find HASHes of record, "*" is replaced by id from `idSetKey`, eg: _*-metakey_ produce  _id-231-metakey_
    - **aggregateParam** : JsonString containing aggregate criteria.
        ```Javascript
            {
                "fieldName": "sum"
            }
        ```


## Filter Format `filter` parameter:
Accepting `json` formated object:
```Javascript
    {
        "fieldToFiler": {
            //Filter params...
        }
        "fieldToFilter": "String to find"
    }
```
    
- **Available criteria**:
    - `lte`,`gte` - (int) value comparison
    - `eq`,`ne` - equal or not
    - `any` - Object containing criteria list to Exclude
    - `some` - Object containing criteria list to Include
    - `match` - String contains 
    - `exists` - if value exists

- **MultiField(#multi)**: Reserved field name for assigning criterias to multiple fields from `meta` record(currently supports only _String contains check_)
    ```Javascript
        {
            "#multi": {
                "fields": [ 
                    //Field list 
                    ...
                ],
                "match": "criteria"
            }
        }
    ```


**Example**
```Javascript
    {
        "metaFieldName": "contained string",
        "metaFieldName2": { //Filter params object
            "gte": 12,
            "lte": 13,
        },
        "metaFieldAnyParams": {
            "any": {
                "0": {
                    "gte": 12,
                    "lte": 13,
                },
                "1": {
                    "gte": 28,
                    "lte": 30,
                }
            }
        },


    }
    ```
        

