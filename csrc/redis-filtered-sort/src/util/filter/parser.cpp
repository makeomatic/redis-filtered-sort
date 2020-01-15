#include "parser.hpp"

#include <iostream>
#include <boost/format.hpp>

#include "object_filter.hpp"
#include "multi_filter.hpp"
#include "scalar_filter.hpp"
#include "some_any_filter.hpp"

using namespace ms;

GenericFilter parseJson(JsonNode node, string topLevelField);

FilterParser::FilterParser()
{
}

bool isFilterFunction(string fn)
{
  return ScalarFilter::hasFn(fn) || SomeAnyFilter::hasFn(fn);
}

vector<string> getFields(JsonNode node)
{
  vector<string> fields;
  for (JsonNode::value_type &field : node)
  {
    string fieldName = field.second.data();
    fields.push_back(fieldName);
  }
  return fields;
}

SomeAnyFilter parseSomeNode(string field, JsonNode node)
{
  SomeAnyFilter filter = SomeAnyFilter(false);
  for (JsonNode::value_type &anySub : node)
  {
    auto subFilter = ScalarFilter(field, "eq", anySub.second.data());
    filter.addSubFilter(subFilter);
  }
  return filter;
}

SomeAnyFilter parseAnyNode(string field, JsonNode node)
{
  SomeAnyFilter filter = SomeAnyFilter(true);
  for (JsonNode::value_type &subNode : node)
  {
    auto tmpFilter = parseJson(subNode.second, field);
    filter.copyFilters(tmpFilter);
  }
  return filter;
}

ScalarFilter parseScalarNode(string field, JsonNode::value_type node)
{
  ScalarFilter filter = ScalarFilter(node.first.data(), field, node.second.data());
  return filter;
}

GenericFilter parseObjectNode(JsonNode node, string key)
{
  if (key.compare("any") == 0)
  {
    return parseAnyNode(key, node);
  }
  if (key.compare("some") == 0)
  {
    return parseSomeNode(key, node);
  }
  return parseJson(node, key);
}

GenericFilter parseJson(JsonNode node, string topLevelField = "")
{
  GenericFilter filter;
  for (JsonNode::value_type &child : node)
  {
    GenericFilter filterToAdd;
    string jsonKey = child.first.data();
    JsonNode jsonSub = child.second;

    if (jsonKey.compare("#multi") == 0)
    {
      string matchValue = node.get<string>("match");
      auto jsonFields = node.get_child("fields");
      auto fields = getFields(jsonFields);
      filterToAdd = MultiFilter(fields, matchValue);
    }
    else
    {
      // it's an object
      if (jsonSub.data().empty())
      {
        filterToAdd = parseObjectNode(jsonSub, jsonKey);
      }
      else
      {
        if (isFilterFunction(jsonKey))
        {
          filterToAdd = ScalarFilter("match", jsonKey, jsonSub.data());
        }
        else
        {
          filterToAdd = ScalarFilter(jsonKey, topLevelField, child.second.data());
        }
      }
    }
    filter.addSubFilter(filterToAdd);
  }

  return filter;
}

GenericFilter FilterParser::ParseJsonTree(JsonNode node)
{
  return parseJson(node);
}