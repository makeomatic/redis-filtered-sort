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

    virtual void addUsedField(vector<string> &fields) = 0;

    virtual void addSubFilter(FilterInterface *) = 0;

    virtual void copyFilters(FilterInterface *) = 0;

    virtual vector<FilterInterface *>* getSubFilters() = 0;

    virtual vector<string> getUsedFields() = 0;

    virtual ~FilterInterface() {};
  };

  class GenericFilter : public FilterInterface {
  public:
    GenericFilter();

    ~GenericFilter() override;

    vector<FilterInterface *>* getSubFilters() override;

    vector<string> getUsedFields() override;

    bool match(string id, map<string, string> value) override;

    void addUsedField(vector<string> &fields) override;

    void addSubFilter(FilterInterface *filter) override;

    void copyFilters(FilterInterface *filter) override;

  private:
    vector<string> fieldsUsed;
    vector<FilterInterface *> filters;
  };

} // namespace ms

#endif