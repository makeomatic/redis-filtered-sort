#include "bust-command.hpp"

#include <string>
#include <iostream>
#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include <cmath>

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
  std::cerr << "Cleaning caches " << tempKeysSet << " Curtime" << args.curTime << " Expire " << args.expire << "\n";

  long long boundary = args.curTime - args.expire;
  auto cmd = redis.getCommand();

  auto expiredKeys = cmd.zrangebyscore(tempKeysSet, boundary, INFINITY);

  for (auto &key : expiredKeys) {
    cmd.del(key);
    cmd.zrem(tempKeysSet, key);
  }
  // ????

  redis.respondLong(expiredKeys.size());
  return 1;
}