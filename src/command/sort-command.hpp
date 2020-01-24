#ifndef __SORT_COMMAND_H
#define __SORT_COMMAND_H

#include "cmd-args.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "redis/context.hpp"
#include "util/filter/filter.hpp"
#include "util/data.hpp"
#include "redis/redismodule.h"
#include "util/task-registry.h"

namespace ms {
  using namespace std;

  class SortCommand {
  private:
    SortArgs args;
    string tempKeysSet;
    string fflKey;
    string pssKey;
    boost::property_tree::ptree jsonFilters;
    FilterInterface *filters;
    string idFilter;
    redis::Context &redis;
    TaskRegistry &taskRegistry;

  public:
    SortCommand(SortArgs args, redis::Context &redis, TaskRegistry &registry);

    ~SortCommand();

    string getTaskDescriptor();

    void init();

    void execute();

    void doWork();

    void respond(string key);

    void SortAndSaveData(ms::Data &sortData);

    void UpdateKeySetAndExpire(string key);
  };
} // namespace ms

#endif