#ifndef FILTER_FILTER
#define FILTER_FILTER

#include <string>
#include <vector>
#include <algorithm>

#include "filter.hpp"


namespace ms {
  class GenericFilter : public FilterInterface {
  public:
    GenericFilter() = default;

    ~GenericFilter() override {
      for (auto &filter: filters) {
        delete (filter);
      }
    };

    vector<FilterInterface *> *getSubFilters() override {
      return &filters;
    };

    vector<string> getUsedFields() override {
      return fieldsUsed;
    };

    bool match(string id, map<string, string> record) override {
      auto subFilters = this->getSubFilters();

      for (auto subFilter : *subFilters) {
        if (!subFilter->match(id, record)) {
          return false;
        }
      }

      return true;
    };

    void addUsedField(vector<string> &fields) override {
      fieldsUsed.insert(fieldsUsed.end(), fields.cbegin(), fields.cend());
      auto uniqueIt = std::unique(fieldsUsed.begin(), fieldsUsed.end());
      fieldsUsed.resize( std::distance(fieldsUsed.begin(), uniqueIt));
    };

    void addSubFilter(FilterInterface *filter) override {
      filters.push_back(filter);
      auto usedFields = filter->getUsedFields();
      this->addUsedField(usedFields);
    };

    void copyFilters(FilterInterface *filter) override {
      auto subFilters = filter->getSubFilters();
      auto usedFields = filter->getUsedFields();
      filters.insert(filters.end(), subFilters->cbegin(), subFilters->cend());
      this->addUsedField(usedFields);
    };

  private:
    vector<string> fieldsUsed;
    vector<FilterInterface *> filters;
  };
}

#endif