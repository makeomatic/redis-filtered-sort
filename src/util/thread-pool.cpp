#include "thread-pool.hpp"
#include <iostream>

using namespace ms;

ThreadPool::~ThreadPool() {
  std::cout << "~ThreadPool" << std::endl;
  io_service_.stop();
  workers_.join_all();
}

ThreadPool::ThreadPool(size_t size) : work_(io_service_) {
  for (size_t i = 0; i < size; ++i) {
    workers_.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));
  }
}
