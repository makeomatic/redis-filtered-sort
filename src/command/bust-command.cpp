#ifndef __BUST_COMMAND_H
#define __BUST_COMMAND_H

#include <string>

#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>

#include "cmd-args.hpp"
#include "redis/context.cpp"

namespace ms {
  class BustCommand {
  private:
    BustArgs args;
    string tempKeysSet;

  public:
    explicit BustCommand(BustArgs &args): args(args) {
      this->tempKeysSet = boost::str(boost::format("%s::fsort_temp_keys") % this->args.idSet);
    };

    ~BustCommand() = default;

    int execute(redis::Context redis) {
      long long boundary = args.curTime - args.expire;
      auto cmd = redis.getCommand();

      auto expiredKeys = cmd.zrangebyscore(tempKeysSet, to_string(boundary), "+inf");

      for (auto &key : expiredKeys) {
        cmd.del(key);
        cmd.zrem(tempKeysSet, key);
      }

      redis.respondLong(expiredKeys.size());
      return 1;
    };
  };
} // namespace ms

#endif