#include "some_any_filter.hpp"

#include <boost/format.hpp>
#include <boost/log/trivial.hpp>

using namespace ms;

SomeAnyFilter::SomeAnyFilter(bool isSome = false)
{
  this->isSome = isSome;
}

bool SomeAnyFilter::match(string id, map<string, string> record)
{
  auto subFilters = this->getSubFilters();
  // std::cerr << tab << "Checking array filter \n";

  for (size_t i = 0; i < subFilters.size(); i++)
  {
    auto subFilter = subFilters[i];
    bool matchResult = subFilter->match(id, record);
    // std::cerr << tab << "FF " << this->filterFunc << " Pre " << matchResult << "\n";
    if (matchResult)
    {
        // std::cerr << tab << "Filter any did match \n";
      return true;
    }
  }
  // std::cerr << tab << "Done array check \n";
  return false;
};
