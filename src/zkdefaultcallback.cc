#include "zkdefaultcallback.h"
#include <glog/logging.h>

namespace async_zk {
void ZKDefaultCallback::get_children_cb(int rc, const std::string& path, const std::vector<std::string>& children) {
  LOG(INFO) << "rc: " << rc << " path: " << path;
  for (size_t i = 0; i < children.size(); ++i) {
    LOG(INFO) << " " << children[i];
  }
  google::FlushLogFiles(google::INFO);
}
void ZKDefaultCallback::get_children_data_cb(int rc, const std::map<std::string, std::string>& data) {
  LOG(INFO) << "rc: " << rc;
  for (std::map<std::string, std::string>::const_iterator it = data.begin(); it != data.end(); ++it) {
    LOG(INFO) << "path: " << it->first << " value: " << it->second;
  }
  google::FlushLogFiles(google::INFO);
}
void ZKDefaultCallback::get_data_cb(int rc, const std::string& path, const std::string& data) {
  LOG(INFO) << "rc: " << rc << " path: " << path << " value: " << data;
}

void ZKDefaultCallback::deleted_event_cb(const std::string& path) {
  LOG(INFO) << "path deleted: " << path;
}

void ZKDefaultCallback::children_changed_event_cb(const std::string& path) {
  LOG(INFO) << "children changed: " << path;
}
void ZKDefaultCallback::data_changed_event_cb(const std::string& path) {
  LOG(INFO) << "data changed: " << path;
}
void ZKDefaultCallback::expired_event_cb(const std::string& path) {
  LOG(INFO) << "expired: " << path;
}
} // end of namespace async_zk
