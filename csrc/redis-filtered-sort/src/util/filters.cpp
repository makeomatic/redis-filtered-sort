#include "filters.hpp"
#include "boost/format.hpp"
#include <boost/algorithm/string.hpp>

#include "util/filter_fn.hpp"
#include <iostream>

using namespace ms;
#define ID_FIELD_NAME "X-ROOT-FIELD-ID"
#define MULTIFILTER_FIELD_NAME "X-ROOT-MULTI-FILTER"

string repeat(string s, int n)
{
  string s1 = s;

  for (int i = 1; i < n; i++)
    s += s1;

  return s;
}

bool CheckGenericFn(Filter filter, string id, map<string, string> record, int level)
{
  string tab = repeat("-", level);

  bool match = false;
  string value = filter.value;

  if (filter.field.compare(ID_FIELD_NAME) == 0)
  {
    return FilterFn::match_string(id, value);
  }
  // std::cout << tab << "Field: " << filter.field << " Type:" << filter.type << " Fn:" << filter.filterFunc << ", value:" << filter.value << " data " << record[filter.field] << "\n";

  if (record.find(filter.field) == record.end())
  {
    return false;
  }

  string valueStr = record[filter.field];

  switch (filter.filterFunc)
  {
  case FILTER_UNK:
  {
    match = false;
  };
  break;
  case FILTER_MATCH:
  {
    match = FilterFn::match_string(valueStr, value);
  }
  break;
  case FILTER_EQ:
  {
    match = FilterFn::eq(valueStr, value);
  };
  break;
  case FILTER_NE:
  {
    match = FilterFn::ne(valueStr, value);
  }
  break;
  case FILTER_GTE:
  {
    match = FilterFn::gte(valueStr, value);
  }
  break;
  case FILTER_LTE:
  {
    match = FilterFn::lte(valueStr, value);
  }
  break;
  case FILTER_EXISTS:
  {

    if (valueStr.empty())
    {
      match = false;
    }
    else
    {
      match = true;
    }
  }
  break;
  case FILTER_ISEMPTY:
  {
    match = valueStr.empty();
  }
  break;
  case FILTER_SOME:
  {
    if (valueStr.empty())
    {
      match = false;
    }
    match = FilterFn::eq(valueStr, value);
  }
  break;
  case FILTER_ANY:
    break;
  case FILTER_ARR:
    break;
  };
  return match;
}

bool Filter::match(string id, map<string, string> record, int level)
{
  string tab = repeat("-", level);
  level++;
  if (this->type == FILTERTYPE_SCALAR)
  {
    return CheckGenericFn(*this, id, record, ++level);
  }

  if (this->type == FILTERTYPE_MULTI)
  {
    auto subFilters = this->subFilters;
    for (size_t j = 0; j < subFilters.size(); j++)
    {
      Filter subFilter = subFilters[j];
      if (subFilter.match(id, record, level))
        return true;
    }
    return false;
  }

  if (this->type == FILTERTYPE_ARR)
  {
    auto subFilters = this->subFilters;
    // std::cerr << tab << "Checking array filter \n";
    bool isSomeOrAny = (this->filterFunc == FILTER_ANY || this->filterFunc == FILTER_SOME);

    level++;

    for (size_t i = 0; i < subFilters.size(); i++)
    {
      Filter subFilter = subFilters[i];
      bool matchResult = subFilter.match(id, record, level);
      // std::cerr << tab << "FF " << this->filterFunc << " Pre " << matchResult << "\n";
      if (isSomeOrAny)
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
    return isSomeOrAny ? false : true;
  }

  if (this->type == FILTERTYPE_OBJECT)
  {
    auto subFilters = this->subFilters;
    for (size_t j = 0; j < subFilters.size(); j++)
    {
      Filter subFilter = subFilters[j];
      if (!subFilter.match(id, record, level))
        return false;
    }
    return true;
  }

  return false;
}

bool Filter::checkRecord(pair<string, map<string, string>> value)
{
  // std::cerr << "CHECK ======= \n";
  auto matched = this->match(value.first.data(), value.second, 1);
  // std::cerr << "CHECK " << matched << "\n";
  return matched;
};

void Filter::loadFromJson(pt::ptree node, int level)
{
  string tab = repeat("  ", level);

  for (pt::ptree::value_type &child : node)
  {
    Filter filter;
    std::cout << tab << " working with: " << child.first.data() << "\n";
    string key = child.first.data();

    if (key.compare("#multi") == 0)
    {
      filter = MultiFilter(child.second);
    }
    else
    {
      filter.field = key;
      auto functionType = getFunctionType(key);
      // it's an object
      string value = child.second.data();
      if (value.empty())
      {
        filter.type = FILTERTYPE_OBJECT;

        filter.filterFunc = functionType;
        if (functionType != FILTER_UNK)
        {
          std::cout << tab << " and is function " << key << "\n";
          if (functionType == FILTER_ANY)
          {
            filter.field = this->field;
            filter.type = FILTERTYPE_ARR;
            for (pt::ptree::value_type &anySub : child.second)
            {
              std::cout << tab << tab << "       parse child " << child.first << "\n";
              Filter tmpFilter;
              tmpFilter.type = FILTERTYPE_ARR;
              tmpFilter.field = filter.field;
              tmpFilter.filterFunc = FILTER_ARR;
              tmpFilter.loadFromJson(anySub.second, ++level);

              filter.subFilters.push_back(tmpFilter);
              filter.copyFields(tmpFilter);
            }
          }
          else if (functionType == FILTER_SOME)
          {
            filter.field = this->field;
            filter.type = FILTERTYPE_ARR;
            for (pt::ptree::value_type &anySub : child.second)
            {
              auto tmpFilter = ScalarFilter(field, "eq", anySub.second.data());
              filter.subFilters.push_back(tmpFilter);
              filter.copyFields(tmpFilter);
            }
          }
          else
          {
            filter.loadFromJson(child.second, 1 + level);
          }
        }
        else
        {
          std::cout << tab << "    and is field " << key << "\n";
          filter.loadFromJson(child.second, ++level);
        }
      }
      else
      {
        std::cout << tab << " child is string:" << child.second.data() << ":" << this->type << "\n";
        if (functionType == FILTER_UNK)
        {
          filter = ScalarFilter(key, "match", child.second.data());
        }
        else
        {
          filter = ScalarFilter(this->field, key, child.second.data());
        }
      }
    }

    this->subFilters.push_back(filter);
    this->copyFields(filter);
  }
}

void Filter::toString(int level)
{
  string s = repeat(".", level + 1);

  std::cout << s << boost::format("< field: %s, func: %d, type: %d, value '%s' >") % this->field % this->filterFunc % this->type % this->value;
  std::cout << "\n";
  for (size_t i = 0; i < this->subFilters.size(); i++)
  {
    this->subFilters[i].toString(1 + level);
  }
}

void Filter::copyFields(Filter src)
{
  this->fieldsUsed.insert(this->fieldsUsed.end(), src.fieldsUsed.begin(), src.fieldsUsed.end());
}

void Filter::copySubFilters(Filter src)
{
  this->subFilters.insert(this->subFilters.end(), src.subFilters.begin(), src.subFilters.end());
}

Filter Filter::ScalarFilter(string fieldName, string filterFunction, string value)
{
  Filter filter = Filter();
  filter.type = FILTERTYPE_SCALAR;
  filter.field = fieldName;
  filter.value = value;
  if (fieldName.compare("#") == 0)
  {
    filter.field = ID_FIELD_NAME;
    filter.filterFunc = FILTER_MATCH;
  }
  else
  {
    filter.fieldsUsed = {fieldName};
    filter.filterFunc = Filter::getFunctionType(filterFunction);
  }

  return filter;
}

Filter Filter::MultiFilter(pt::ptree node)
{
  string matchValue = node.get<string>("match");
  auto fields = node.get_child("fields");

  Filter filter = Filter();

  filter.type = FILTERTYPE_MULTI;
  filter.filterFunc = FILTER_ARR;
  filter.value = matchValue;
  filter.field = MULTIFILTER_FIELD_NAME;

  for (pt::ptree::value_type &field : fields)
  {
    string fieldName = field.second.data();
    filter.fieldsUsed.push_back(fieldName);
    auto fieldFilter = Filter::ScalarFilter(fieldName, "match", matchValue);
    filter.subFilters.push_back(fieldFilter);
  }

  return filter;
}