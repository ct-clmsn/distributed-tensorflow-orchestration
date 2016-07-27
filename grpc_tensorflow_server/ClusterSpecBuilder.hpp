/*
  ClusterSpecBuilder.hpp
  ct-clmsn(at)gmail
 */
#ifndef __CLUSTERSPECBLDR__
#define __CLUSTERSPECBLDR__ 1

#include <string>

class ClusterSpecBuilder {

   public:
      virtual bool fetch() = 0;
      std::string getClusterSpec() { return cluster_spec; }

   protected:
      std::string cluster_spec;
};

#endif
