#include "arg_parser.hpp"

using namespace ms;

long long arg_parser::readLong(RedisModuleString *redisString, long long defaultValue) {
    long long num = 0;
    if (redisString != NULL){
        RedisModule_StringToLongLong(redisString, &num);
    }
    return num != 0 ? num : defaultValue;
}

string arg_parser::readString(RedisModuleString *redisString) {
    size_t stringSize;
    const char * cString = RedisModule_StringPtrLen(redisString, &stringSize);
    if (stringSize > 0) {
        return string(cString, stringSize);
    }
    return string();
}

SortArgs arg_parser::parseSortCmdArgs(RedisModuleString **argv, int argc) {
    SortArgs args;
    args.offset = 0;
    args.limit = 0;
    args.expire = 30000;
    
    args.idSet = this->readString(argv[1]);
    args.metaKey = this->readString(argv[2]);
    args.hashKey = this->readString(argv[3]);
    args.filter = this->readString(argv[5]);
    args.curTime = this->readLong(argv[6], 0);
    args.order = this->readString(argv[4]);

    if (argc > 7 ) args.offset = this->readLong(argv[7], 0);
    if (argc > 8 ) args.limit = this->readLong(argv[8], 10);
    if (argc > 9 ) args.expire = this->readLong(argv[9], 30000);

    if (argc == 11 && argv[10] != NULL) {
        args.keyOnly = 1;
    } else {
        args.keyOnly = 0;
    }
    return args;
}

AggregateArgs arg_parser::parseAggregateCmdArgs(RedisModuleString **argv, int argc) {
    AggregateArgs args;
    args.pssKey = this->readString(argv[1]);
    args.metaKey = this->readString(argv[2]);
    args.filter = this->readString(argv[3]);

    return args;
};

BustArgs arg_parser::parseBustCmdArgs(RedisModuleString **argv, int argc) {
    BustArgs args;
    args.idSet = this->readString(argv[1]);
    args.expire = 30000;
    args.curTime = this->readLong(argv[2],0);

    if (argc > 3) args.expire = this->readLong(argv[3], 30000);
    return args;
}
