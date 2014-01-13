#include "zkclient.h"
#include <gflags/gflags.h>

namespace async_zk {
  DEFINE_string(zk_host, "", "zookeeper cluster hosts");
  DEFINE_string(zk_root, "", "zookeeper root directory");
  DEFINE_int32(zk_session_timeout, 30, "zookeeper session timeout in seconds");
}
