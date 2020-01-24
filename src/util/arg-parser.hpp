#ifndef __ARG_PARSER_H
#define __ARG_PARSER_H

#include "../command/cmd-args.hpp"
#include "redis/redismodule.h"

namespace ms {
  using namespace std;

  class arg {

  public:
    arg();

    ~arg();

    SortArgs parseSortCmdArgs(RedisModuleString **, int);

    BustArgs parseBustCmdArgs(RedisModuleString **, int);

    AggregateArgs parseAggregateCmdArgs(RedisModuleString **, int);
  };

} // namespace ms

#endif