#ifndef FILTER_FILTER_H
#define FILTER_FILTER_H

#include <vector>
#include <string>
#include <map>

namespace ms {
  using namespace std;

#define ID_FIELD_NAME "#"

  class FilterInterface {
  public:
    virtual bool match(string id, map<string, string> value) = 0;

    virtual void addUsedField(vector<string> fields) = 0;

    virtual void addSubFilter(FilterInterface *) = 0;

    virtual void copyFilters(FilterInterface *) = 0;

    virtual vector<FilterInterface *> getSubFilters() = 0;

    virtual vector<string> getUsedFields() = 0;
  };

  class GenericFilter : public FilterInterface {
  public:
    GenericFilter();

    ~GenericFilter();

    vector<FilterInterface *> getSubFilters();

    vector<string> getUsedFields();

    bool match(string id, map<string, string> value);

    void addUsedField(vector<string> fields);

    void addSubFilter(FilterInterface *filter);

    void copyFilters(FilterInterface *filter);

    string to_string();

  private:
    vector<string> fieldsUsed;
    vector<FilterInterface *> filters;
  };

} // namespace ms

#endif