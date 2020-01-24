#ifndef REDIS_FILTERED_SORT_TASK_REGISTRY_H
#define REDIS_FILTERED_SORT_TASK_REGISTRY_H

#include <string>
#include <boost/signals2.hpp>
#include <map>
#include <mutex>

namespace ms {
  using namespace std;
  namespace signals = boost::signals2;

  class NoTaskException : public std::logic_error {
  public:
    NoTaskException(string message) : std::logic_error(message) {};
  };

  class Observer {
  public:
    Observer(const Observer&) = delete;
    Observer& operator=(const Observer&) = delete;
    Observer() = default;
    signals::signal<void()> signal;
  };

  class TaskRegistry {
  private:
    mutex globalMutex;
    map<string, mutex> slotMutex;
    map<string, Observer> cmdSlots;

    bool _taskExists(const string& taskName) {
      return cmdSlots.find(taskName) != cmdSlots.end();
    };

  public:
    typedef function<void()> CallbackFn;

    bool taskExists(string taskName) {
      globalMutex.lock();
      auto result = _taskExists(taskName);
      globalMutex.unlock();
      return result;
    }

    void addTask(const string& taskName) {
      if (taskExists(taskName)) {
        throw NoTaskException("Task exists");
      }

      globalMutex.lock();

      cmdSlots[taskName];
      slotMutex[taskName];

      globalMutex.unlock();
    };

    void deleteTask(const string& taskName) {
      globalMutex.lock();
      if (_taskExists(taskName)) {
        slotMutex[taskName].lock();

        auto &obs = cmdSlots.at(taskName);
        obs.signal.disconnect_all_slots();
        cmdSlots.erase(taskName);

        slotMutex[taskName].unlock();
        globalMutex.unlock();
      } else {
        globalMutex.unlock();
        throw NoTaskException("No such task");
      }
    }

    signals::connection registerWaiter(const string& taskName, const CallbackFn& cb) {
      if (taskExists(taskName)) {
        slotMutex[taskName].lock();

        auto &obs = cmdSlots.at(taskName);
        auto signalConnection = obs.signal.connect(cb);

        slotMutex[taskName].unlock();
        return signalConnection;
      } else {
        throw NoTaskException("No such task");
      };
    }

    void notifyAndDelete(const string& taskName) {
      if (taskExists(taskName)) {
        slotMutex[taskName].lock();
        auto &obs = cmdSlots.at(taskName);
        obs.signal();
        slotMutex[taskName].unlock();
        deleteTask(taskName);
      }
    }

    void deleteWaiter(const string& taskName, const signals::connection& slot) {
      if (_taskExists(taskName)) {
        slotMutex[taskName].lock();

        auto &obs = cmdSlots.at(taskName);
        obs.signal.disconnect(slot);

        slotMutex[taskName].unlock();
      } else {
        throw NoTaskException("No such task");
      }
    }
  };

}

#endif //REDIS_FILTERED_SORT_TASK_REGISTRY_H
