#include "multi_filter.hpp"
#include "boost/format.hpp"

#include "scalar_filter.hpp"

using namespace ms;

MultiFilter::MultiFilter(vector<string> fields, string pattern) {
  for (auto &field : fields) {
    auto sub = new ScalarFilter("match", field, pattern);
    this->addSubFilter(sub);
  }
}

bool MultiFilter::match(string id, map<string, string> record) {
  auto subFilters = this->getSubFilters();
  for (size_t j = 0; j < subFilters.size(); j++) {
    auto subFilter = subFilters[j];
    if (subFilter->match(id, record)) {
      return true;
    }
  }
  return false;
}
