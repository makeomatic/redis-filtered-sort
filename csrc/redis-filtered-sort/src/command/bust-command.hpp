#ifndef __BUST_COMMAND_H
#define __BUST_COMMAND_H

#include "cmd-args.hpp"
#include "util/redis-wrapper.hpp"

namespace ms
{
using namespace std;

class BustCommand
{
private:
  BustArgs args;
  string tempKeysSet;

public:
  BustCommand(BustArgs);
  ~BustCommand();
  void init();
  int execute(redis::Context);
};
} // namespace ms

#endif