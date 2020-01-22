#include "some_any_filter.hpp"

using namespace ms;

SomeAnyFilter::SomeAnyFilter(bool isSome) {
  this->isSome = isSome;
}

bool SomeAnyFilter::match(string id, map<string, string> record) {
  auto subFilters = this->getSubFilters();

  for (size_t i = 0; i < subFilters.size(); i++) {
    auto subFilter = subFilters[i];
    bool matchResult = subFilter->match(id, record);

    if (matchResult) {
      return true;
    }
  }
  return false;
};
