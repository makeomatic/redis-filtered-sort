#ifndef __BUST_COMMAND_H
#define __BUST_COMMAND_H

#include "cmd-args.hpp"
#include "../util/redis_commands.hpp"

namespace ms {
    using namespace std;

    class BustCommand {
    private:
        BustArgs args;
        string tempKeysSet;
    
    public:
        BustCommand(BustArgs);
        ~BustCommand();
        void init();
        int execute(RedisCommander);
    };
}

#endif