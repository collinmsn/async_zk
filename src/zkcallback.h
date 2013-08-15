#ifndef _ZK_CALLBACK_H_
#define _ZK_CALLBACK_H_
#include <map>
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

class ZKCallback : public boost::enable_shared_from_this<ZKCallback> {
 public:
  virtual ~ZKCallback(){};
  virtual void get_children_cb(int rc, const std::string& path,
                               const std::vector<std::string>& children) = 0;
  virtual void get_children_data_cb(int rc,
                             const std::map<std::string, std::string>& data) = 0;
  virtual void get_data_cb(int rc, const std::string& path, const std::string& data) = 0;
  virtual void created_event_cb(const std::string& path) = 0;
  virtual void deleted_event_cb(const std::string& path) = 0;
  virtual void children_changed_event_cb(const std::string& path) = 0;
  virtual void data_changed_event_cb(const std::string& path) = 0;
  virtual void expired_event_cb(const std::string& path) = 0;
};

typedef boost::shared_ptr<ZKCallback> ZKCallbackPtr;
#endif
