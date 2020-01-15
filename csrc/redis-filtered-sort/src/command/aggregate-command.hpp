#ifndef __AGGREGATE_COMMAND_H
#define __AGGREGATE_COMMAND_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "cmd-args.hpp"
#include "util/redis_commands.hpp"

namespace ms {
    using namespace std;

    class AggregateCommand {
    private:
        AggregateArgs args;
        boost::property_tree::ptree jsonFilters;

    public:
        AggregateCommand(AggregateArgs);
        ~AggregateCommand();
        void init();
        int execute(RedisCommander);
    };
}

#endif