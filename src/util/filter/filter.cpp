#include "filter.hpp"
#include <string>
#include <vector>
#include <iostream>

using namespace ms;

GenericFilter::GenericFilter() {};

GenericFilter::~GenericFilter() {
  std::cerr << "Destructing filter\n";
  for (auto &filter: filters) {
    delete(filter);
  }
};

vector<FilterInterface *> *GenericFilter::getSubFilters() {
  return &filters;
};

vector<string> GenericFilter::getUsedFields() {
  return fieldsUsed;
}

bool GenericFilter::match(string id, map<string, string> record) {
  auto subFilters = this->getSubFilters();

  for (size_t j = 0; j < subFilters->size(); j++) {
    auto subFilter = subFilters->at(j);
    if (!subFilter->match(id, record)) {
      return false;
    }
  }

  return true;
}

void GenericFilter::addUsedField(vector<string> &fields) {
  fieldsUsed.insert(fieldsUsed.end(), fields.cbegin(), fields.cend());
}

void GenericFilter::addSubFilter(FilterInterface *filter) {
  filters.push_back(filter);
  auto usedFields = filter->getUsedFields();
  this->addUsedField(usedFields);
};

void GenericFilter::copyFilters(FilterInterface *filter) {
  auto subFilters = filter->getSubFilters();
  auto usedFields = filter->getUsedFields();
  filters.insert(filters.end(), subFilters->cbegin(), subFilters->cend());
  this->addUsedField(usedFields);
};