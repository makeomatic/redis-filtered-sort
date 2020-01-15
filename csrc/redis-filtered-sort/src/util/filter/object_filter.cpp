#include "object_filter.hpp"
#include "boost/format.hpp"

using namespace ms;

ObjectFilter::ObjectFilter()
{
}

bool ObjectFilter::match(string id, map<string, string> record)
{
    auto subFilters = this->getSubFilters();
    for (size_t j = 0; j < subFilters.size(); j++)
    {
        auto subFilter = subFilters[j];
        if (!subFilter.match(id, record))
            return false;
    }
    return true;
}