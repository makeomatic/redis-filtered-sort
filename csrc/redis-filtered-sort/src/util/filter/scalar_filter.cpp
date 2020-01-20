#include "scalar_filter.hpp"
#include "boost/format.hpp"

using namespace ms;

ScalarFilter::ScalarFilter(string fn, string field, string pattern)
{
  if (this->matchFns.find(fn) == this->matchFns.end())
  {
    auto error = boost::format("Unknown function %s") % fn;
    throw invalid_argument(boost::str(error));
  }

  this->fn = fn;
  this->field = field;
  this->pattern = pattern;
}

bool ScalarFilter::match(string id, map<string, string> record)
{
  if (field.compare(ID_FIELD_NAME) == 0)
  {
    auto filterFn = matchFns.at("match");
    return filterFn(id, pattern);
  }
  else
  {
    auto filterFn = matchFns.at(fn);
    auto fieldValue = record.at(field);
    return filterFn(fieldValue, pattern);
  }
}

bool ScalarFilter::match(string id)
{
  if (field.compare(ID_FIELD_NAME) == 0)
  {
    auto filterFn = matchFns.at("match");
    return filterFn(id, pattern);
  }
  else
  {
    throw "Matching on non id field!";
  }
}

MatchFunctionMap ScalarFilter::matchFns =
    {{"match", [](string value, string pattern) {
        MatchResult_t *res = str_match(value.c_str(), pattern.c_str());
        if (res->size > 0 && res->error_count == 0)
        {
          free_match_result(res);
          return true;
        }
        free_match_result(res);
        return false;
      }},
     {"eq", [](string value, string pattern) {
        return value.compare(pattern) == 0;
      }},
     {"ne", [](string value, string pattern) {
        return value.compare(pattern) != 0;
      }},
     {"lte", [](string value, string pattern) {
        long double a = 0, b = 0;
        a = stol(value.empty() ? "0" : value);
        b = stol(pattern.empty() ? "0" : pattern);
        return a <= b;
      }},
     {"gte", [](string value, string pattern) {
        long double a = 0, b = 0;
        a = stol(value.empty() ? "0" : value);
        b = stol(pattern.empty() ? "0" : pattern);
        return a >= b;
      }},
     {"exists", [](string value, string pattern) {
        return !value.empty();
      }},
     {"empty", [](string value, string pattern) {
        return value.empty();
      }}};