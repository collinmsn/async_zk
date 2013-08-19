#ifndef _ZK_DEFAULT_CALLBACK_H_
#define _ZK_DEFAULT_CALLBACK_H_
#include <string>
#include "zkcallback.h"

/**
 * zk callback default implementation
 * just output logs to trace its behavior
 */

class ZKDefaultCallback : public ZKCallback{
public:
  virtual void get_children_cb(int rc, const std::string& path, const std::vector<std::string>& children);
  virtual void get_children_data_cb(int rc,
                             const std::map<std::string, std::string>& data);
  virtual void get_data_cb(int rc, const std::string& path, const std::string& data);
  virtual void deleted_event_cb(const std::string& path);  
  virtual void children_changed_event_cb(const std::string& path);
  virtual void data_changed_event_cb(const std::string& path);
  virtual void expired_event_cb(const std::string& path);
};

#endif
