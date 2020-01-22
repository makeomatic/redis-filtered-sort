#ifndef FILTER_SOME_ANY_FILTER_H
#define FILTER_SOME_ANY_FILTER_H

#include <vector>
#include <string>
#include <map>

#include "filter.hpp"

namespace ms {
  using namespace std;

  class SomeAnyFilter : public GenericFilter {
  public:
    SomeAnyFilter(bool isSome = false);

    bool match(string id, map<string, string> value);

    static bool hasFn(string fn) {
      return fn.compare("some") == 0 || fn.compare("any") == 0;
    };

  private:
    bool isSome;
  };
} // namespace ms

#endif