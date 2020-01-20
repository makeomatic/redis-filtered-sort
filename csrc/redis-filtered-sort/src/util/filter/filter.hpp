#ifndef FILTER_FILTER_H
#define FILTER_FILTER_H

#include <vector>
#include <string>
#include <map>

namespace ms
{
using namespace std;

#define ID_FIELD_NAME "X-ROOT-FIELD-ID"
#define MULTIFILTER_FIELD_NAME "X-ROOT-MULTI-FILTER"

// class FilterInterface
// {
// public:
//   virtual bool match(string id, map<string, string> value) = 0;
//   virtual void addUsedField(vector<string> fields) = 0;
//   virtual void addSubFilter(FilterInterface &);
//   virtual void copyFilters(FilterInterface &);
// };

class GenericFilter //: public FilterInterface
{
public:
  GenericFilter();
  ~GenericFilter();

  vector<GenericFilter> getSubFilters();
  vector<string> getUsedFields();
  bool match(string id, map<string, string> value);
  void addUsedField(vector<string> fields);
  void addSubFilter(GenericFilter filter);
  void copyFilters(GenericFilter filter);

private:
  vector<string> fieldsUsed;
  vector<GenericFilter> filters;
};

} // namespace ms

#endif