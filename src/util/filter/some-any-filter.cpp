#ifndef FILTER_SOME_ANY_FILTER
#define FILTER_SOME_ANY_FILTER

#include <string>
#include <map>

#include "filter.cpp"

namespace ms {

  class SomeAnyFilter : public GenericFilter {
  public:
    explicit SomeAnyFilter() = default;

    bool match(string id, map<string, string> record) override {
      auto subFilters = this->getSubFilters();

      for (auto subFilter : *subFilters) {
        bool matchResult = subFilter->match(id, record);

        if (matchResult) {
          return true;
        }
      }
      return false;
    };

    static bool hasFn(const string& fn) {
      return fn == "some" || fn == "any";
    };
  };
} // namespace ms

#endif