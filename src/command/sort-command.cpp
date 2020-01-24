#include "sort-command.hpp"

#include "string"
#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>

#include "util/filter/parser.hpp"

#include <iostream>

using namespace ms;

SortCommand::SortCommand(SortArgs args, redis::Context &redis, TaskRegistry &registry) : args(args), redis(redis), taskRegistry(registry) {
  init();
}

SortCommand::~SortCommand() {
  delete filters;
}

string SortCommand::getTaskDescriptor() {
  return pssKey.compare(fflKey) != 0 ? pssKey : boost::str(boost::format("%s::%s") % pssKey % fflKey);
}

void SortCommand::init() {
  tempKeysSet = boost::str(boost::format("%s::fsort_temp_keys") % args.idSet);

  stringstream jsonText;
  jsonText << args.filter;
  boost::property_tree::json_parser::read_json(jsonText, jsonFilters);
  filters = FilterParser::ParseJsonTree(jsonFilters);

  std::vector<string> ffKeyOpts = {args.idSet, args.order};
  std::vector<string> psKeyOpts = {args.idSet, args.order};

  size_t filterSize = jsonFilters.size();

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
    boost::optional<string> idFilter = jsonFilters.get<string>("#");
    if (!idFilter.value().empty()) {
      idFilter = idFilter.value();
      ffKeyOpts.push_back(idFilter.value());
    }
  }

  fflKey = boost::join(ffKeyOpts, ":");
  pssKey = boost::join(psKeyOpts, ":");
}

void SortCommand::respond(string key) {
  if (args.keyOnly) {
    redis.respondString(key);
  } else {
    redis.respondList(key, args.offset, args.limit);
  }
}

void SortCommand::SortAndSaveData(Data &sortData) {
  auto redisCommand = redis.getCommand();
  auto redisData = redis.getData();

  if (args.metaKey.empty() || args.hashKey.empty()) {
    sortData.sort(args.order);
  } else {
    sortData.sortMeta(args.hashKey, args.order);
  }

  sortData.save(pssKey);

  redisCommand.pexpire(tempKeysSet, args.expire);
  redisData.registerCaches(pssKey, tempKeysSet, args.curTime, args.expire);
}

void SortCommand::UpdateKeySetAndExpire(string key) {
  auto redisCommand = redis.getCommand();
  auto redisData = redis.getData();

  redisCommand.pexpire(tempKeysSet, args.expire);
  redisData.registerCaches(key, tempKeysSet, args.curTime, args.expire);
}

void SortCommand::doWork() {
  auto redisCommand = redis.getCommand();
  auto redisData = redis.getData();

  int pssKType = redisCommand.type(pssKey);
  int fflKType = redisCommand.type(fflKey);

  if (fflKType == REDISMODULE_KEYTYPE_LIST) {
    redisCommand.pexpire(tempKeysSet, args.expire);
    redisData.registerCaches(pssKey, tempKeysSet, args.curTime, args.expire);
    redisData.registerCaches(fflKey, tempKeysSet, args.curTime, args.expire);
    return respond(fflKey);
  }

  Data sortData = Data(redis, args.metaKey);

  if (pssKType == REDISMODULE_KEYTYPE_LIST) {
    if (fflKey.compare(pssKey) == 0) {
      UpdateKeySetAndExpire(pssKey);
      return respond(pssKey);
    }
    sortData.loadList(pssKey);
  } else {
    sortData.loadSet(args.idSet);
  }

  if (sortData.size() == 0) {
    if (args.keyOnly == 1) {
      return redis.respondString(pssKey);
    }
    return redis.respondLong(0);
  }

  SortAndSaveData(sortData);
  UpdateKeySetAndExpire(pssKey);

  if (fflKey.compare(pssKey) == 0) {
    return respond(pssKey);
  }

  vector<string> filteredData;

  filteredData = args.metaKey.empty() ? sortData.filter(idFilter) : sortData.filterMeta(filters);

  if (filteredData.size() == 0) {
    if (args.keyOnly == 1) {
      redis.respondString(fflKey);
      return;
    }
    redis.respondEmptyArray();
    return;
  }

  sortData.save(fflKey, filteredData);
  UpdateKeySetAndExpire(fflKey);
  return respond(fflKey);
}

void SortCommand::execute() {
  auto taskDescriptor = getTaskDescriptor();

  if (taskRegistry.taskExists(taskDescriptor) || taskRegistry.taskExists(pssKey)) {
    auto boundProcessCommand = bind(&SortCommand::doWork, this);
    taskRegistry.registerWaiter(taskDescriptor, [&boundProcessCommand]() {
      boundProcessCommand();
    });
  } else {
    taskRegistry.addTask(taskDescriptor);
    this->doWork();
    if (pssKey.compare(fflKey) == 0) {
      taskRegistry.notifyAndDelete(taskDescriptor);
    } else {
      taskRegistry.notifyAndDelete(pssKey);
      taskRegistry.notifyAndDelete(taskDescriptor);
    }

  }
}