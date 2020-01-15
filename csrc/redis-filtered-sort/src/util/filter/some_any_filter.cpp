#include "some_any_filter.hpp"
#include "boost/format.hpp"

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
    bool matchResult = subFilter.match(id, record);
    // std::cerr << tab << "FF " << this->filterFunc << " Pre " << matchResult << "\n";
    if (isSome)
    {
      if (matchResult)
      {
        // std::cerr << tab << "Filter any did match \n";
        return true;
      }
    }
    else
    {
      if (!matchResult)
        return false;
    }
  }
  // std::cerr << tab << "Done array check \n";
  return isSome ? false : true;
}