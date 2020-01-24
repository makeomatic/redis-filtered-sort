#ifndef REDIS_FILTERED_SORT_THREAD_POOL_HPP
#define REDIS_FILTERED_SORT_THREAD_POOL_HPP

#include<boost/thread.hpp>
#include <boost/asio.hpp>

namespace ms {
  class ThreadPool {
  public:
    ThreadPool(size_t size);

    ~ThreadPool();

    template<class F> void Enqueue(F f) {
      io_service_.post(f);
    };

  private:
    boost::thread_group workers_;
    boost::asio::io_service io_service_;
    boost::asio::io_service::work work_;
  };
}

#endif //REDIS_FILTERED_SORT_THREAD_POOL_HPP
