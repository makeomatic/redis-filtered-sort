#ifndef __AGGREGATE_COMMAND
#define __AGGREGATE_COMMAND

#include <string>
#include <map>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "util/data.hpp"
#include "redis/context.cpp"

#include "./cmd-args.hpp"

namespace ms {
  namespace pt = boost::property_tree;

  class AggregateCommand {
  private:
    AggregateArgs args;
    boost::property_tree::ptree jsonFilters;

  public:
    explicit AggregateCommand(AggregateArgs &args): args(args) {
      stringstream jsonText;
      jsonText << args.filter;
      boost::property_tree::json_parser::read_json(jsonText, jsonFilters);
    };

    ~AggregateCommand() = default;

    int execute(redis::Context redis) {
      auto presortedData = Data(redis, args.metaKey);
      presortedData.loadList(args.pssKey);

      if (presortedData.size() > 0) {
        vector<pair<string, string>> aggregateFields;
        vector<string> fieldsNeeded;

        for (pt::ptree::value_type &node : jsonFilters) {
          auto pair = make_pair(node.first.data(), node.second.data());
          aggregateFields.emplace_back(pair);
          fieldsNeeded.emplace_back(node.first.data());
        }

        if (!aggregateFields.empty()) {
          pt::ptree resultDoc = pt::ptree();
          map<string, long> aggregated;

          auto allMetaData = presortedData.loadMetadata(fieldsNeeded);

          for (const auto& filterRecord : allMetaData) {
            map<string, string> filterData = filterRecord.second;

            for (const auto& aggregateField : aggregateFields) {
              string aggrFn = aggregateField.second;
              string aggrField = aggregateField.first;
              auto recordValue = filterData[aggrField];

              if (aggrFn == "sum") {
                long parsedValue = stoll(recordValue.empty() ? "0" : recordValue);
                aggregated[aggrField] = aggregated[aggrField] + parsedValue;
              }
            }
          }

          for (auto &aggrResult : aggregated) {
            resultDoc.put(aggrResult.first, aggrResult.second);
          }

          stringstream responseStream;

          pt::write_json(responseStream, resultDoc, false);
          string response = responseStream.str();

          std::cerr << "Responding:" << response << "\n";
          redis.respondString(response);

          return 0;
        }
      }
      string emptyResponse = "{}";
      redis.respondString(emptyResponse);
      return 1;
    };
  };
} // namespace ms

#endif