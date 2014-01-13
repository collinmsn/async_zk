/*
 * task_queue.cc
 *
 *  Created on: Aug 12, 2013
 *      Author: Huabin Zheng
 */

#include "task_queue.h"

namespace async_zk {
  const char* ZKTaskWrapper::kTaskTypeNames[] = {
    "ADD_LISTENER_TASK",
    "INIT_TASK",
    "ADD_AUTH_TASK",
    "OTHER_TASK",
  };
}
