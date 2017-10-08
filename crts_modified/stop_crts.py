#!/usr/bin/python
import commands
import time
import socket
#find the pids of any process that contains CRTS.
pids = commands.getoutput("ps -ef | grep crts_controller | grep -v grep | awk '{print $2}'").split()
for pid in pids:
    print pid
    cmd = 'kill -2 '
    cmd += pid
    #print cmd
    #kill the process
    commands.getoutput(cmd)
    time.sleep(1)
    cmd = 'kill -2 '
    cmd += pid
    commands.getoutput(cmd)
    #cmd = 'kill -9 '
    #cmd += pid
    #commands.getoutput(cmd)
#tear down tunCRTS if it's still up
#commands.getoutput('sudo ip tuntap del dev tunCRTS mode tun')
