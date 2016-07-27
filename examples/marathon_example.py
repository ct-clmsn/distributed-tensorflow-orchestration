'''
   marathon_example.py
      performs a simple matrix multiply using 3 compute nodes
'''
def parseargs():
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

if __name__ == '__main__':
   from sys import argv
   import tensorflow as tf
   from dtforchestrator import *

   args = parseargs()

   with MarathonTensorFlowSession(args.url, args.taskname, args.n_tasks, args.usr, args.usrpwd, args.uri, args.cpu, args.mem) as tfdevices:

      with tf.device(tfdevices.getDeviceSpec(1)): 
         matrix1 = tf.constant([[3.],[3.]])

      with tf.device(tfdevices.getDeviceSpec(2)): 
         matrix2 = tf.constant([[3.,3.]])


      with tf.device(tfdevices.getDeviceSpec(0)):
         matrix0 = tf.constant([[3.,3.]])

      product1 = tf.matmul(matrix0, matrix1)
      product2 = tf.matmul(matrix2, matrix1)

      with tf.Session(tfdevices.localGRPC()) as sess: 
         res = sess.run(product1)
         print res
         res = sess.run(product2)
         print res

