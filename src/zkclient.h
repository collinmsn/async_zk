#ifndef _ZKCLIENT_H_
#define _ZKCLIENT_H_
#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/locks.hpp>
#include <glog/logging.h>
#include <zookeeper/zookeeper.h>
#include "zkcallback.h"
#include <deque>

enum ZKClientRole {
  ACCESSOR,
  PUBLISHER
};

template <ZKClientRole role>
class ZKClient {
public:
  typedef boost::function<int()> ZKTask;
public:
ZKClient(const std::string& host, const std::string& root, int session_timeout) : host_(host), root_(root), session_timeout_(session_timeout) {
    zh_ = NULL;
  }
  ~ZKClient() {  if (zh_) {
      zookeeper_close(zh_);
    }}
  void add_auth(const std::string& user, const std::string& passwd, const ZKCallbackPtr& callback) {
    tasks_.push_back(boost::bind(&ZKClient::add_auth_task, this, user, passwd, callback));
  }
private:
  int add_auth_task(const std::string& user, const std::string& passwd, const ZKCallbackPtr& callback) {
    while (!is_connected()) {}
    const std::string scheme("digest");
    std::string cert = user + ":" + passwd;
    int rc = zoo_add_auth(zh_, scheme.c_str(), cert.c_str(), cert.size(), NULL, NULL);
    if (rc == ZOK) {
      callback->add_auth_cb(rc, user, passwd);
    }
  
    return rc;
  }
  int init() {
    if (zh_) {
      zookeeper_close(zh_);
    }
    int state = 0;
    zh_ = zookeeper_init(host_.c_str(), init_watcher, session_timeout_, NULL, static_cast<void*>(&state), 0);
    boost::unique_lock<boost::mutex> lock(init_mutex_);
    init_cond_.wait(lock);
    // init finished
    LOG(INFO) << "init: state: " << state;
    return state;
  }
  static void init_watcher(zhandle_t *zh, int type, int state, const char *path, void* watcher_ctx) {
    LOG(INFO) << "init_watcher: state: " << state;
    int *return_state = static_cast<int*>(watcher_ctx);
    *return_state = state;
    init_cond_.notify_one();
  }
private:
  std::string host_;
  std::string root_;
  int session_timeout_;
  zhandle_t *zh_;
  std::deque<ZKTask> tasks_;
  boost::mutex init_mutex_;
  boost::condition_variable init_cond_;
};
#endif
