#ifndef REDIS_FILTERED_SORT_THREAD_POOL
#define REDIS_FILTERED_SORT_THREAD_POOL

#include<boost/thread.hpp>
#include <boost/asio.hpp>
#include <iostream>

namespace ms {
  class ThreadPool {
  public:
    ThreadPool(size_t size) : work_(io_service_) {
      for (size_t i = 0; i < size; ++i) {
        workers_.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));
      }
    };

    ~ThreadPool() {
      io_service_.stop();
      workers_.join_all();
    };

    template<class F> void Enqueue(F f) {
      io_service_.post(f);
    };

  private:
    boost::thread_group workers_;
    boost::asio::io_service io_service_;
    boost::asio::io_service::work work_;
  };
}

#endif