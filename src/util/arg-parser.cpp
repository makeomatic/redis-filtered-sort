

#ifndef __ARG_PARSER_H
#define __ARG_PARSER_H

#include "command/cmd-args.hpp"
#include "redis/redismodule.h"
#include "redis/util.cpp"

namespace ms {
  using namespace redis;

  class ArgParser {

  public:
    ArgParser() = default;;

    ~ArgParser() = default;;

    static SortArgs parseSortCmdArgs(RedisModuleString **argv, int argc) {
      SortArgs args;
      args.offset = 0;
      args.limit = 0;
      args.expire = 30000;

      args.idSet = Utils::readString(argv[1]);
      args.metaKey = Utils::readString(argv[2]);
      args.hashKey = Utils::readString(argv[3]);
      args.filter = Utils::readString(argv[5]);
      args.curTime = Utils::readLong(argv[6], 0);
      args.order = Utils::readString(argv[4]);

      if (argc > 7)
        args.offset = Utils::readLong(argv[7], 0);
      if (argc > 8)
        args.limit = Utils::readLong(argv[8], 10);
      if (argc > 9)
        args.expire = Utils::readLong(argv[9], 30000);

      if (argc == 11 && argv[10] != nullptr) {
        args.keyOnly = 1;
      } else {
        args.keyOnly = 0;
      }
      return args;
    };

    static BustArgs parseBustCmdArgs(RedisModuleString **argv, int argc) {
      BustArgs args;
      args.idSet = Utils::readString(argv[1]);
      args.expire = 30000;
      args.curTime = Utils::readLong(argv[2], 0);

      if (argc > 3)
        args.expire = Utils::readLong(argv[3], 30000);
      return args;
    };

    static AggregateArgs parseAggregateCmdArgs(RedisModuleString **argv, int argc) {
      AggregateArgs args;
      args.pssKey = Utils::readString(argv[1]);
      args.metaKey = Utils::readString(argv[2]);
      args.filter = Utils::readString(argv[3]);

      return args;
    };
  };

} // namespace ms

#endif