/*
 * pool_demo.cc
 *
 *  Created on: Aug 12, 2013
 *      Author: root
 */
#include "pool_demo.h"

void PoolDemo::add_expired_listener(const std::string& path) {
  client_->add_expired_listener(path, shared_from_this());
}
void PoolDemo::add_auth(const std::string& user, const std::string& passwd) {
  //client_.add_auth(user, passwd, boost::dynamic_pointer_cast<PoolDemo>(shared_from_this()));
  client_->add_auth(user, passwd);
}
void PoolDemo::get_children(const std::string& path, bool watch) {
  // /hw.test.xoa.renren.com/1/0"
  client_->get_children(path, watch, shared_from_this());
}
PoolDemo::PoolDemo() {
  client_ = ZKClient<ACCESSOR>::Instance();
}
void PoolDemo::get_children_cb(int rc, const std::string& path,
                               const std::vector<std::string>& children) {
  ZKDefaultCallback::get_children_cb(rc, path, children);
  client_->get_children_data(children, true, shared_from_this());
}
void PoolDemo::get_children_data_cb(int rc,
                           const std::map<std::string, std::string>& data) {
  ZKDefaultCallback::get_children_data_cb(rc, data);
}
void PoolDemo::get_data_cb(int rc, const std::string& path, const std::string& data) {
  ZKDefaultCallback::get_data_cb(rc, path, data);
}

void PoolDemo::deleted_event_cb(const std::string& path) {
  ZKDefaultCallback::deleted_event_cb(path);
}
void PoolDemo::children_changed_event_cb(const std::string& path) {
  ZKDefaultCallback::children_changed_event_cb(path);
  client_->get_children(path, true, shared_from_this());
}
void PoolDemo::data_changed_event_cb(const std::string& path) {
  ZKDefaultCallback::data_changed_event_cb(path);
  client_->get_data(path, true, shared_from_this());
}
void PoolDemo::expired_event_cb(const std::string& path) {
  ZKDefaultCallback::expired_event_cb(path);
  client_->get_children(path, true, shared_from_this());
}

