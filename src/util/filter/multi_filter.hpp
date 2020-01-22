#ifndef FILTER_MULTI_FILTER_H
#define FILTER_MULTI_FILTER_H

#include <vector>
#include <string>
#include <map>

#include "filter.hpp"

namespace ms {
  using namespace std;

  class MultiFilter : public GenericFilter {
  public:
    MultiFilter(vector<string> fields, string pattern);

    bool match(string id, map<string, string> value);

  private:
    string fields;
  };
} // namespace ms

#endif