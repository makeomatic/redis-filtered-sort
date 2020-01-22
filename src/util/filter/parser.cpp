#include "parser.hpp"

#include <iostream>
#include <boost/format.hpp>

#include "filter.hpp"

#include "multi_filter.hpp"
#include "scalar_filter.hpp"
#include "some_any_filter.hpp"

using namespace ms;

void parseJson(FilterInterface *filter, JsonNode node, string topLevelField);

FilterParser::FilterParser() {
}

bool isFilterFunction(string fn) {
  return ScalarFilter::hasFn(fn) || SomeAnyFilter::hasFn(fn);
}

vector<string> getFields(JsonNode node) {
  vector<string> fields;
  for (JsonNode::value_type &field : node) {
    string fieldName = field.second.data();
    fields.push_back(fieldName);
  }
  return fields;
}

SomeAnyFilter *parseSomeNode(string field, JsonNode node) {
  SomeAnyFilter *filter = new SomeAnyFilter(false);
  for (JsonNode::value_type &anySub : node) {
    auto subFilter = new ScalarFilter("match", field, anySub.second.data());
    filter->addSubFilter(subFilter);
  }
  return filter;
}

SomeAnyFilter *parseAnyNode(string field, JsonNode node) {
  SomeAnyFilter *filter = new SomeAnyFilter(true);
  for (JsonNode::value_type &subNode : node) {
    GenericFilter *tmpFilter = new GenericFilter();
    parseJson(tmpFilter, subNode.second, field);
    filter->addSubFilter(tmpFilter);
  }
  return filter;
}

ScalarFilter *parseScalarNode(string field, JsonNode::value_type node) {
  return new ScalarFilter(node.first.data(), field, node.second.data());
}

FilterInterface *parseObjectNode(JsonNode node, string key, string topLevelField) {
  if (key.compare("any") == 0) {
    return parseAnyNode(topLevelField, node);
  }
  if (key.compare("some") == 0) {
    return parseSomeNode(topLevelField, node);
  }

  GenericFilter *filter = new GenericFilter();
  parseJson(filter, node, key);
  return filter;
}

void parseJson(FilterInterface *filter, JsonNode node, string topLevelField = "") {
  for (JsonNode::value_type &child : node) {
    FilterInterface *filterToAdd;
    string jsonKey = child.first.data();
    JsonNode jsonSub = child.second;

    if (jsonKey.compare("#multi") == 0) {
      string matchValue = jsonSub.get<string>("match");
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

FilterInterface *FilterParser::ParseJsonTree(JsonNode node) {
  GenericFilter *filter = new GenericFilter();
  parseJson(filter, node);
  return filter;
}