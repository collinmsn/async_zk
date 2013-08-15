/*
 * task_queue.h
 *
 *  Created on: Aug 12, 2013
 *      Author: root
 */

#ifndef _TASK_QUEUE_H_
#define _TASK_QUEUE_H_
#include <sys/time.h>
#include <stdio.h>
#include <queue>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>
#include <glog/logging.h>

typedef boost::function<int()> ZKTask;
struct ZKTaskWrapper {
  enum Priority {
    ADD_LISTENER,
    INIT,
    ADD_AUTH,
    OTHER_TASK
  };
  ZKTaskWrapper(const ZKTask& task, Priority priority = OTHER_TASK) : task_(task) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    priority_ = tv.tv_sec * 1000000 + tv.tv_usec;
    assert(priority_ < kMask);
    priority_ &= kMask;
    priority_ |= (uint64_t(priority) << kMaskBits);
    LOG(INFO) << "type: " << priority << " priority: " << priority_;
  }
  static const std::string task_type(uint64_t priority) {
    int type = (priority >> kMaskBits);
    if (type == ADD_LISTENER) {
      return "ADD_LISTENER";
    } else if (type == INIT) {
      return "INIT";
    } else if (type == ADD_AUTH) {
      return "ADD_AUTH";
    } else {
      return "OTHER_TASK";
    }
    return "";
  }
  bool operator<(const ZKTaskWrapper& rhs) const {
    // 值越大，优先级越低
    return priority_ > rhs.priority_;
  }
  ZKTask task_;
  uint64_t priority_;
  static const size_t kMaskBits = 56;
  static const uint64_t kMask = (1LLU << kMaskBits) - 1;
};
template<typename T>
class TaskQueue {
 public:
  TaskQueue() {}
  virtual ~TaskQueue() {}
  void push_back(const T& item) {
    boost::mutex::scoped_lock lock(mutex_);
    queue_.push(item);
    LOG(INFO) << "push task, queue size: " << queue_.size();
    cond_.notify_one();
  }
  T pop_front() {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (queue_.empty()) {
      cond_.wait(lock);
    }
    LOG(INFO) << "pop task, queue size: " << queue_.size();
    T item = queue_.top();
    queue_.pop();
    return item;
  }

 private:
  boost::mutex mutex_;
  boost::condition_variable cond_;
  std::priority_queue<T> queue_;
};

#endif /* _TASK_QUEUE_H_ */
