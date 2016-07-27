FROM centos7
MAINTAINER ct-clmsn@gmail.com

RUN yum -y update && yum -y install gcc gcc-c++ libcurl-devel boost boost-devel boost-regex && yum clean all

COPY grpc_tensorflow_server_remote /opt/
RUN chmod ugo+x /opt/grpc_tensorflow_server_remote

CMD [ "/opt/grpc_tensorflow_server_remote" ] #, "--marathon_url=$MARATHON_URL", "--job_name=$JOBNAME$TASKID", "--task_id=$TASKID", "--num_tasks=$NUMTASKS", "--marathon_usr=$MARATHON_USR", "--marathon_pwd=$MARATHON_USRPWD" ]

