
#ifndef __SORT_COMMAND
#define __SORT_COMMAND

#include "cmd-args.hpp"

#include "thread"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>

#include "util/filter/parser.cpp"
#include "redis/context.cpp"
#include "util/filter/filter.hpp"
#include "util/data.hpp"
#include "redis/redismodule.h"
#include "redis/util.cpp"
#include "util/task-registry.cpp"
#include "util/thread-pool.cpp"

namespace ms {
  using namespace std;

  class SortCommand {
  private:
    SortArgs args;
    string tempKeysSet;
    string fflKey;
    string pssKey;
    boost::property_tree::ptree jsonFilters;
    FilterInterface *filters{};
    string idFilter;
    redis::Context redis;
    TaskRegistry &taskRegistry;

  public:
    ~SortCommand() {
      delete filters;
    }

    SortCommand(SortArgs args, RedisModuleCtx *redis, TaskRegistry &registry) : args(args), redis(redis), taskRegistry(registry) {
      tempKeysSet = boost::str(boost::format("%s::fsort_temp_keys") % args.idSet);

      stringstream jsonText;
      jsonText << args.filter;
      boost::property_tree::json_parser::read_json(jsonText, jsonFilters);
      filters = FilterParser().ParseJsonTree(jsonFilters);

      std::vector<string> ffKeyOpts = {args.idSet, args.order};
      std::vector<string> psKeyOpts = {args.idSet, args.order};

      size_t filterSize = jsonFilters.size();

      if (!args.metaKey.empty()) {
        if (args.hashKey != "") {
          ffKeyOpts.push_back(args.metaKey);
          psKeyOpts.push_back(args.metaKey);

          ffKeyOpts.push_back(args.hashKey);
          psKeyOpts.push_back(args.hashKey);
        }

        if (filterSize > 0) {
          ffKeyOpts.push_back(args.filter);
        }
      } else if (filterSize >= 1) {
        boost::optional<string> optIdFilter = jsonFilters.get<string>("#");
        if (!optIdFilter.value().empty()) {
          idFilter = optIdFilter.value();
          ffKeyOpts.push_back(idFilter);
        }
      }

      fflKey = boost::join(ffKeyOpts, ":");
      pssKey = boost::join(psKeyOpts, ":");
    }

    string getTaskDescriptor() {
      return pssKey == fflKey ? pssKey : boost::str(boost::format("%s::%s") % pssKey % fflKey);
    };

    bool allTasksPending() {
      string taskDescriptor = getTaskDescriptor();
      if (sortOnly()) {
        return taskRegistry.taskExists(pssKey);
      } else {
        return taskRegistry.taskExists(taskDescriptor);
      }
    }

    void waitForPoolEvents(RedisModuleBlockedClient *bc) {
      string taskDescriptor = getTaskDescriptor();
      bool filterTaskExists = taskRegistry.taskExists(taskDescriptor);

      auto boundProcessCommand = bind(&SortCommand::sortAndFilterData, this);
      auto taskToWait = (!sortOnly() && filterTaskExists) ? taskDescriptor : pssKey;

      BOOST_LOG_TRIVIAL(debug) << "Pausing thread waiting for " << taskToWait;
      taskRegistry.registerWaiter(taskToWait, [this, boundProcessCommand, taskToWait, bc]() {
        BOOST_LOG_TRIVIAL(debug) << "Got event " << taskToWait;

        if(hasCachedFilteredData()) {
          BOOST_LOG_TRIVIAL(debug) << "\t respond cached " << taskToWait;
          respondCachedFilteredData();
        } else {
          boundProcessCommand();
        };
        redis::GlobalUtil::UnblockClient(bc, NULL);
      });
    }

    void runInPool(RedisModuleBlockedClient *bc) {
      string taskDescriptor = getTaskDescriptor();
      bool sortTaskExists = taskRegistry.taskExists(pssKey);
      bool filterTaskExists = taskRegistry.taskExists(taskDescriptor);

      BOOST_LOG_TRIVIAL(debug) << "PRE Work " << taskDescriptor << " || " << pssKey;
      BOOST_LOG_TRIVIAL(debug) << "PRE Work " << filterTaskExists << " || " << sortTaskExists;

      if (!sortTaskExists && !hasCachedSortedData()) {
        BOOST_LOG_TRIVIAL(debug) << "Adding sort task: " << pssKey;
        taskRegistry.addTask(pssKey);
      }

      if (!sortOnly() && !filterTaskExists && !hasCachedFilteredData()){
        BOOST_LOG_TRIVIAL(debug) << "Adding filter task: " << taskDescriptor;
        taskRegistry.addTask(taskDescriptor);
      }

      this->sortAndFilterData();
      redis::GlobalUtil::UnblockClient(bc, NULL);
    }

    void execute(ThreadPool *threadPool) {
      // early exit if filtered Data exist;
      if (hasCachedFilteredData()) {
        BOOST_LOG_TRIVIAL(debug) << "The most earliest exit " << pssKey << " || " << fflKey;
        return respondCachedFilteredData();
      }

      auto bc = redis::GlobalUtil::BlockClient(redis.ctx, NULL, NULL, NULL, 0);

      auto wrapperThread = thread([this, bc, threadPool]() {
        auto thSafeCtx = ms::redis::GlobalUtil::GetThreadSafeContext(bc);
        auto cmd = new SortCommand(args, thSafeCtx, taskRegistry);

        if (allTasksPending()) {
          BOOST_LOG_TRIVIAL(debug) << "All tasks pending, waiting for pool results";
          cmd->waitForPoolEvents(bc);
        } else {
          threadPool->Enqueue([bc, cmd, thSafeCtx](){
            try {
              cmd->runInPool(bc);
            } catch(std::exception &e) {
              std::string errMessage = boost::str(boost::format("Command error: %s") %e.what());
              RedisModule_ReplyWithError(thSafeCtx, errMessage.c_str());
              ms::redis::GlobalUtil::UnblockClient(bc, NULL);
            }
          });
        }
      });

      wrapperThread.detach();
    };

    void sortAndFilterData() {
      Data dataToSort = Data(redis, args.metaKey);
      bool presorted = hasCachedSortedData();

      if (presorted) {
        dataToSort.loadList(pssKey);
      } else {
        dataToSort.loadSet(args.idSet);
        BOOST_LOG_TRIVIAL(debug) << "Save and cache data " << pssKey;
        sortData(dataToSort);
        dataToSort.save(pssKey);
      };

      updateKeySetAndExpire(pssKey);

      if (!presorted) {
        BOOST_LOG_TRIVIAL(debug) << "Clean sort task " << getTaskDescriptor();
        taskRegistry.notifyAndDelete(pssKey);
      }

      if (sortOnly()) {
        BOOST_LOG_TRIVIAL(debug) << "Respond sort only " << pssKey;
        return respond(pssKey);
      }

      vector<string> filteredData;
      filteredData = args.metaKey.empty() ? dataToSort.filter(idFilter) : dataToSort.filterMeta(filters);

      dataToSort.save(fflKey, filteredData);
      updateKeySetAndExpire(fflKey);

      BOOST_LOG_TRIVIAL(debug) << "Clean filter task " << getTaskDescriptor();
      taskRegistry.notifyAndDelete(getTaskDescriptor());

      return respond(fflKey);
    };

    void respond(string key) {
      if (args.keyOnly) {
        redis.respondString(key);
      } else {
        redis.respondList(key, args.offset, args.limit);
      }
    };

    bool sortOnly() {
      return pssKey == fflKey;
    }

    bool hasCachedSortedData() {
      auto redisCommand = redis.getCommand();
      int pssKType = redisCommand.type(pssKey);
      return pssKType == REDISMODULE_KEYTYPE_LIST;
    }

    bool hasCachedFilteredData() {
      auto redisCommand = redis.getCommand();
      int fflKType = redisCommand.type(fflKey);
      return fflKType == REDISMODULE_KEYTYPE_LIST;
    };

    void respondCachedFilteredData() {
      auto redisCommand = redis.getCommand();
      auto redisData = redis.getData();
      redisCommand.pexpire(tempKeysSet, args.expire);
      redisData.registerCaches(pssKey, tempKeysSet, args.curTime, args.expire);
      redisData.registerCaches(fflKey, tempKeysSet, args.curTime, args.expire);
      BOOST_LOG_TRIVIAL(debug) << "Respond cached " << fflKey;
      return respond(fflKey);
    };

    void sortData(Data &sortData) {
      if (args.metaKey.empty() || args.hashKey.empty()) {
        sortData.sort(args.order);
      } else {
        sortData.sortMeta(args.hashKey, args.order);
      }
    };

    void updateKeySetAndExpire(const string &key) {
      auto redisCommand = redis.getCommand();
      auto redisData = redis.getData();

      redisCommand.pexpire(tempKeysSet, args.expire);
      redisData.registerCaches(key, tempKeysSet, args.curTime, args.expire);
    };
  };
} // namespace ms

#endif