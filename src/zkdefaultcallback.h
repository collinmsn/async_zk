#ifndef _ZK_DEFAULT_CALLBACK_H_
#define _ZK_DEFAULT_CALLBACK_H_
#include <string>
#include "zkcallback.h"

class ZKDefaultCallback : public ZKCallback{
public:
  virtual void add_auth_cb(int rc, const std::string& user, const std::string& passwd);
  
};

#endif
