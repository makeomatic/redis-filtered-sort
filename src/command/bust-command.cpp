#include "bust-command.hpp"

#include <string>
#include <iostream>
#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace ms;
using namespace std;
using namespace boost::algorithm;

BustCommand::BustCommand(BustArgs args) {
  this->args = args;
  this->init();
}

BustCommand::~BustCommand() {
}

void BustCommand::init() {
  this->tempKeysSet = boost::str(boost::format("%s::fsort_temp_keys") % this->args.idSet);
}

int BustCommand::execute(redis::Context redis) {
  long long boundary = args.curTime - args.expire;
  auto cmd = redis.getCommand();

  auto expiredKeys = cmd.zrangebyscore(tempKeysSet, to_string(boundary), "+inf");

  for (auto &key : expiredKeys) {
    cmd.del(key);
    cmd.zrem(tempKeysSet, key);
  }

  redis.respondLong(expiredKeys.size());
  return 1;
}