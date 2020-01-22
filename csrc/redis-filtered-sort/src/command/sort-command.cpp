#include "sort-command.hpp"

#include "string"
#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "util/data.hpp"
#include "util/filter/filter.hpp"
#include "util/filter/parser.hpp"

#include <iostream>
#include <boost/log/trivial.hpp>

using namespace ms;

SortCommand::SortCommand(SortArgs args)
{
  this->args = args;
  this->init();
}

SortCommand::~SortCommand()
{
}

// TODO CHANGEME
void mlog(string message)
{
  std::cout << "LOG--->" << message << "\n";
}

void SortCommand::init()
{
   this->tempKeysSet = boost::str(boost::format("%s::fsort_temp_keys") % this->args.idSet);
  try
  {
    stringstream jsonText;
    jsonText << args.filter;
    boost::property_tree::json_parser::read_json(jsonText, this->jsonFilters);
    this->filters = FilterParser::ParseJsonTree(this->jsonFilters);
  }
  catch (exception &e)
  {
    mlog(e.what());
  }
  std::vector<string> ffKeyOpts = {args.idSet, args.order};
  std::vector<string> psKeyOpts = {args.idSet, args.order};

  size_t filterSize = this->jsonFilters.size();

  if (!args.metaKey.empty())
  {
    if (args.hashKey != "")
    {
      ffKeyOpts.push_back(args.metaKey);
      psKeyOpts.push_back(args.metaKey);

      ffKeyOpts.push_back(args.hashKey);
      psKeyOpts.push_back(args.hashKey);
    }

    if (filterSize > 0)
    {
      ffKeyOpts.push_back(args.filter);
    }
  }
  else if (filterSize >= 1)
  {
    boost::optional<string> idFilter = this->jsonFilters.get<string>("#");
    if (!idFilter.value().empty())
    {
      this->idFilter = idFilter.value();
      ffKeyOpts.push_back(idFilter.value());
    }
  }

  this->fflKey = boost::join(ffKeyOpts, ":");
  this->pssKey = boost::join(psKeyOpts, ":");
  BOOST_LOG_TRIVIAL(debug) << "psskey " << this->pssKey;
  BOOST_LOG_TRIVIAL(debug) << "fflkey " << this->fflKey;
}

int SortCommand::execute(redis::Context &redis)
{
  auto redisCommand = redis.getCommand();
  auto redisData = redis.getData();

  int pssKType = redisCommand.type(this->pssKey);
  int fflKType = redisCommand.type(this->fflKey);
  auto args = this->args;

  BOOST_LOG_TRIVIAL(debug) << "Starting command";

  if (fflKType == REDISMODULE_KEYTYPE_LIST)
  {
    BOOST_LOG_TRIVIAL(debug) << "respond FFL key " << this->fflKey;

    redisCommand.pexpire(this->tempKeysSet, args.expire);

    redisData.registerCaches(this->pssKey, this->tempKeysSet, args.curTime, args.expire);
    redisData.registerCaches(this->fflKey, this->tempKeysSet, args.curTime, args.expire);
    redis.respondList(this->fflKey, args.offset, args.limit);
    return 0;
  }

  Data sortData = Data(redis, args.metaKey);

  if (pssKType == REDISMODULE_KEYTYPE_LIST)
  {
    //Respond pss
    if (this->fflKey.compare(this->pssKey) == 0)
    {
      BOOST_LOG_TRIVIAL(debug) << "No filter request respond presorted set";
      redisCommand.pexpire(this->tempKeysSet, args.expire);

      redisData.registerCaches(this->pssKey, this->tempKeysSet, args.curTime, args.expire);
      return redis.respondList(this->pssKey, args.offset, args.limit);
    }
    //Load pss cache if sort requested
    sortData.loadList(this->pssKey);
    if (sortData.size() == 0)
    {
      BOOST_LOG_TRIVIAL(warning) << "No data in pss";
      //exit
      return 0;
    }
  }
  else
  {
    sortData.loadSet(args.idSet);
    BOOST_LOG_TRIVIAL(debug) << "Loaded set " << args.idSet << " size " << sortData.size();

    if (sortData.size() > 0)
    {
      if (args.metaKey.empty() || args.hashKey.empty())
      {
        sortData.sort(args.order);
      }
      else
      {
        std::cerr << "Sorting " << sortData.size() << "\n";
        sortData.sortMeta(args.hashKey, args.order);
      }

      if (redisCommand.type(this->pssKey) == REDISMODULE_KEYTYPE_EMPTY)
      {
        sortData.save(this->pssKey);
      }
      else
      {
        //RM_LOG_VERBOSE(this->redisContext,"Key Already created key %s ommiting write", this->pssKey.c_str());
        std::cerr << "Skip write " << this->pssKey;
      }

      redisCommand.pexpire(this->tempKeysSet, args.expire);
      // TODO FIXME
      redisData.registerCaches(this->pssKey, this->tempKeysSet, args.curTime, args.expire);
    }
    else
    {
      if (args.keyOnly == 1)
      {
        redis.respondString(this->pssKey);
        return REDISMODULE_OK;
      }

      redis.respondLong(0);
      return REDISMODULE_OK;
    }

    if (this->fflKey.compare(this->pssKey) == 0)
    {
      return redis.respondList(this->pssKey, args.offset, args.limit);
    }

    redisCommand.pexpire(this->tempKeysSet, args.expire);
    redisData.registerCaches(this->pssKey, this->tempKeysSet, args.curTime, args.expire);
  }

  vector<string> filteredData;
  if (args.metaKey.empty())
  {
    filteredData = sortData.filter(this->idFilter);
  }
  else
  {
    filteredData = sortData.filterMeta(this->filters);
  }
  BOOST_LOG_TRIVIAL(debug) << "Filtered records count " << filteredData.size();
  if (filteredData.size() > 0)
  {
    std::cerr << "Save filtered: " << filteredData.size() << "\n";
    if (redisCommand.type(this->fflKey) == REDISMODULE_KEYTYPE_EMPTY)
    {
      sortData.save(this->fflKey, filteredData);
    }
    else
    {
      // RM_LOG_VERBOSE(this->redisContext,"Filter Key already created created key %s: ommiting write", this->fflKey.c_str());
      std::cerr << "Skip write " << this->fflKey;
    }
    redisCommand.pexpire(this->tempKeysSet, args.expire);
    // TODO FIXME
    redisData.registerCaches(this->fflKey, this->tempKeysSet, args.curTime, args.expire);
    return redis.respondList(this->fflKey, args.offset, args.limit);
  }

  if (args.keyOnly == 1)
  {
    redis.respondString(this->fflKey);
    return REDISMODULE_OK;
  }

  redis.respondEmptyArray();

  return REDISMODULE_OK;
}