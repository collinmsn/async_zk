#ifndef ASYNC_ZK_ZKCALLBACK_H_
#define ASYNC_ZK_ZKCALLBACK_H_
#include <map>
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace async_zk {
  /**
   * callback interfaces
   */

  class ZKCallback : public boost::enable_shared_from_this<ZKCallback> {
  public:
    virtual ~ZKCallback(){};
    // called when get children command finished 
    virtual void get_children_cb(int rc, const std::string& path,
				 const std::vector<std::string>& children) = 0;
    // called when get children's data command finished
    virtual void get_children_data_cb(int rc,
				      const std::map<std::string, std::string>& data) = 0;
    // called when get node's data finished
    virtual void get_data_cb(int rc, const std::string& path, const std::string& data) = 0;
    // called when a node is deleted
    // NOTE: I do not use the created event because I am not quite aware of its behavior
    virtual void deleted_event_cb(const std::string& path) = 0;
    // called when a node's children changed
    virtual void children_changed_event_cb(const std::string& path) = 0;
    // called when a node's data changed
    virtual void data_changed_event_cb(const std::string& path) = 0;
    // called when client expired
    virtual void expired_event_cb(const std::string& path) = 0;
  };

  typedef boost::shared_ptr<ZKCallback> ZKCallbackPtr;
} // end of namespace async_zk
#endif
