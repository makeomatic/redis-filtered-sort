#ifndef __FILTERS_H
#define __FILTERS_H

#include <vector>
#include <string>
#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>

#include "string_match.h"

namespace ms
{
namespace pt = boost::property_tree;
using namespace std;

typedef enum FilterFunc
{
    FILTER_UNK = 1,
    FILTER_ARR,
    FILTER_MATCH,
    FILTER_GTE,
    FILTER_LTE,
    FILTER_EQ,
    FILTER_NE,
    FILTER_EXISTS,
    FILTER_ISEMPTY,
    FILTER_SOME,
    FILTER_ANY
} FilterFunc_t;

typedef enum FilterType
{
    FILTERTYPE_OBJECT = 0,
    FILTERTYPE_MULTI,
    FILTERTYPE_SCALAR,
    FILTERTYPE_ARR,
} FilterType_t;

class Filter
{
private:
public:
    string field;
    string value;
    vector<string> fieldsUsed;

    vector<Filter> subFilters;
    FilterType_t type;
    FilterFunc_t filterFunc;

    Filter();
    ~Filter();

    static Filter ScalarFilter(string fieldName, string filterFunction, string value);
    static Filter MultiFilter(pt::ptree node);

    void ParseSubfilters(pt::ptree node);

    static inline FilterFunc_t getFunctionType(string fnName)
    {
        if (fnName.compare("match") == 0)
        {
            return FILTER_MATCH;
        }
        else if (fnName.compare("gte") == 0)
        {
            return FILTER_GTE;
        }
        else if (fnName.compare("lte") == 0)
        {
            return FILTER_LTE;
        }
        else if (fnName.compare("eq") == 0)
        {
            return FILTER_EQ;
        }
        else if (fnName.compare("ne") == 0)
        {
            return FILTER_NE;
        }
        else if (fnName.compare("exists") == 0)
        {
            return FILTER_EXISTS;
        }
        else if (fnName.compare("isempty") == 0)
        {
            return FILTER_ISEMPTY;
        }
        else if (fnName.compare("some") == 0)
        {
            return FILTER_SOME;
        }
        else if (fnName.compare("any") == 0)
        {
            return FILTER_ANY;
        }
        return FILTER_UNK;
    };

    bool match(string id, map<string, string> value, int level);
    bool match_object(map<string, string> data);

    void loadFromJson(pt::ptree jsonFilters, int level);
    bool checkRecord(pair<string, map<string, string>> entry);

    void copyFields(Filter src);
    void copySubFilters(Filter src);
    void toString(int level);
};

Filter::Filter()
{
}

Filter::~Filter()
{
}
}; // namespace ms

#endif