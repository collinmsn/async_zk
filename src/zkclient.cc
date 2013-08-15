#include "zkclient.h"
#include <gflags/gflags.h>

DEFINE_string(zk_host, "127.0.0.1:2181", "zookeeper cluster hosts");
DEFINE_string(zk_root, "test", "zookeeper root directory");
DEFINE_int32(session_timeout, 30, "zookeeper session timeout in seconds");
