#ifndef _ZKCLIENT_H_
#define _ZKCLIENT_H_
#include <string>
#include <deque>
#include <set>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>
#include <zookeeper/zookeeper.h>
#include <gflags/gflags.h>
#include "zkcallback.h"
#include "task_queue.h"

DECLARE_string(zk_host);
DECLARE_string(zk_root);
DECLARE_int32(zk_session_timeout);

class TaskRunner {
public:
TaskRunner(TaskQueue<ZKTaskWrapper>* queue)
    : queue_(queue), stop_(false) {
  }
  void operator()() {
    while (!stop_) {
      google::FlushLogFiles(google::INFO);
      ZKTaskWrapper task_wrapper = queue_->pop_front();
      LOG(INFO)<< "task_type: " << ZKTaskWrapper::task_type(task_wrapper.priority_);
      int rc = task_wrapper.task_();
      switch (rc) {
        case ZOK:
          break;
        default:
          sleep(15);
          LOG(WARNING)<< "task failed, repeat: rc " << rc << ": " << zerror(rc);
          // give expired task a chance to run
          queue_->push_back(task_wrapper);
          break;
      }
    }
  }
  void stop() {
    stop_ = true;
  }
private:
  TaskQueue<ZKTaskWrapper>* queue_;
  bool stop_;
};
enum ZKClientRole {
  ACCESSOR,
  PUBLISHER
};

struct AuthInfo {
  std::string user_;
  std::string passwd_;
 AuthInfo(const std::string& user, const std::string& passwd)
 : user_(user),
    passwd_(passwd) {

 }
  bool operator<(const AuthInfo& rhs) const {
    return user_ < rhs.user_ || passwd_ < rhs.passwd_;
  }
};
template<ZKClientRole role>
class ZKClient : public boost::noncopyable {
public:
  typedef std::map<std::string, ZKCallbackPtr> ListenerMap;
  static ZKClient<role>* Instance() {
    if (!s_self_) {
      boost::mutex::scoped_lock lock(singleton_mutex_);
      if (!s_self_) {
        s_self_ = new ZKClient();
      }
    }
    return s_self_;
  }
protected:
ZKClient()
    : host_(FLAGS_zk_host),
      root_(FLAGS_zk_root),
      session_timeout_(FLAGS_zk_session_timeout),
      zh_(NULL),
      excutor_(new boost::thread(TaskRunner(&tasks_))) {
    reinit_session();
  }
  ~ZKClient() {
    if (zh_) {
      zookeeper_close(zh_);
    }
  }
public:
  void add_expired_listener(const std::string& path,
                            const ZKCallbackPtr& listener) {
    tasks_.push_back(
        ZKTaskWrapper(
            boost::bind(&ZKClient::add_expired_listener_task, this, path,
                        listener),
            ZKTaskWrapper::ADD_LISTENER));
  }
  void add_auth(const std::string& user, const std::string& passwd) {
    tasks_.push_back(
        ZKTaskWrapper(
            boost::bind(&ZKClient::add_auth_task, this, user, passwd), ZKTaskWrapper::ADD_AUTH));
  }
  void get_children(const std::string& path, bool watch,
                    const ZKCallbackPtr& callback) {
    tasks_.push_back(
        ZKTaskWrapper(
            boost::bind(&ZKClient::get_children_task, this, path, watch,
                        callback)));
  }
  void get_children_data(const std::vector<std::string>& nodes, bool watch,
                         const ZKCallbackPtr& callback) {
    tasks_.push_back(
        ZKTaskWrapper(
            boost::bind(&ZKClient::get_children_data_task, this, nodes, watch,
                        callback)));
  }
  void get_data(const std::string& path, bool watch,
                const ZKCallbackPtr& callback) {
    tasks_.push_back(
        ZKTaskWrapper(
            boost::bind(&ZKClient::get_data_task, this, path, watch,
                        callback)));
  }
private:
  // called in zookeeper completion thread
  void reinit_session() {
    tasks_.push_back(
        ZKTaskWrapper(boost::bind(&ZKClient::reinit_session_task, this),
                      ZKTaskWrapper::INIT));
  }
  void data_changed(const std::string& path) {
    tasks_.push_back(
        ZKTaskWrapper(boost::bind(&ZKClient::data_changed_task, this, path)));
  }
  void children_changed(const std::string& path) {
    tasks_.push_back(
        ZKTaskWrapper(boost::bind(&ZKClient::children_changed_task, this, path)));
  }
private:
  int add_auth_task(const std::string& user, const std::string& passwd) {
    AuthInfo auth(user, passwd);
    auth_infos_.insert(auth);

    const std::string scheme("digest");
    std::string cert = user + ":" + passwd;
    int rc = zoo_add_auth(zh_, scheme.c_str(), cert.c_str(), cert.size(), NULL,
                          NULL);
    LOG(INFO) << __func__ << " rc: " << rc << ": " << zerror(rc) << ", (" << user << " " << passwd << ")";
    return rc;
  }
  int get_children_task(const std::string& path, bool watch,
                        const ZKCallbackPtr& callback) {
    children_listeners_.insert(ListenerMap::value_type(path, callback));

    struct String_vector strings;
    memset(&strings, 0, sizeof(struct String_vector));
    int rc = ZOK;
    if (!watch) {
      rc = zoo_get_children(zh_, path.c_str(), 0, &strings);
    } else {
      rc = zoo_wget_children(zh_, path.c_str(), ZKClient<role>::event_watcher, this, &strings);
    }
    if (rc == ZOK) {
      std::vector<std::string> children;
      for (int32_t i = 0; i < strings.count; ++i) {
        children.push_back(path + "/" + std::string(strings.data[i]));
      }
      callback->get_children_cb(rc, path, children);
    }
    deallocate_String_vector(&strings);
    LOG(INFO) << __func__ << " rc: " << rc << ": " << zerror(rc) << ", path: " << path;    
    return rc;
  }
  int get_children_data_task(const std::vector<std::string>& nodes, bool watch,
                             const ZKCallbackPtr& callback) {
    std::map<std::string, std::string> data;
    const size_t kBufSize = 20 * 1024;
    char buf[kBufSize];
    int rc = ZOK;
    int buf_len = kBufSize;
    for (size_t i = 0; i < nodes.size(); ++i) {
      content_listeners_.insert(ListenerMap::value_type(nodes[i], callback));
      buf_len = kBufSize;
      memset(buf, 0, buf_len);
      if (!watch) {
        rc = zoo_get(zh_, nodes[i].c_str(), 0, buf, &buf_len, NULL);
      } else {
        rc = zoo_wget(zh_, nodes[i].c_str(), ZKClient<role>::event_watcher, this, buf, &buf_len,
                      NULL);
      }
      if (rc == ZNONODE) {
        LOG(WARNING) << __func__ << " rc: " << rc << ": " << zerror(rc) << ", path: " << nodes[i];
        rc = ZOK;
        continue;
      } else if (rc == ZOK) {
        LOG(INFO) << __func__ << " rc: " << rc << ": " << zerror(rc) << ", path: " << nodes[i];
        data[nodes[i].c_str()] = std::string(buf);
      } else {
        LOG(ERROR) << __func__ << " rc: " << rc << ": " << zerror(rc) << ", path: " << nodes[i];        
        return rc;
      }
    }
    callback->get_children_data_cb(rc, data);
    LOG(INFO) << __func__ << " rc: " << rc << ": " << zerror(rc);
    return ZOK;
  }
  int get_data_task(const std::string& path, bool watch,
                    const ZKCallbackPtr& callback) {
    content_listeners_.insert(ListenerMap::value_type(path, callback));
    std::string data;
    const size_t kBufSize = 20 * 1024;
    char buf[kBufSize] = {0};
    int rc = ZOK;
    int buf_len = kBufSize;
    if (!watch) {
      rc = zoo_get(zh_, path.c_str(), 0, buf, &buf_len, NULL);
    } else {
      rc = zoo_wget(zh_, path.c_str(), ZKClient<role>::event_watcher, this, buf, &buf_len,
                    NULL);
    }
    if (rc == ZOK) {
      data.assign(buf);
      callback->get_data_cb(rc, path, data);
      LOG(INFO) << __func__ << " rc: " << rc << ": " << zerror(rc) << ", path: " << path;
    } else if (rc == ZNONODE) {
      LOG(ERROR) << __func__ << " rc: " << rc << ": " << zerror(rc) << ", path: " << path;
      rc = ZOK;
    }
    return rc;
  }
  int add_expired_listener_task(const std::string& path,
                                const ZKCallbackPtr& listener) {
    expired_listeners_.insert(ListenerMap::value_type(path, listener));
    return ZOK;
  }
  int reinit_session_task() {
    LOG(WARNING)<< "reinit_session_task";
    init();
    for (std::set<AuthInfo>::iterator it = auth_infos_.begin(); it != auth_infos_.end(); ++it) {
      add_auth(it->user_, it->passwd_);
    }
    for (ListenerMap::iterator it = expired_listeners_.begin(); it != expired_listeners_.end(); ++it) {
      ListenerMap::mapped_type listener = it->second;
      listener->expired_event_cb(it->first);
    }
    return ZOK;
  }
  int data_changed_task(const std::string& path) {
    ListenerMap::iterator it = content_listeners_.find(path);
    if (it != content_listeners_.end()) {
      it->second->data_changed_event_cb(path);
    }
    return ZOK;
  }
  int children_changed_task(const std::string& path) {
    ListenerMap::iterator it = children_listeners_.find(path);
    if (it != children_listeners_.end()) {
      it->second->children_changed_event_cb(path);
    }
    return ZOK;
  }
  //////
  int init() {
    if (zh_) {
      zookeeper_close(zh_);
    }
    std::string host(host_);
    if (host.empty()) {
      LOG(FATAL) << "zookeeper server is not specified";
    }
    if (!root_.empty()) {
      if (!boost::algorithm::starts_with(root_, "/")) {
        host += "/" + root_;
      }
      else {
        host += root_;
      }
    } else {
      LOG(WARNING) << "zookeeper path root is empty";
    }
    volatile int state = 0;
    while (true) {
      LOG(INFO) << "zookeeper_init host: " << host << ", session_timeout: " << session_timeout_ << "s";
      zh_ = zookeeper_init(host.c_str(), ZKClient<role>::init_watcher, session_timeout_ * 1000, NULL,
                           reinterpret_cast<void*>(const_cast<int*>(&state)), 0);
      if (zh_) {
        break;
      }
      // 当host连不上时，zh_为NULL
      // schedule一个重试任务
      LOG(ERROR) << "zookeeper_init handler is NULL, probably network is down";
      sleep(15);
    }

    // check state
    int count = 1000;
    while (--count > 0) {
      if (state == ZOO_CONNECTED_STATE) {
        break;
      }
      usleep(20);
    }
    LOG(INFO)<< "init: state: " << state;
    return state;
  }
  static void init_watcher(zhandle_t *zh, int type, int state, const char *path,
                           void* watcher_ctx) {
    LOG(WARNING) << "InitWatcher type: " << type << " "
                 << watcherEvent2String(type) << " state: " << state << " "
                 << state2String(state) << " path: " << path;
    if (state == ZOO_CONNECTED_STATE) {
      int *ret_state = reinterpret_cast<int*>(watcher_ctx);
      *ret_state = state;
    }
    if (type == ZOO_SESSION_EVENT) {
    } else if (type == ZOO_CHANGED_EVENT) {
    } else if (type == ZOO_CHILD_EVENT) {
    } else if (type == ZOO_DELETED_EVENT) {
    } else if (type == ZOO_CREATED_EVENT) {
    }
  }
  static void event_watcher(zhandle_t *zh, int type, int state,
                            const char *path, void *watcher_ctx) {
    LOG(WARNING) << "EventWatcher type: " << type << " "
                 << watcherEvent2String(type) << " state: " << state << " "
                 << state2String(state) << " path: " << path;
    ZKClient<role> *client = reinterpret_cast<ZKClient<role>*>(watcher_ctx);
    if (type == ZOO_SESSION_EVENT) {
      if (state == ZOO_EXPIRED_SESSION_STATE) {
        sleep(5);
        client->reinit_session();
      }
    } else if (type == ZOO_CHANGED_EVENT) {
      client->data_changed(path);
    } else if (type == ZOO_CHILD_EVENT) {
      client->children_changed(path);
    } else if (type == ZOO_DELETED_EVENT) {
      // should delete unused listener
    } else if (type == ZOO_CREATED_EVENT) {
      // no op
    }
  }
  /**
   * @see zookeeper.c watcherEvent2String
   */
  static const char* watcherEvent2String(const int ev) {
    if (ev == 0) {
      return "ZOO_ERROR_EVENT";
    } else if (ev == ZOO_CREATED_EVENT) {
      return "ZOO_CREATED_EVENT";
    } else if (ev == ZOO_DELETED_EVENT) {
      return "ZOO_DELETED_EVENT";
    } else if (ev == ZOO_CHANGED_EVENT) {
      return "ZOO_CHANGED_EVENT";
    } else if (ev == ZOO_CHILD_EVENT) {
      return "ZOO_CHILD_EVENT";
    } else if (ev == ZOO_SESSION_EVENT) {
      return "ZOO_SESSION_EVENT";
    } else if (ev == ZOO_NOTWATCHING_EVENT) {
      return "ZOO_NOTWATCHING_EVENT";
    }

    return "INVALID_EVENT";
  }
  /**
   * @see zookeeper.c state2String
   */
  static const char* state2String(const int state) {
    if (state == 0) {
      return "ZOO_CLOSED_STATE";
    } else if (state == ZOO_CONNECTING_STATE) {
      return "ZOO_CONNECTING_STATE";
    } else if (state == ZOO_ASSOCIATING_STATE) {
      return "ZOO_ASSOCIATING_STATE";
    } else if (state == ZOO_CONNECTED_STATE) {
      return "ZOO_CONNECTED_STATE";
    } else if (state == ZOO_EXPIRED_SESSION_STATE) {
      return "ZOO_EXPIRED_SESSION_STATE";
    } else if (state == ZOO_AUTH_FAILED_STATE) {
      return "ZOO_AUTH_FAILED_STATE";
    }
    return "INVALID_STATE";
  }

private:
  std::string host_;
  std::string root_;
  int session_timeout_;
  zhandle_t *zh_;
  TaskQueue<ZKTaskWrapper> tasks_;
  boost::shared_ptr<boost::thread> excutor_;
  ListenerMap expired_listeners_;
  ListenerMap children_listeners_;
  ListenerMap content_listeners_;
  std::set<AuthInfo> auth_infos_;
  static boost::mutex singleton_mutex_;
  static ZKClient<role>* s_self_;
};

template<ZKClientRole role>
boost::mutex ZKClient<role>::singleton_mutex_;

template<ZKClientRole role>
ZKClient<role>* ZKClient<role>::s_self_ = NULL;

#endif
