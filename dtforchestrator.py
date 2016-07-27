'''
   dtforchestrator.py

   glue code to get mesos and tensorflow cooperating

   ct-clmsn
   mar2016
'''

import logging, string, uuid, time, requests, argparse, json, re 
from socket import gethostname
from string import Template
from time import sleep
from subprocess import Popen

from multiprocessing import Process, Queue, JoinableQueue

from requests import Request, Session
from requests.auth import HTTPBasicAuth

logging.basicConfig(level=logging.INFO)

marathon_template=Template('{\n\
    "id": "$a$c",\n\
    "cmd": "env && chmod ugo+x grpc_tensorflow_server_remote && ./grpc_tensorflow_server_remote --url=$b --localhost=$j --job_name=$a$c --task_id=$c --num_tasks=$d --usr=$h --usrpwd=$i --mx_attempts=$k",\n\
    "cpus": $e,\n\
    "mem": $f,\n\
    "instances": 1,\n\
    "constraints" : [["os", "LIKE", "centos7"]],\n\
    "uris" : [\n\
       "$g/grpc_tensorflow_server"\n\
    ],\n\
    "ports": [\n\
       0\n\
    ]\n\
}')

marathon_docker_template=Template('{\n\
   "id" : "$a$c",\n\
   "constraints": [["os", "LIKE", "centos7"]],\n\
   "container" : {\n\
      "docker" : {\n\
         "network" : "HOST",\n\
         "image" : "$g",\n\
         "forcePullImage" : true\n\
      },\n\
      "type" : "DOCKER"\n\
   },\n\
   "cmd": "env && /opt/grpc_tensorflow_server_remote --localhost=$j --url=$b --job_name=$a --num_tasks=$d --task_id=$c --usr=$h --usrpwd=$i --mx_attempts=$k",\n\
   "cpus" : $e,\n\
   "mem" : $f,\n\
   "instances" : 1\n\
}')


def isRunning(tsknom_str, tsk_idx, resptxt): 
   # scan for task - if this returns None then it's failed 
   m = re.search('(\"id\"\:\"/%s%d\")' % (tsknom_str, tsk_idx), resptxt) #resp.text)

   if m is None:
      return (False, None)

   return (True, resptxt)

def getHostPort(respstr):
   jsonobj = json.loads(respstr)

   if jsonobj.has_key('app') and jsonobj['app'].has_key('tasks') and len(jsonobj['app']['tasks']) > 0 and jsonobj['app']['tasks'][0].has_key('ports') and len(jsonobj['app']['tasks'][0]['ports']) > 0:
      hostm = jsonobj['app']['tasks'][0]['host']
      portm = jsonobj['app']['tasks'][0]['ports'][0]
      return (hostm, portm)

   return (None, None)
   

def expBackoff(attempts):
   sleep((2 ** attempts))

MAXATTEMPTS=10

def post_marathon_tasks(marathon_url_str, tsknom_str, cpus_float, mem_float, tsk_idx, ntasks_int, uri_str, marathon_usr, marathon_usrpwd, localhost_str, mxattempts=10, docker=False):
   if docker == False:
      args = dict(a=tsknom_str, b=marathon_url_str, c=str(tsk_idx), d=str(ntasks_int), e=str(cpus_float), f=str(mem_float), g=uri_str, h=marathon_usr, i=marathon_usrpwd, j=localhost_str, k=str(mxattempts))
      curlreq_str = marathon_template.substitute(args)
   else:
      args = dict(a=tsknom_str, b=marathon_url_str, c=str(tsk_idx), d=str(ntasks_int), e=str(cpus_float), f=str(mem_float), g=uri_str, h=marathon_usr, i=marathon_usrpwd, j=localhost_str, k=str(mxattempts))
      curlreq_str = marathon_docker_template.substitute(args)

   marathon_url_str = marathon_url_str.replace('marathon://', 'http://')
   marathon_url_post_str = '%s/v2/apps' % (marathon_url_str,)

   #print curlreq_str
   #print marathon_url_post_str 
   s = Session()
   resp = s.post(url=marathon_url_post_str, data=curlreq_str, auth=HTTPBasicAuth(marathon_usr, marathon_usrpwd), headers={'Content-Type' : 'application/json' })

   # check to make sure the tasks are up
   attempts = 0
   while attempts < MAXATTEMPTS:
      resp = s.get(url='%s/v2/apps/%s%d' % (marathon_url_str, tsknom_str, tsk_idx), auth=HTTPBasicAuth(marathon_usr, marathon_usrpwd)) #, headers={'Content-Type' : 'application/json' })

      if resp.status_code == 200:
         foundtsk, hostportstr = isRunning(tsknom_str, tsk_idx, resp.text) #s, marathon_url_str, tsknom_str, tsk_idx, marathon_usr, marathon_usrpwd)

         if foundtsk: 
            host, port = getHostPort(resp.text)
            if host != None and port != None:
               return TensorFlowGRPCDevice(tsknom_str, tsk_idx, host, port)

      attempts += 1
      expBackoff(attempts)

   # put Nothing in the queue - we failed big time
   return (None, (None, None))

class Consumer(Process):
   def __init__(self, tsk_q, res_q):
      Process.__init__(self)
      self.tq = tsk_q
      self.rq = res_q

   def run(self):
      pname = self.name
      while True:
         ntsk = self.tq.get()
         if ntsk is None or ntsk == None:
            self.tq.task_done()
            break
         res = ntsk()
         self.tq.task_done()
         self.rq.put(res)
      return

class Task(object):
   def __init__(self, func, kwargs):
      self.func = func
      self.args = kwargs
   def __call__(self):
      res = self.func(*self.args)
      return res

class TensorFlowGRPCDevice(object):
   def __init__(self, job_str, tskid_int, host_str, port_str):
      self.clusterspec = '/job:%s/task:%d' % (job_str, tskid_int)
      self.hostport = '%s:%s' % (host_str, port_str)

def launch_mesos_tf(marathon_url_str, tsknom_str, cpu_float, mem_float, ntasks_int, uri_str, marathon_usr, marathon_usrpwd, localhost_str, mxattempts=10):
   toret_nodes = dict()

   docker = False
   if uri_str.find('docker') > -1:
      uri_str = uri_str.replace('docker://', '')
      docker = True
 
   uri_str = uri_str.rstrip('/')
   marathon_url_str = marathon_url_str.rstrip('/') 

   counter = 0
   tq = JoinableQueue()
   q = Queue()
   plist = list()

   consumers = [ Consumer(tq, q) for i in xrange(ntasks_int) ]
   for c in consumers:
      c.start()

   for i in xrange(ntasks_int):
      tq.put(Task(post_marathon_tasks, (marathon_url_str, tsknom_str, cpu_float, mem_float, i+1, ntasks_int, uri_str, marathon_usr, marathon_usrpwd, localhost_str, mxattempts, docker)))

   for i in xrange(ntasks_int):
      tq.put(None)

   tq.join()

   for i in xrange(1, ntasks_int+1):
      toret_nodes[i] = q.get()

   return toret_nodes

def teardown_marathon_tf_session(marathon_url_str, marathon_usr, marathon_usrpwd, tsknom_str, tsk_idx):
   s = Session()
   marathon_url_str = marathon_url_str.replace("marathon://", "http://")
   resp = s.delete(url='%s/v2/apps/%s%d' % (marathon_url_str, tsknom_str, tsk_idx), auth=HTTPBasicAuth(marathon_usr, marathon_usrpwd))
   #print resp

def distTensorflowInit():
   parser = argparse.ArgumentParser(description='Marathon for TensorFlow.')
   parser.add_argument('--n_tasks', default=1, help='an integer for the accumulator')
   parser.add_argument('--cpu', default=100.0, help='an integer for the accumulator')
   parser.add_argument('--mem', default=100.0, help='an integer for the accumulator')
   parser.add_argument('--taskname', default=uuid.uuid1(), help='name for the task')
   parser.add_argument('--url', help='DNS addr to marathon')
   parser.add_argument('--usr', help='marathon username')
   parser.add_argument('--usrpwd', help='marathon password')
   parser.add_argument('--uri', help='curl-friendly URI to the tensorflow client executable (url?, hdfs?, docker?)')
   args = parser.parse_args()
   return args

def launch_local_tf(job_name, tskid_int, nodes_dict):
   nodes_list = [ nodes_dict[k].hostport for k in nodes_dict.keys() ]
   clusterspec_str = "%s|%s" % (job_name, (("%s;" * len(nodes_list)).rstrip(';') % tuple(nodes_list)))
   cmd = "grpc_tensorflow_server/grpc_tensorflow_server --cluster_spec=%s --job_name=%s --task_id=%d" % (clusterspec_str, job_name, tskid_int)
   #print 'cmd', cmd
   p = Popen(cmd.split())
   return p

class MultiprocessTensorFlowSession(object):
   def __init__(self, job_name, tasks_int, port_int=2222):
      self.localhost = gethostname()
      self.port = int(port_int)
      self.taskcount = int(tasks_int)
      self.job_name = job_name
      self.nodes = dict()

   def __enter__(self):
      localhost_str = "%s:%d" % (self.localhost, self.port)
      self.nodes = dict([ (i, TensorFlowGRPCDevice(self.job_name, i, self.localhost, self.port+i)) for i in xrange(1, self.taskcount) ])
      self.nodes[0] = TensorFlowGRPCDevice(self.job_name, 0, self.localhost, self.port)

      self.localprocs = [  launch_local_tf(self.job_name, i, self.nodes) for i in xrange(1, self.taskcount) ]
      self.localproc = launch_local_tf(self.job_name, 0, self.nodes)
      return self

   def __exit__(self, type, value, traceback):
      [ p.terminate() for p in self.localprocs ]
      self.localproc.terminate()
      del self.nodes
      self.nodes = dict()

   def localGRPC(self):
      return "grpc://%s" % (self.nodes[0].hostport,)

   def getDeviceSpec(self, dev):
      if self.nodes.has_key(dev):
         return self.nodes[dev].clusterspec

   def getHostPort(self, i):
      if self.nodes.has_key(i):
         return self.nodes[i].hostport

   def getPort(self, i):
      return self.nodes[i].hostport.split(':')[1]

   def getHost(self, i):
      return self.nodes[i].hostport.split(':')[0]

   def __iter__(self):
      for k in self.nodes.keys():
         yield k
   
class MarathonTensorFlowSession(object):
   def __init__(self, marathon_url, job_name, num_tasks, marathon_usr, marathon_pwd, uri, cpu=1.0, mem=1024.0, port=2222):
      self.marathon_url = marathon_url
      self.job_name = job_name
      self.num_tasks = int(num_tasks)
      self.usr = marathon_usr
      self.pwd = marathon_pwd
      self.uri = uri
      self.cpu = cpu
      self.mem = mem
      self.port = port
      self.localhost = gethostname()
      self.nodes = dict()

   def __enter__(self):
      localhost_str = "%s:%d" % (self.localhost, self.port)
      self.nodes = launch_mesos_tf(self.marathon_url, self.job_name, self.cpu, self.mem, self.num_tasks, self.uri, self.usr, self.pwd, localhost_str)
      self.nodes[0] = TensorFlowGRPCDevice(self.job_name, 0, self.localhost, self.port)
      self.localproc = launch_local_tf(self.job_name, 0, self.nodes)
      return self

   def __exit__(self, type, value, traceback):
      for i in xrange(self.num_tasks):
         teardown_marathon_tf_session(self.marathon_url, self.usr, self.pwd, self.job_name, i+1)
      self.localproc.terminate()
      del self.nodes
      self.nodes = dict()

   def localGRPC(self):
      return "grpc://%s" % (self.nodes[0].hostport,)

   def getDeviceSpec(self, dev):
      return self.nodes[dev].clusterspec

   def getHostPort(self, i):
      return self.nodes[i].hostport

   def getPort(self, i):
      return self.nodes[i].hostport.split(':')[1]

   def getHost(self, i):
      return self.nodes[i].hostport.split(':')[0]

   def __iter__(self):
      for k in self.nodes.keys():
         yield k
