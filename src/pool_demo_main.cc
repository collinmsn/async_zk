/*
 * pool_demo_main.cc
 *
 *  Created on: Aug 12, 2013
 *      Author: root
 */
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "pool_demo.h"
DEFINE_string(watch_path, "", "the path to watch. eg. /some/path/to/watch");

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, false);

  boost::shared_ptr<PoolDemo> demo(new PoolDemo());
  if (FLAGS_watch_path.empty()) {
    LOG(FATAL) << "specify a valid watch path";
  }

  // 启动后，先后添加expire listener，添加auth, 获取children
  demo->add_expired_listener(FLAGS_watch_path);
  demo->add_auth("test", "test");
  demo->get_children(FLAGS_watch_path, true);
  // wait forever
  select(0, 0, 0, 0, 0);
  return 0;
}



