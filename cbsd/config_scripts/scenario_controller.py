import os
import time
import socket
import struct

# This class is going to be used to change the different parameters of the scenario_controller

class ScenarioController(object):


    def __init__(self,cbsd=None):
        self.ip = '127.0.0.1'
        self.port = 5681
        self.mySocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.myParams = dict()
        self.myParams['currentNodeId'] = 1
        self.myParams['mod'] = 1
        self.myParams['crc'] = 1
        self.myParams['ifec'] = 2
        self.myParams['ofec'] = 3
        self.myParams['freq'] = 860e6
        self.myParams['bandwidth'] = 2e6
        self.myParams['gain'] = 20

    def send(self):
        packed = struct.pack('<iiiiiiddd', 1, int(self.myParams['currentNodeId']), int(self.myParams['mod']), int(self.myParams['crc']), int(self.myParams['ifec']), int(self.myParams['ofec']), float(self.myParams['freq']), float(self.myParams['bandwidth']),float(self.myParams['gain']))
        self.mySocket.sendto(packed, (self.ip,self.port))

    def set_parameter(self,node,key,value):
        self.myParams['currentNodeId'] = node
        self.myParams[key] = value
        self.send()

def main():
    sc = ScenarioController()
    sc.set_parameter(1,'freq',3652.5e6)
    sc.set_parameter(2,'freq',3656.5e6)

if __name__ == '__main__':
    main()
