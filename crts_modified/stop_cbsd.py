#!/usr/bin/python
import commands
import time
import socket
#find the pids of any process that contains CRTS.
pids = commands.getoutput("ps -ef | grep radio | grep -v grep | awk '{print $2}'").split()
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

    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    addr = '127.0.0.1'
    port = 7816
    info = "stop"
    sock.sendto(info, (addr, port))
    time.sleep(1)
    cmd = 'kill -9 '
    cmd += pid
    commands.getoutput(cmd)

    #cmd = 'kill -9 '
    #cmd += pid
    #commands.getoutput(cmd)
#tear down tunCRTS if it's still up
#commands.getoutput('sudo ip tuntap del dev tunCRTS mode tun')
