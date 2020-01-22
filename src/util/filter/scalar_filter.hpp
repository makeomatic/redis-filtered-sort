#ifndef FILTER_SCALAR_FILTER_H
#define FILTER_SCALAR_FILTER_H

#include <vector>
#include <string>
#include <map>
#include <functional>

#include "util/string_match.h"
#include "filter.hpp"

namespace ms {
  using namespace std;
  typedef function<bool(string, string)> MatchFunction;
  typedef map<string, MatchFunction> MatchFunctionMap;

  class ScalarFilter : public GenericFilter {
  public:
    ScalarFilter(string fn, string field, string pattern);

    bool match(string id, map<string, string> value);

    bool match(string id);

    static MatchFunctionMap matchFns;

    static bool hasFn(string fn) {
      return ScalarFilter::matchFns.find(fn) == ScalarFilter::matchFns.end();
    };

  private:
    string fn;
    string field;
    string pattern;
  };

} // namespace ms

#endif