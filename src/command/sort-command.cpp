#include "sort-command.hpp"

#include "string"
#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>

#include "util/data.hpp"
#include "util/filter/parser.hpp"

#include <iostream>

using namespace ms;

SortCommand::SortCommand(SortArgs args, redis::Context &redis) : redis(redis) {
  this->args = args;
  this->init();
}

SortCommand::~SortCommand() {
  delete filters;
}

void SortCommand::init() {
  this->tempKeysSet = boost::str(boost::format("%s::fsort_temp_keys") % this->args.idSet);
  stringstream jsonText;
  jsonText << args.filter;
  boost::property_tree::json_parser::read_json(jsonText, this->jsonFilters);
  this->filters = FilterParser::ParseJsonTree(this->jsonFilters);

  std::vector<string> ffKeyOpts = {args.idSet, args.order};
  std::vector<string> psKeyOpts = {args.idSet, args.order};

  size_t filterSize = this->jsonFilters.size();

  if (!args.metaKey.empty()) {
    if (args.hashKey != "") {
      ffKeyOpts.push_back(args.metaKey);
      psKeyOpts.push_back(args.metaKey);

      ffKeyOpts.push_back(args.hashKey);
      psKeyOpts.push_back(args.hashKey);
    }

    if (filterSize > 0) {
      ffKeyOpts.push_back(args.filter);
    }
  } else if (filterSize >= 1) {
    boost::optional<string> idFilter = this->jsonFilters.get<string>("#");
    if (!idFilter.value().empty()) {
      this->idFilter = idFilter.value();
      ffKeyOpts.push_back(idFilter.value());
    }
  }

  this->fflKey = boost::join(ffKeyOpts, ":");
  this->pssKey = boost::join(psKeyOpts, ":");
}

void SortCommand::respond(string key) {
  if (args.keyOnly) {
    redis.respondString(key);
  } else {
    redis.respondList(key, args.offset, args.limit);
  }
}

void SortCommand::execute() {
  auto redisCommand = redis.getCommand();
  auto redisData = redis.getData();

  int pssKType = redisCommand.type(pssKey);
  int fflKType = redisCommand.type(fflKey);

  if (fflKType == REDISMODULE_KEYTYPE_LIST) {
    redisCommand.pexpire(tempKeysSet, args.expire);

    redisData.registerCaches(pssKey, tempKeysSet, args.curTime, args.expire);
    redisData.registerCaches(fflKey, tempKeysSet, args.curTime, args.expire);
    return this->respond(fflKey);
  }

  Data sortData = Data(redis, args.metaKey);

  if (pssKType == REDISMODULE_KEYTYPE_LIST) {
    //Respond pss
    if (fflKey.compare(pssKey) == 0) {
      redisCommand.pexpire(tempKeysSet, args.expire);
      redisData.registerCaches(pssKey, tempKeysSet, args.curTime, args.expire);
      return this->respond(pssKey);
    }
    //Load pss cache if sort requested
    sortData.loadList(pssKey);
  } else {
    sortData.loadSet(args.idSet);

    if (sortData.size() > 0) {
      if (args.metaKey.empty() || args.hashKey.empty()) {
        sortData.sort(args.order);
      } else {
        sortData.sortMeta(args.hashKey, args.order);
      }

      if (redisCommand.type(pssKey) == REDISMODULE_KEYTYPE_EMPTY) {
        sortData.save(pssKey);
      } else {
        std::cerr << "Skip write " << pssKey;
      }

      redisCommand.pexpire(tempKeysSet, args.expire);
      redisData.registerCaches(pssKey, tempKeysSet, args.curTime, args.expire);
    } else {
      if (args.keyOnly == 1) {
        return redis.respondString(pssKey);
      }
      return redis.respondLong(0);
    }

    if (this->fflKey.compare(pssKey) == 0) {
      return this->respond(pssKey);
    }

    redisCommand.pexpire(tempKeysSet, args.expire);
    redisData.registerCaches(pssKey, tempKeysSet, args.curTime, args.expire);
  }

  vector<string> filteredData;

  if (args.metaKey.empty()) {
    filteredData = sortData.filter(idFilter);
  } else {
    filteredData = sortData.filterMeta(filters);
  }

  if (filteredData.size() > 0) {
    if (redisCommand.type(fflKey) == REDISMODULE_KEYTYPE_EMPTY) {
      sortData.save(fflKey, filteredData);
    } else {
      // RM_LOG_VERBOSE(this->redisContext,"Filter Key already created created key %s: ommiting write", this->fflKey.c_str());
      std::cerr << "Skip write " << fflKey;
    }
    redisCommand.pexpire(tempKeysSet, args.expire);
    redisData.registerCaches(fflKey, tempKeysSet, args.curTime, args.expire);
    return respond(fflKey);
  }

  if (args.keyOnly == 1) {
    redis.respondString(fflKey);
    return;
  }

  redis.respondEmptyArray();
}