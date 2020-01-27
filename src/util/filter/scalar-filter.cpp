#ifndef FILTER_SCALAR_FILTER
#define FILTER_SCALAR_FILTER

#include <utility>
#include <vector>
#include <string>
#include <map>
#include <functional>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "util/string_match.h"
#include "filter.cpp"

namespace ms {
  using namespace std;
  typedef function<bool(string, string)> MatchFunction;
  typedef map<string, MatchFunction> MatchFunctionMap;

  class ScalarFilter : public GenericFilter {
  public:
    ScalarFilter(string fn, const string& field, string pattern) {
      if (ScalarFilter::matchFns.find(fn) == ScalarFilter::matchFns.end()) {
        auto error = boost::format("Unknown function %s") % fn;
        throw invalid_argument(boost::str(error));
      }

      this->fn = fn;
      this->field = field;
      vector<string> fields = {field};
      this->addUsedField(fields);
      this->matchPattern = std::move(pattern);
    };

    bool match(string id, map<string, string> record) override {
      if (field == ID_FIELD_NAME) {
        auto filterFn = matchFns.at("match");
        return filterFn(id, matchPattern);
      } else {
        const auto& filterFn = matchFns.at(fn);
        auto fieldValue = record.at(field);
        return filterFn(fieldValue, matchPattern);
      }
    };

    bool match(const string& id) {
      if (field == ID_FIELD_NAME) {
        auto filterFn = matchFns.at("match");
        return filterFn(id, matchPattern);
      } else {
        throw invalid_argument("Matching on non id field!");
      }
    };

    static bool hasFn(const string& fn) {
      return ScalarFilter::matchFns.find(fn) == ScalarFilter::matchFns.end();
    };

    static inline MatchFunctionMap matchFns = {
      {"match", [](const string& value, const string& pattern) {
        const string lower_value = boost::algorithm::to_lower_copy(value);
        const string lower_pattern = boost::algorithm::to_lower_copy(pattern);

        MatchResult_t *res = str_match(lower_value.c_str(), lower_pattern.c_str());
        if (res->size > 0 && res->error_count == 0) {
          free_match_result(res);
          return true;
        }
        free_match_result(res);
        return false;
      }},

      {"eq",[](const string& value, const string& pattern) {
        const string lower_value = boost::algorithm::to_lower_copy(value);
        const string lower_pattern = boost::algorithm::to_lower_copy(pattern);
        return lower_value == lower_pattern;
      }},

      {"ne", [](const string& value, const string& pattern) {
        return value != pattern;
      }},
      {"lte",[](const string& value, const string& pattern) {
        long double a = 0, b = 0;
        a = stol(value.empty() ? "0" : value);
        b = stol(pattern.empty() ? "0" : pattern);
        return a <= b;
      }},
      {"gte", [](const string& value, const string& pattern) {
        long double a = 0, b = 0;
        a = stol(value.empty() ? "0" : value);
        b = stol(pattern.empty() ? "0" : pattern);
        return a >= b;
      }},
      {"exists", [](const string& value, const string& pattern) {
        return !value.empty();
      }},
      {"isempty", [](const string& value, const string& pattern) {
        return value.empty();
      }}
    };

  private:
    string fn;
    string field;
    string matchPattern;
  };

}

#endif