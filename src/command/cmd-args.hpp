#ifndef __CMD_ARGS_H
#define __CMD_ARGS_H

#include "string"

namespace ms {
  using namespace std;

  struct SortArgs {
    string tempKeysSet;
    string idSet;
    string metaKey;
    string hashKey;
    string order;
    string filter;
    long long curTime;
    long long offset;
    long long limit;
    long long expire;
    long long keyOnly;
  };

  struct BustArgs {
    string idSet;
    long long expire;
    long long curTime;
  };

  struct AggregateArgs {
    string pssKey;
    string metaKey;
    string filter;
  };
};

#endif