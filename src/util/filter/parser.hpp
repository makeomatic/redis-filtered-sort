#ifndef FILTER_PARSER_H
#define FILTER_PARSER_H

#include <vector>
#include <string>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include "filter.hpp"

namespace ms {
  using namespace std;

  typedef boost::property_tree::ptree JsonNode;

  class FilterParser {
  public:
    FilterParser();

    static FilterInterface *ParseJsonTree(JsonNode node);

  private:
    string fn;
    string field;
    string pattern;
  };

} // namespace ms

#endif