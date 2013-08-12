#include "zkdefaultcallback.h"
#include <glog/logging.h>

void ZKDefaultCallback::add_auth_cb(int rc, const std::string& user, const std::string& passwd) {
  LOG(INFO) << "rc: " << rc << " user: " << user << " passwd: " <<  passwd;
}
