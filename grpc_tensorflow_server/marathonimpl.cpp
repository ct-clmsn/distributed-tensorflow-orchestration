/*
 * implementation of marathon interface for tensorflow's distributed runtime
 *
 * ct-clmsn 
 * Mar 2016
 */

#include "marathonimpl.hpp"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <unistd.h>
#include <curl/curl.h>

#ifndef CXX15 
   #include <boost/lexical_cast.hpp>
   #include <boost/regex.hpp>
   using namespace boost;
#else
   #include <regex>
#endif

using namespace std;

/*
 * curl callback function that copies response from marathon server into the "data" buffer
 */
static int writer(char* data, size_t size, size_t nmemb, string *writerData) {
   if (writerData == NULL) {
      return 0;
   }

   writerData->append(data, size*nmemb);

   return size*nmemb;
}

/*
 * bool return code, did this succeed (true) or fail (false)
 *    marathon_curl_cmd( 
 * 	const string marathon_dns_url_str = base url to the marathon server,
 * 	string& marathon_task_json = response json string from marathon server,
 * 	const string usr = marathon user name
 * 	const string pwd = marathon user password
 */
bool marathon_curl_cmd(const string marathon_dns_url_str, string& marathon_task_json, const string usr, const string pwd) {
   int retcode = 0;
   CURL* curl;
   CURLcode res;
   char errorBuffer[CURL_ERROR_SIZE];

   curl_global_init(CURL_GLOBAL_DEFAULT);
   curl = curl_easy_init();

   if(curl) {
      res = curl_easy_setopt(curl, CURLOPT_URL, marathon_dns_url_str.c_str()); 
      if(res != CURLE_OK) {
         cout << "failed to set url" << endl;
         retcode = -1;
      }

      res = curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
      if(res != CURLE_OK) {
         cout << "failed to set http header" << endl;
         retcode = -1;
      }

      res = curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
      if(res != CURLE_OK) {
         cout << "failed to set http get" << endl;
         retcode = -1;
      }

      res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
      if(res != CURLE_OK) {
         cout << "failed to set error buffer" << endl;
         retcode = -1;
      }

      res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer); 
      if(res != CURLE_OK) {
         cout << "failed to set writer" << endl;
         retcode = -1;
      }

      res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &marathon_task_json);
      if(res != CURLE_OK) {
         cout << "failed to set write data" << endl;
         retcode = -1;
      }

      const string usrpwd = usr + ":" + pwd;
      res = curl_easy_setopt(curl, CURLOPT_USERPWD, "marathon:st0rmtr00per");
      if(res != CURLE_OK) {
         cout << "failed to set usrname" << endl;
         retcode = -1;
      }

      errorBuffer[0] = 0;
      res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
         cout << "curl_easy_perform failed" << endl;
         retcode = -1;
      }

   }

   curl_easy_cleanup(curl);
   return retcode != -1 ? true : false;
}

/*
 * bool success (true) fails (false)
 *    taskIdIsAvailable
 *       const string& marathon_results = data from the marathon server
 *       const string task_id = the application's task id/name
 */
bool taskIdIsAvailable(const string& marathon_results, const string task_id) {
   regex tasks_regex("\"id\":\"" + task_id + "\"");
   smatch tasks_match;
   regex_search(marathon_results, tasks_match, tasks_regex);
   return !tasks_match.empty();
}

/*
 * bool success(true) failure (false)
 *    getHostsPorts(
 *       const string& marathon_dns_str = the marathon server's url
 *       const string task_id = the task id/name 
 *       const string usr = marathon user name 
 *       const string pwd = marathon user's password
 *       string& cluster_spec = storage for the result
 */
bool getHostsPorts(const string& marathon_dns_str, const string task_id, const string usr, const string pwd, string& cluster_spec) {
   const string marathon_dns_url_str = marathon_dns_str + "/v2/apps/" + task_id;

   string marathon_results;
   marathon_curl_cmd(marathon_dns_url_str, marathon_results, usr, pwd);

   regex hostport_regex("\"host\":\"[a-zA-Z0-9.]+\",\"ports\":\\[([0-9]+)(,)?\\]");
   smatch hostport_match;
   const bool found_hostport = regex_search(marathon_results, hostport_match, hostport_regex);
   if(!found_hostport) { cout << "host+port not found" << endl; return false; }
   if(hostport_match.empty()) { cout << "host+port not found" << endl; return false; }
   const string hostportstr = hostport_match.str();

   regex host_regex("\"host\":\"[a-zA-Z0-9.]+\""); 
   smatch host_match;
   const bool found_host = regex_search(hostportstr, host_match, host_regex);
   if(!found_host) { cout << "hosts not found" << endl; return false; }
   if(host_match.empty()) { cout << "hosts not found" << endl; return false; }
   const string jsonhoststr = host_match.str();

   regex hostname_regex(":\"([a-zA-Z0-9.]+)+\""); 
   smatch hostname_match;
   const bool found_hostname = regex_search(jsonhoststr, hostname_match, hostname_regex);
   if(!found_hostname) { cout << "hostname not found" << endl; return false; }
   if(hostname_match.empty()) { cout << "hostname not found" << endl; return false; }
   const string hostname_str = hostname_match.str();
   const string hostname = hostname_str.substr(2,hostname_str.length()-3);

   regex port_regex("\"ports\":\\[([0-9]+)(,)?\\]");
   smatch port_match;
   const bool found_ports = regex_search(hostportstr, port_match, port_regex);
   if(!found_ports) { cout << "ports not found" << endl; return false; }
   if(port_match.empty() || port_match.size() < 2) { cout << "ports not found" << endl; return false; }
   const string port_str = port_match[1].str();
   
   regex portstr_regex("([0-9]+)+"); 
   smatch portstr_match; // final pair
   const bool found_portstr = regex_search(port_str, portstr_match, portstr_regex);
   if(!found_portstr) { cout << "port not found" << endl; return false; }
   if(portstr_match.empty()) { cout << "port not found" << endl; return false; }
   const string portstr = portstr_match.str();
   
   // append result for cluster_spec string
   cluster_spec = hostname + ":" + portstr;   
   return true;
}

bool fetchClusterSpec(const string marathon_dns_str, const string task_id, const string usr, const string pwd, string& cluster_spec) {
   const string marathon_dns_url_str = marathon_dns_str + "/v2/apps/" + task_id;
   string marathon_results;
   const bool curl_success = marathon_curl_cmd(marathon_dns_url_str, marathon_results, usr, pwd);
   const bool fetchedspec = (curl_success == true && marathon_results.size() > 0 && taskIdIsAvailable(marathon_results, task_id) && getHostsPorts(marathon_dns_str, task_id, usr, pwd, cluster_spec));
   return fetchedspec;
}

bool getMarathonClusterSpec(const string marathon_dns_str, const string task_id, const string usr, const string pwd, const int num_tasks, string& cluster_spec, const int MAXATTEMPTS) {
   cluster_spec = task_id + "|";

   int tasks_attempts [num_tasks];

   for(int i = 0; i < num_tasks; i++) {
      bool addedspec = true;

#ifndef CXX15 
      const string task_count = boost::lexical_cast<string>(i+1);
#else
      const string task_count = to_string(i+1);
#endif

      tasks_attempts[i] = 0;
      // busy loop that keeps polling marathon site for task info
      while((addedspec == true) && (tasks_attempts[i] < MAXATTEMPTS)) {
         string node_info_str;
         const string task_id_str = "/" + task_id + task_count;
      
         // exponential backoff
         this_thread::sleep_for(chrono::seconds((int)pow(2, tasks_attempts[i])));
         const bool successful_fetch = fetchClusterSpec(marathon_dns_str, task_id_str, usr, pwd, node_info_str);
      
         if(successful_fetch == false && node_info_str.size() < 1) {
            tasks_attempts[i] += 1;
            continue;
         }
         else {
            cluster_spec += node_info_str;
            if (i < num_tasks-1) {
               cluster_spec+=";";
            }
            addedspec = false;
         }
      }
   }
      
   int tasks_attempts_check = 0;
   for(int i = 0; i < num_tasks; i++) {
      tasks_attempts_check += (tasks_attempts[i] < MAXATTEMPTS) ? 0 : 1;
   }
   
   return (tasks_attempts_check < num_tasks) ? true : false;
}
   
/*
 
#define MAXATTEMPTS 10

void getClusterSpec(const string marathon_dns_str, const string task_id, const string usr, const string pwd, const int num_tasks, string& cluster_spec) {
   cluster_spec = task_id + "|";

   for(int i = 0; i < num_tasks; i++) {
      bool addedspec = true;

#ifndef CXX15 
      const string task_count = boost::lexical_cast<string>(i+1);
#else
      const string task_count = to_string(i+1);
#endif

      int attempts = 0;
      // busy loop that keeps polling marathon site for task info
      while((addedspec == true) && (attempts < MAXATTEMPTS)) {
         string node_info_str;
         const string task_id_str = "/" + task_id + task_count;

         // exponential backoff
         this_thread::sleep_for(chrono::seconds((int)pow(2, attempts)));
         const bool successful_fetch = fetchClusterSpec(marathon_dns_str, task_id_str, usr, pwd, node_info_str);

         if(successful_fetch == false && node_info_str.size() < 1) {
            attempts += 1;
            continue;
         }
         else {
            cluster_spec += node_info_str;
            if (i < num_tasks-1) {
               cluster_spec+=";";
            }
            addedspec = false;
         }
      }
   }
}
*/

/*
 * test driver
 *
int main(int argc, char** argv) {
   const char* marathon_uri = argv[1];
   const string tsk_id(argv[2]);
   const string usr(argv[3]);
   const string pwd(argv[4]);

   const int ntasks = argc > 5 ? stoi(argv[5]) : 1;
   string cluster_spec;
   getClusterSpec(marathon_uri, tsk_id, usr, pwd, ntasks, cluster_spec);
   cout << cluster_spec << endl;
}
*/

