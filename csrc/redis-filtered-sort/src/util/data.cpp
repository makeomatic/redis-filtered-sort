#include "data.hpp"
#include <algorithm>
#include <map>
#include "boost/algorithm/string/replace.hpp"
#include <iostream>

#include "filter/filter.hpp"
#include "filter/scalar_filter.hpp"

#include <boost/log/trivial.hpp>

using namespace ms;

Data::Data(redis::Context &redis, string metaTemplate) : redis(redis)
{
  this->dataKeyTemplate = metaTemplate;
}

Data::~Data()
{
}

void Data::loadSet(string set)
{
  auto cmd = this->redis.getCommand();
  this->data = cmd.smembers(set);
}

void Data::loadList(string list)
{
  auto cmd = this->redis.getCommand();
  this->data = cmd.lrange(list, 0, -1);
}

void Data::use(vector<string> data)
{
  this->data = data;
}

void Data::save(string key)
{
  std::cerr << "Save list " << key << "\n";
  auto data = this->redis.getData();
  data.saveList(key, this->data);
}

void Data::save(string key, vector<string> list)
{
  std::cerr << "Save custom list " << key << "\n";
  auto data = this->redis.getData();
  data.saveList(key, list);
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

  for( auto &field: fields) {
    BOOST_LOG_TRIVIAL(debug) << "Field to get: " << field;
  }

  for (size_t i = 0; i < this->data.size(); i++)
  {

    string dataValue = this->data[i];
    string metaKey = boost::algorithm::replace_all_copy(this->dataKeyTemplate, "*", dataValue);

    map<string, string> fieldValues;
    auto command = this->redis.getCommand();
    for_each(fields.cbegin(), fields.cend(),
             [&command, &metaKey, &fieldValues](string field) {
               string fieldValue = command.hget(metaKey, field);
               fieldValues.insert({field, fieldValue});
             });

    if (!fieldValues.empty())
    {
      metaData.push_back({dataValue, fieldValues});
    }
    else
    {
      // BOOST_LOG_TRIVIAL(error) << "Missing value for key in "<< metaKey;
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

vector<string> Data::filterMeta(FilterInterface *filter)
{
  vector<string> result;
  auto metaData = this->loadMetadata(filter->getUsedFields());
  std::for_each(metaData.begin(), metaData.end(),
                [&filter, &result](const pair<string, map<string, string>> &entry) {
                  if (filter->match(entry.first, entry.second))
                  {
                    result.push_back(entry.first);
                  }
                });

  return result;
}

vector<string> Data::filter(string pattern)
{
  vector<string> result;
  ScalarFilter filter = ScalarFilter("match", ID_FIELD_NAME, pattern);
  std::for_each(this->data.begin(), this->data.end(),
                [&filter, &result](const string &entry) {
                  if (filter.match(entry))
                  {
                    result.push_back(entry);
                  }
                });

  return result;
}
