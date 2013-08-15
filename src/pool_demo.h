/*
 * pool_demo.h
 *
 *  Created on: Aug 12, 2013
 *      Author: root
 */

#ifndef _POOL_DEMO_H_
#define _POOL_DEMO_H_
#include <vector>
#include <string>
#include "zkdefaultcallback.h"
#include "zkclient.h"

class PoolDemo : public ZKDefaultCallback {
 public:
  PoolDemo();
  void add_expired_listener(const std::string& path);
  void add_auth(const std::string& user, const std::string& passwd);
  void get_children(const std::string& path, bool watch);
 public:
  virtual void get_children_cb(int rc, const std::string& path,
                               const std::vector<std::string>& children);
  virtual void get_children_data_cb(int rc,
                           const std::map<std::string, std::string>& data);
  virtual void get_data_cb(int rc, const std::string& path, const std::string& data);
  virtual void created_event_cb(const std::string& path);
  virtual void deleted_event_cb(const std::string& path);
  virtual void children_changed_event_cb(const std::string& path);
  virtual void data_changed_event_cb(const std::string& path);
  virtual void expired_event_cb(const std::string& path);
 private:
  ZKClient<ACCESSOR> *client_;
  std::vector<std::string> endpoints_;
};

#endif /* _POOL_DEMO_H_ */
