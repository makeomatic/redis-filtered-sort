#include "aggregate-command.hpp"

#include <string>
#include <map>
#include <iostream>
#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "util/data.hpp"

using namespace ms;
using namespace std;
using namespace boost::algorithm;
namespace pt = boost::property_tree;

AggregateCommand::AggregateCommand(AggregateArgs args) {
  this->args = args;
  this->init();
}

AggregateCommand::~AggregateCommand() {
}

void log(string message) {
  std::cout << "LOG--->" << message << "\n";
}

void AggregateCommand::init() {
  try {
    stringstream jsonText;
    jsonText << args.filter;
    boost::property_tree::json_parser::read_json(jsonText, this->jsonFilters);
  }
  catch (exception &e) {
    log(e.what());
  }
}

int AggregateCommand::execute(redis::Context redis) {
  auto presortedData = Data(redis, args.metaKey);
  presortedData.loadList(args.pssKey);

  if (presortedData.size() > 0) {
    vector<pair<string, string>> aggregateFields;
    vector<string> fieldsNeeded;

    for (pt::ptree::value_type &node : this->jsonFilters) {
      auto pair = make_pair(node.first.data(), node.second.data());
      aggregateFields.push_back(pair);
      fieldsNeeded.push_back(node.first.data());
    }

    if (aggregateFields.size() > 0) {
      auto allMetaData = presortedData.loadMetadata(fieldsNeeded);

      pt::ptree resultDoc = pt::ptree();

      map<string, long> aggregated;
      std::cerr << "Field needed:" << aggregateFields.size() << "\n";
      std::cerr << "Metadata size:" << allMetaData.size() << "\n";

      for (size_t i = 0; i < allMetaData.size(); i++) {
        pair<string, map<string, string>> filterRecord = allMetaData[i];
        map<string, string> filterData = filterRecord.second;

        for (size_t fieldIndex = 0; fieldIndex < aggregateFields.size(); fieldIndex++) {
          pair<string, string> aggregateField = aggregateFields[fieldIndex];
          string aggrFn = aggregateField.second;
          string aggrField = aggregateField.first;
          auto recordValue = filterData[aggrField];

          if (aggrFn.compare("sum") == 0) {
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

  redis.respondString("{}");
  return 1;
}