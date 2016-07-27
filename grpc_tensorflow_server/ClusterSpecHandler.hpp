/*
  ClusterSpectHandler.hpp
  ct-clmsn(at)gmail
 */
#ifndef __CLUSTERSPECHDLR__
#define __CLUSTERSPECHDLR__ 1

#include <string>
#include <iostream>

using namespace std;

#ifndef CXX15 
   #include <boost/lexical_cast.hpp>
   #include <boost/regex.hpp>
   using namespace boost;
#else
   #include <regex>
#endif

#include "ClusterSpecBuilder.hpp"
#include "MarathonClusterSpecBuilder.hpp"

typedef struct _clusterspechdlr {

//  string str_args [] = { options->job_name, options->task_index, num_tasks_str, usr, usrpwd, mx_attempts_str };
//
   bool operator()(const string uri, const string variables[], const int variables_count, string& clusterspec) 
   {
      regex cluster_uri_prefix_re("[a-zA-Z]+(:/{2})");
      smatch cluster_uri_prefix_match;
      const bool found_cluster_uri_prefix = regex_search(uri, cluster_uri_prefix_match, cluster_uri_prefix_re);

      bool success = false;

      if(!found_cluster_uri_prefix) { cout << "cluster prefix not found" << endl; success = false; return success; }
      if(cluster_uri_prefix_match.empty()) { cout << "cluser prefix not found" << endl; success = false; return success; }
      const string cluster_uri_str = cluster_uri_prefix_match.str();

      if (cluster_uri_str == MARATHON) {
         const string clean_uri_str = "http://" + uri.substr(MARATHON.size(), uri.size()-MARATHON.size());
         success = runMarathonClusterSpecBuilder(clean_uri_str, variables, variables_count, clusterspec);
      }
      else {
         success = false;
         clusterspec = ""; 
      }

      return success; 
   }

   private:
      enum ArgField { TASK_NAME = 0, TASK_ID, TASKS_NUM, USR_STR, USR_PWD, MX_ATTEMPTS };

      bool testMarathonParams(const string variables [], const int variables_count) {
          if(variables_count < 6) { return false; }

          regex tasks_num_re("[0-9]+");
          smatch tasks_num_match;
          const bool found_tasks_num = regex_search(variables[TASKS_NUM], tasks_num_match, tasks_num_re);
          const bool found_mx_attempts = regex_search(variables[MX_ATTEMPTS], tasks_num_match, tasks_num_re);
          return (found_tasks_num == true && found_mx_attempts == true);
      }

      bool runMarathonClusterSpecBuilder(const string uri, const string variables[], const int variables_count, string& clusterspec)
      {
         if(testMarathonParams(variables, variables_count)) {
            MarathonClusterSpecBuilder mcsb(uri, variables[TASK_NAME], variables[USR_STR], variables[USR_PWD], stoi(variables[TASKS_NUM]), stoi(variables[MX_ATTEMPTS])); 
            const bool success = mcsb.fetch();
            clusterspec = mcsb.getClusterSpec();
            return success;
         }

         return false;
      }

      const string MARATHON = "marathon://";

} ClusterSpecHandler;

#endif
