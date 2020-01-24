#ifndef __DATA_H
#define __DATA_H

#include <vector>
#include <map>
#include <string>
#include "redis/context.hpp"

#include "./filter/filter.hpp"

namespace ms {
  using namespace std;

  class Data {
  private:
    redis::Context &redis;
    vector<string> data;
    string dataKeyTemplate;

  public:
    string dataTemplate;

    Data(redis::Context &redis, string);

    ~Data();

    void loadList(string);

    void loadSet(string);

    void use(vector<string>);

    void save(string);

    void save(string, vector<string>);

    vector<pair<string, map<string, string>>> loadMetadata(vector<string> fields);

    size_t size();

    void sort(string);

    void sortMeta(string, string);

    vector<string> filter(string);

    vector<string> filterMeta(FilterInterface *);
  };

} // namespace ms

#endif