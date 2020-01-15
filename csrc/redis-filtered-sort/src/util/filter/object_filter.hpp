#ifndef FILTER_OBJECT_FILTER_H
#define FILTER_OBJECT_FILTER_H

#include <vector>
#include <string>
#include <map>

#include "filter.hpp"

namespace ms
{
using namespace std;
class ObjectFilter : public GenericFilter
{
public:
    ObjectFilter();
    bool match(string id, map<string, string> value);

private:
    string fn;
    string field;
    string pattern;
};
} // namespace ms

#endif