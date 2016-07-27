/*
 * implementation of marathon interface for tensorflow's distributed runtime
 *
 * ct-clmsn 
 * Mar 2016
*/

#ifndef __MARATHON_IMPL__
#define __MARATHON_IMPL__ 1

#include <string>

bool getMarathonClusterSpec(const std::string marathon_dns_str, const std::string task_id, const std::string usr, const std::string pwd, const int num_tasks, std::string& cluster_spec, const int MAXATTEMPTS);


#endif


