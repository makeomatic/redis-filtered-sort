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

class GenericFilter
{
public:
  GenericFilter();
  ~GenericFilter();
  virtual bool match(string id, map<string, string> value) const;

  vector<GenericFilter> getSubFilters();
  vector<string> getUsedFields();

  void addUsedField(vector<string> fields);
  void addSubFilter(GenericFilter filter);
  void copyFilters(GenericFilter filter);

private:
  vector<string> fieldsUsed;
  vector<GenericFilter> filters;
};

} // namespace ms

#endif