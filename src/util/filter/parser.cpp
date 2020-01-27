#ifndef FILTER_PARSER
#define FILTER_PARSER

#include <vector>
#include <string>
#include <boost/property_tree/ptree.hpp>

#include "filter.cpp"
#include "multi-filter.cpp"
#include "scalar-filter.cpp"
#include "some-any-filter.cpp"

namespace ms {
  using namespace std;

  typedef boost::property_tree::ptree JsonNode;

  class FilterParser {
  private:
    static bool isFilterFunction(const string& fn) {
      return ScalarFilter::hasFn(fn) || SomeAnyFilter::hasFn(fn);
    }

    static vector<string> getFields(JsonNode node) {
      vector<string> fields;
      for (JsonNode::value_type &field : node) {
        string fieldName = field.second.data();
        fields.push_back(fieldName);
      }
      return fields;
    }

    static SomeAnyFilter *parseSomeNode(const string& field, JsonNode node) {
      auto *filter = new SomeAnyFilter();
      for (JsonNode::value_type &anySub : node) {
        auto subFilter = new ScalarFilter("match", field, anySub.second.data());
        filter->addSubFilter(subFilter);
      }
      return filter;
    }

    SomeAnyFilter *parseAnyNode(const string& field, const JsonNode& node) {
      auto *filter = new SomeAnyFilter();
      for (auto &subNode : node) {
        auto *tmpFilter = new GenericFilter();
        parseJson(tmpFilter, subNode.second, field);
        filter->addSubFilter(tmpFilter);
      }
      return filter;
    }

    FilterInterface *parseObjectNode(const JsonNode& node, const string& key, const string& topLevelField) {
      if (key == "any") {
        return parseAnyNode(topLevelField, node);
      }
      if (key == "some") {
        return parseSomeNode(topLevelField, node);
      }

      auto *filter = new GenericFilter();
      parseJson(filter, node, key);
      return filter;
    }

    void parseJson(FilterInterface *filter, JsonNode node, const string& topLevelField = "") {
      for (JsonNode::value_type &child : node) {
        FilterInterface *filterToAdd;
        string jsonKey = child.first;
        JsonNode jsonSub = child.second;

        if (jsonKey == "#multi") {
          auto matchValue = jsonSub.get<string>("match");
          auto jsonFields = jsonSub.get_child("fields");
          auto fields = getFields(jsonFields);
          filterToAdd = new MultiFilter(fields, matchValue);
        } else {
          // it's an object
          if (jsonSub.data().empty()) {
            filterToAdd = parseObjectNode(jsonSub, jsonKey, topLevelField);
          } else {
            if (isFilterFunction(jsonKey)) {
              filterToAdd = new ScalarFilter("match", jsonKey, jsonSub.data());
            } else {
              filterToAdd = new ScalarFilter(jsonKey, topLevelField, child.second.data());
            }
          }
        }
        filter->addSubFilter(filterToAdd);
      }

    }

  public:
    FilterParser() = default;;

    FilterInterface *ParseJsonTree(const JsonNode& node) {
      auto *filter = new GenericFilter();
      parseJson(filter, node);
      return filter;
    };
  };

} // namespace ms

#endif
