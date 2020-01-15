#include "bust-command.hpp"

#include <string>
#include <map>
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

int BustCommand::execute(RedisCommander redis)
{
  return redis.cleanCaches(this->tempKeysSet, args.curTime, args.expire);
}