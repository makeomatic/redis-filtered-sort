#ifndef __SORT_COMMAND_H
#define __SORT_COMMAND_H

#include "cmd-args.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "../util/redis_commands.hpp"
#include "../util/filters.hpp"

namespace ms {
    using namespace std;
    using namespace boost::algorithm;

    class SortCommand {
    private:
        SortArgs args;
        string tempKeysSet;
        string fflKey;
        string pssKey;
        boost::property_tree::ptree jsonFilters;
        Filter filters;
        string idFilter;
    public:
        SortCommand(SortArgs);
        ~SortCommand();
        void init();
        int execute(RedisCommander);
    };
}

#endif