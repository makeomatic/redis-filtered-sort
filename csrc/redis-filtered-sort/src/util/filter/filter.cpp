#include "filter.hpp"
#include <string>
#include <vector>
#include <boost/log/trivial.hpp>

using namespace ms;

GenericFilter::GenericFilter(){};
GenericFilter::~GenericFilter(){};

vector<FilterInterface*> GenericFilter::getSubFilters()
{
  return filters;
};

vector<string> GenericFilter::getUsedFields()
{
  return fieldsUsed;
}

bool GenericFilter::match(string id, map<string, string> record)
{
  auto subFilters = this->getSubFilters();
  BOOST_LOG_TRIVIAL(debug) << "Generic filter start =====================";
  for (size_t j = 0; j < subFilters.size(); j++)
  {
    auto subFilter = subFilters[j];
    if (!subFilter->match(id, record)) {
      BOOST_LOG_TRIVIAL(debug) << "Generic filter mismatch =====================";
      return false;
    } 
  }

  BOOST_LOG_TRIVIAL(debug) << "Generic filter match =====================" ;
  return true;
}

void GenericFilter::addUsedField(vector<string> fields)
{
  fieldsUsed.insert(fieldsUsed.end(), fields.cbegin(), fields.cend());
}

void GenericFilter::addSubFilter(FilterInterface *filter)
{
  filters.push_back(filter);
  this->addUsedField(filter->getUsedFields());
};

void GenericFilter::copyFilters(FilterInterface *filter)
{
  auto subFilters = filter->getSubFilters();
  filters.insert(filters.end(), subFilters.cbegin(), subFilters.cend());
  this->addUsedField(filter->getUsedFields());
};

// string GenericFilter::to_string() {
//   string strVal = "\n" + string(typeid(this).name()) + " with fields [";
//   for (auto &field : fieldsUsed)
//   {
//     strVal += field + ", ";
//   }
//   strVal += "]\n\tSubFilters:\n";
//   for (auto &filter: filters) {
//     strVal += "\t\t" + filter.to_string();
//   }
//   strVal += "\n";
//   return strVal;
// }