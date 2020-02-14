#ifndef FILTER_MULTI_FILTER
#define FILTER_MULTI_FILTER

#include <vector>
#include <string>
#include <map>

#include "filter.cpp"
#include "scalar-filter.cpp"

namespace ms {
  using namespace std;

  class MultiFilter : public GenericFilter {
  public:
    MultiFilter(const vector<string>& fields, const string& pattern) {
      for (auto &field : fields) {
        auto sub = new ScalarFilter("match", field, pattern);
        this->addSubFilter(sub);
      }
    };

    bool match(string id, map<string, string> record) override {
      auto subFilters = this->getSubFilters();
      for (size_t j = 0; j < subFilters->size(); j++) {
        auto subFilter = subFilters->at(j);
        if (subFilter->match(id, record)) {
          return true;
        }
      }
      return false;
    }
    ;

  private:
    string fields;
  };
} // namespace ms

#endif
