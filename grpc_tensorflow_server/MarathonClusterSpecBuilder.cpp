/*
 * implementation of marathon interface for tensorflow's distributed runtime
 *
 * ct-clmsn 
 * Mar 2016
 */

#include "MarathonClusterSpecBuilder.hpp"
#include "marathonimpl.hpp"
#include <iostream>
using namespace std;
bool MarathonClusterSpecBuilder::fetch() {
   return getMarathonClusterSpec(dns_str, task_id, usr, pwd, num_tasks, cluster_spec, max_attempts);
}

