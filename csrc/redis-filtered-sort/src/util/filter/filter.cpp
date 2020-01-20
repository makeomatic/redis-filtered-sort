#include "filter.hpp"

using namespace ms;

GenericFilter::GenericFilter(){};
GenericFilter::~GenericFilter(){};

vector<GenericFilter> GenericFilter::getSubFilters()
{
    return filters;
};

vector<string> GenericFilter::getUsedFields()
{
    return fieldsUsed;
}

bool GenericFilter::match(string id, map<string, string> value)
{
    return false;
}

void GenericFilter::addUsedField(vector<string> fields)
{
    fieldsUsed.insert(fieldsUsed.end(), fields.cbegin(), fields.cend());
}

void GenericFilter::addSubFilter(GenericFilter filter)
{
    filters.push_back(filter);
    this->addUsedField(filter.getUsedFields());
};

void GenericFilter::copyFilters(GenericFilter filter)
{
    auto subFilters = filter.getSubFilters();
    filters.insert(filters.end(), subFilters.cbegin(), subFilters.cbegin());
    this->addUsedField(filter.getUsedFields());
};