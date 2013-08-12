#ifndef _ZK_CALLBACK_H_
#define _ZK_CALLBACK_H_
#include <string>
#include <boost/shared_ptr.hpp>

class ZKCallback {
public:
  virtual void add_auth_cb(int rc, const std::string& user, const std::string& passwd) = 0;
  
};

typedef boost::shared_ptr<ZKCallback> ZKCallbackPtr;
#endif
