#ifndef __FILTER_FUNC_H
#define __FILTER_FUNC_H

#include <string>
#include "string_match.h"
#include "boost/algorithm/string.hpp"
#include <iostream>

namespace ms
{
using namespace std;

class FilterFn
{
public:
    static bool eq(string, string);
    static bool ne(string, string);
    static bool lte(string, string);
    static bool gte(string, string);
    static bool match_string(string, string);
};

bool FilterFn::match_string(string value, string pattern)
{
    string lowerValue = value;
    string lowerPattern = value;

    MatchResult_t *res = str_match(value.c_str(), pattern.c_str());

    if (res->size > 0 && res->error_count == 0)
    {
        free_match_result(res);
        return true;
    }
    free_match_result(res);
    return false;
}

bool FilterFn::eq(string str, string str2)
{
    string strLower = boost::algorithm::to_lower_copy(str);
    string str2Lower = boost::algorithm::to_lower_copy(str2);
    if (strLower.compare(str2) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

bool FilterFn::ne(string str, string str2)
{
    string strLower = boost::algorithm::to_lower_copy(str);
    string str2Lower = boost::algorithm::to_lower_copy(str2);
    if (strLower.compare(str2) == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

bool FilterFn::lte(string str, string str2)
{
    long double a = 0, b = 0;
    a = stol(str.empty() ? "0" : str);
    b = stol(str2.empty() ? "0" : str2);

    return a <= b;
}

bool FilterFn::gte(string str, string str2)
{
    long double a = 0, b = 0;
    a = stol(str.empty() ? "0" : str);
    b = stol(str2.empty() ? "0" : str2);

    return a >= b;
}

} // namespace ms

#endif