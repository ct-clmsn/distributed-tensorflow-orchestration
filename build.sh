TENSORFLOW_HOME=
DTF_HOME=$TENSORFLOW_HOME/tensorflow/tensorflow/core/distributed_runtime/rpc/

# copy remote grpc server source to the correct tensorflow source directory
cp BUILD $DTF_HOME
cp ClusterSpecHandler.hpp $DTF_HOME
cp grpc_tensorflow_server_remote.cc $DTF_HOME
cp MarathonClusterSpecBuilder.hpp $DTF_HOME
cp marathonimpl.hpp $DTF_HOME
cp ClusterSpecBuilder.hpp $DTF_HOME
cp Dockerfile $DTF_HOME
cp MarathonClusterSpecBuilder.cpp $DTF_HOME
cp marathonimpl.cpp $DTF_HOME
 
echo "build tensorflow now"
