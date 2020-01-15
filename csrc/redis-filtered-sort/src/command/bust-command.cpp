#include "bust-command.hpp"

#include <string>
#include <map>
#include <iostream>
#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace ms;
using namespace std;
using namespace boost::algorithm;

BustCommand::BustCommand(BustArgs args)
{
  this->args = args;
  this->init();
}

BustCommand::~BustCommand()
{
}

void BustCommand::init()
{
  this->tempKeysSet = boost::str(boost::format("%s::fsort_temp_keys") % this->args.idSet);
}

int BustCommand::execute(redis::Context redis)
{
  // return redis.cleanCaches(this->tempKeysSet, args.curTime, args.expire);
  std::cerr << "Cleaning caches " << tempKeysSet << " Curtime" << args.curTime << " Expire " << args.expire;

  long long boundary = args.curTime - args.expire;
  auto cmd = redis.getCommand();

  auto expiredKeys = cmd.zrangebyscore(tempKeysSet, boundary, -1);

  for (auto &key : expiredKeys)
  {
    cmd.del(key);
  }
  // ????
  // cmd.del(tempKeysSet);
  redis.respondLong(expiredKeys.size());
  return 1;
}