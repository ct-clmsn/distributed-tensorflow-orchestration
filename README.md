-- Distributed Tensorflow Orchestrator --

This software package contains tools that are useful 
for running Distributed Tensorflow on a Mesos managed
cluster environment.

Currently, the software supports clusters that
interact with Mesos using the Marathon Framework.

-- WHAT ARE THESE TOOLS? --

The tools you'll interact with are:

 * 'grpc_tensorflow_server_remote'
 * 'dtforchestrator.py'
 * 'example.py'

'grpc_tensorflow_server' requires a "clusterspec" string
to be provided on the command line.

Because Mesos is asynchronous in nature, a modified version
of the 'grpc_tensorflow_server' runtime was developed
to collect the "clusterspec" string from Mesos or
Marathon using polling with exponential backoff.

'dtforchrestrator.py' is a Python module containing
code that let's users start a "Mesos/Marathon/Multiprocess"
distributed tensorflow session using Python's 'with' 
clause.

'example.py' includes Python code showing how to
use 'dtforchestrator.py'

-- INSTALLATION INSTRUCTIONS --

The 'grpc_tensorflow_server' directory houses the
following files:

 * BUILD
 * ClusterSpecHandler.hpp
 * grpc_tensorflow_server_remote.cc
 * MarathonClusterSpecBuilder.hpp
 * marathonimpl.hpp
 * ClusterSpecBuilder.hpp
 * Dockerfile  
 * MarathonClusterSpecBuilder.cpp
 * marathonimpl.cpp

Open 'build.sh' and set the variable

	TENSORFLOW_HOME

to the that that contains the tensorflow
source code. Example:

	TENSORFLOW_HOME=$HOME/Downloads/tensorflow/core/distributed_runtime/rpc/

when that edit is complete, run ./build.sh

Then go to your $TENSORFLOW_HOME directory and run the bazel
build process.

A Dockerfile is provided in the grpc_tensorflow_server directory to
ease with Marathon deployments.

This software also supports running "multiprocess" versions of 
distributed tensorflow compute sessions on a local host - a great 
way to test prior to distributed deployments.

The version of grpc_tensorflow_server_remote.cc is a couple months old
work to get this effort into tensorflow-trunk will be pursued.
