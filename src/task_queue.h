/*
 * task_queue.h
 *
 *  Created on: Aug 12, 2013
 *      Author: Huabin Zheng
 */

#ifndef ASYNC_ZK_TASK_QUEUE_H_
#define ASYNC_ZK_TASK_QUEUE_H_
#include <sys/time.h>
#include <stdio.h>
#include <queue>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>
#include <glog/logging.h>

namespace async_zk {

  typedef boost::function<int()> ZKTask;

  struct ZKTaskWrapper {
    // zk任务放到一个优先队列中，按优先级执行
    // 优先级按大类分成ADD_LISTENER, INIT等几类
    // 同类型优先级的任务，按添加时间再进行排序
    enum TaskType {
      ADD_LISTENER_TASK,
      // zookeeper_init
      INIT_TASK,
      ADD_AUTH_TASK,
      OTHER_TASK,
      NUM_TASK_TYPE
    };
    ZKTaskWrapper() {
    }

  ZKTaskWrapper(const ZKTask& task, TaskType task_type = OTHER_TASK) : task_(task) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    priority_ = tv.tv_sec * 1000000 + tv.tv_usec;
    priority_ &= kMask;
    priority_ |= (uint64_t(task_type) << kMaskBits);
    LOG(INFO) << "task type: " << task_type << " priority: " << priority_;
  }

    static const std::string task_type(const ZKTaskWrapper& task) {
      int type = (task.priority_ >> kMaskBits);
      return kTaskTypeNames[type];
    }
    bool operator<(const ZKTaskWrapper& rhs) const {
      // 值越大，优先级越低
      return priority_ > rhs.priority_;
    }
    int operator()() {
      return task_();
    }

    ZKTask task_;
    uint64_t priority_;
    static const size_t kMaskBits = 56;
    static const uint64_t kMask = (1LLU << kMaskBits) - 1;
    static const char* kTaskTypeNames[NUM_TASK_TYPE];
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
    std::pair<T, bool> pop_front() {
      boost::unique_lock<boost::mutex> lock(mutex_);
      if (queue_.empty()) {
	boost::posix_time::milliseconds timeout(15000);
	if (!cond_.timed_wait(lock, timeout)) {
	  return std::pair<T, bool>(T(), false);
	}
      }
      LOG(INFO) << "pop task, queue size: " << queue_.size();
      T item = queue_.top();
      queue_.pop();
      return std::pair<T, bool>(item, true);
    }

  private:
    boost::mutex mutex_;
    boost::condition_variable cond_;
    std::priority_queue<T> queue_;
  };
} // end of async_zk namespace

#endif /* ASYNC_ZK_TASK_QUEUE_H_ */
