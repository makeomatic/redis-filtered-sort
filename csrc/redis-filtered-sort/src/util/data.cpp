#include "data.hpp"
#include <algorithm>
#include <map>
#include "boost/algorithm/string/replace.hpp"
#include <iostream>
using namespace ms;

void Data::loadSet(string set)
{
  this->data = this->redis.loadSet(set);
}

void Data::loadList(string list)
{
  this->data = this->redis.loadList(list);
}

void Data::use(vector<string> data)
{
  this->data = data;
}

void Data::save(string list)
{
  std::cerr << "Save list " << list << "\n";
  this->redis.saveList(list, this->data);
}

void Data::save(string list, vector<string> data)
{
  std::cerr << "Save custom list " << list << "\n";
  this->redis.saveList(list, data);
}

size_t Data::size()
{
  return this->data.size();
}

typedef map<string, string> MetaDataRecord;
typedef pair<string, MetaDataRecord> DataRecord;

vector<pair<string, map<string, string>>> Data::loadMetadata(vector<string> fields)
{
  vector<DataRecord> metaData;

  this->redis.lockContext();

  for (size_t i = 0; i < this->data.size(); i++)
  {

    string dataValue = this->data[i];
    string metaKey = boost::algorithm::replace_all_copy(this->dataKeyTemplate, "*", dataValue);

    map<string, string> fieldValues;

    for_each(fields.cbegin(), fields.cend(),
             [this, &metaKey, &fieldValues](string field) {
               string fieldValue = this->redis.getHashField(metaKey, field);
               fieldValues.insert({field, fieldValue});
             });

    if (!fieldValues.empty())
    {
      metaData.push_back({dataValue, fieldValues});
    }
    else
    {
      std::cerr << "Missing value" << metaKey << "\n";
    }
  }

  this->redis.unlockContext();

  return metaData;
}

vector<string> Data::sort(string order)
{
  if (order.compare("ASC") == 0)
  {
    std::sort(this->data.begin(), this->data.end(), [](string &a, string &b) { return a.compare(b) <= 0; });
  }
  else
  {
    std::sort(this->data.begin(), this->data.end(), [](string &a, string &b) { return a.compare(b) >= 0; });
  }
  return this->data;
};

vector<string> Data::sortMeta(string field, string order)
{
  auto metaData = this->loadMetadata(vector<string>{field});

  auto sortAscFn = [&field](const DataRecord &a, const DataRecord &b) -> bool {
    string aVal = a.second.at(field);
    string bVal = b.second.at(field);
    return aVal.compare(bVal) < 0;
  };

  auto sortDescFn = [&field](const DataRecord &a, const DataRecord &b) -> bool {
    string aVal = a.second.at(field);
    string bVal = b.second.at(field);
    return aVal.compare(bVal) > 0;
  };

  if (order.compare("ASC") == 0)
  {
    std::sort(metaData.begin(), metaData.end(), sortAscFn);
  }
  else
  {
    std::sort(metaData.begin(), metaData.end(), sortDescFn);
  }

  vector<string> result;

  std::for_each(metaData.begin(), metaData.end(),
                [&field, &result](const pair<string, map<string, string>> &entry) {
                  auto m = entry.second;
                  result.push_back(entry.first);
                });

  this->data = result;
  return result;
};

vector<string> Data::filterMeta(Filter filter)
{
  vector<string> result;
  auto metaData = this->loadMetadata(filter.fieldsUsed);
  std::for_each(metaData.begin(), metaData.end(),
                [&filter, &result](const pair<string, map<string, string>> &entry) {
                  if (filter.checkRecord(entry))
                  {
                    result.push_back(entry.first);
                  }
                });

  return result;
}

vector<string> Data::filter(string pattern)
{
  vector<string> result;
  std::for_each(this->data.begin(), this->data.end(),
                [&pattern, &result](const string &entry) {
                  auto matchResult = FilterFn::match_string(entry, pattern);
                  if (matchResult)
                  {
                    result.push_back(entry);
                  }
                });

  return result;
}
