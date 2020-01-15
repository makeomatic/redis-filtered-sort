#ifndef __ARG_PARSER_H
#define __ARG_PARSER_H

#include "../command/cmd-args.hpp"
#include "redismodule.h"

namespace ms {
    using namespace std;

    class arg_parser {
    private:
        long long readLong(RedisModuleString*, long long);
        string readString(RedisModuleString*);
    public:
        arg_parser();
        ~arg_parser();
        SortArgs parseSortCmdArgs(RedisModuleString**, int);
        BustArgs parseBustCmdArgs(RedisModuleString**, int);
        AggregateArgs parseAggregateCmdArgs(RedisModuleString**, int);
    };
    
    arg_parser::arg_parser() {
    }

    arg_parser::~arg_parser() {
    }    
}

#endif