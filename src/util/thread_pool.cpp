#include "thread_pool.hpp"

namespace taraxa::util {

ThreadPool::ThreadPool(size_t num_threads, bool _start)
    : ioc_(num_threads), ioc_work_(boost::asio::make_work_guard(ioc_)) {
  assert(0 < num_threads);
  threads_.reserve(num_threads);
  if (_start) {
    start();
  }
}

void ThreadPool::start() {
  std::unique_lock l(threads_mu_);
  if (!threads_.empty()) {
    return;
  }
  for (uint i = 0; i < threads_.capacity(); ++i) {
    threads_.emplace_back([this] { ioc_.run(); });
  }
}

bool ThreadPool::is_running() const {
  std::shared_lock l(threads_mu_);
  return !threads_.empty();
}

void ThreadPool::stop() {
  std::unique_lock l(threads_mu_);
  ioc_.stop();
  for (auto &th : threads_) {
    th.join();
  }
  threads_.clear();
  ioc_.restart();  // for potential restart
}

void ThreadPool::post(uint64_t do_in_ms, asio_callback action) {
  ++debug_num_pending_tasks_;
  if (!do_in_ms) {
    boost::asio::post(ioc_, [this, action = std::move(action)] {
      action({});
      --debug_num_pending_tasks_;
    });
    return;
  }
  auto timer = std::make_shared<boost::asio::deadline_timer>(ioc_);
  timer->expires_from_now(boost::posix_time::milliseconds(do_in_ms));
  timer->async_wait([this, action = std::move(action), timer](auto const &err_code) {
    action(err_code);
    --debug_num_pending_tasks_;
  });
}

void ThreadPool::post(uint64_t do_in_ms, std::function<void()> action) {
  post(do_in_ms, [action = std::move(action)](auto const &err) {
    if (!err) {
      action();
      return;
    }
    assert(err == boost::asio::error::operation_aborted);
  });
}

void ThreadPool::post_loop(Periodicity const &periodicity, std::function<void()> action) {
  post(periodicity.delay_ms, [=, action = move(action)]() mutable {
    action();
    post_loop({periodicity.period_ms}, move(action));
  });
}

ThreadPool::~ThreadPool() { stop(); }

}  // namespace taraxa::util
