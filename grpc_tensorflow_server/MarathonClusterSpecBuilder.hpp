/*
  MarathonClusterSpecBuilder.hpp
  ct-clmsn(at)gmail
 */
#ifndef __MARATHONCLUSTERSPECBLDR__
#define __MARATHONCLUSTERSPECBLDR__ 1

#include <string>
#include "ClusterSpecBuilder.hpp"

class MarathonClusterSpecBuilder : public ClusterSpecBuilder {

   public:
      MarathonClusterSpecBuilder(const std::string marathon_dns_str, const std::string task_id_str, const std::string usr_str, const std::string pwd_str, const int tasks_num=1, const int mx_attempts=10) : 
         dns_str(marathon_dns_str), 
         task_id(task_id_str), 
         usr(usr_str), 
         pwd(pwd_str), 
         num_tasks(tasks_num), 
         max_attempts(mx_attempts) {
      }

      virtual bool fetch();

   private:
      std::string dns_str, task_id, usr, pwd;
      int num_tasks, max_attempts;

};

#endif
