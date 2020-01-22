#ifndef __SORT_COMMAND_H
#define __SORT_COMMAND_H

#include "cmd-args.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "redis/context.hpp"
#include "util/filter/filter.hpp"

namespace ms
{
using namespace std;

class SortCommand
{
private:
  SortArgs args;
  string tempKeysSet;
  string fflKey;
  string pssKey;
  boost::property_tree::ptree jsonFilters;
  FilterInterface *filters;
  string idFilter;

public:
  SortCommand(SortArgs);
  ~SortCommand();
  void init();
  int execute(redis::Context &);
};
} // namespace ms

#endif