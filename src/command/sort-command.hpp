#ifndef __SORT_COMMAND_H
#define __SORT_COMMAND_H

#include "cmd-args.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "redis/context.hpp"
#include "util/filter/filter.hpp"
#include "redis/redismodule.h"

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

  public:
    SortCommand(SortArgs args, redis::Context &redis);

    ~SortCommand();

    void init();

    void execute();

    void respond(string key);
  };
} // namespace ms

#endif