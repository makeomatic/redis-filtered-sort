#include "arg-parser.hpp"
#include "redis/redismodule.h"
#include "redis/util.hpp"

using namespace ms;
using namespace redis;

arg::arg() {
}

arg::~arg() {
}

SortArgs arg::parseSortCmdArgs(RedisModuleString **argv, int argc) {
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

  if (argc == 11 && argv[10] != NULL) {
    args.keyOnly = 1;
  } else {
    args.keyOnly = 0;
  }
  return args;
}

AggregateArgs arg::parseAggregateCmdArgs(RedisModuleString **argv, int argc) {
  AggregateArgs args;
  args.pssKey = Utils::readString(argv[1]);
  args.metaKey = Utils::readString(argv[2]);
  args.filter = Utils::readString(argv[3]);

  return args;
};

BustArgs arg::parseBustCmdArgs(RedisModuleString **argv, int argc) {
  BustArgs args;
  args.idSet = Utils::readString(argv[1]);
  args.expire = 30000;
  args.curTime = Utils::readLong(argv[2], 0);

  if (argc > 3)
    args.expire = Utils::readLong(argv[3], 30000);
  return args;
}
