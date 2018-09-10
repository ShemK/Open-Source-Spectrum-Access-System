import socket
import json
import threading
import json


class RFNestInfo():
    def __init__(self):
        self.udp_ip = "0.0.0.0"
        self.udp_port = 9867
        self.sock = socket.socket(socket.AF_INET, # Internet
                      socket.SOCK_DGRAM) # UDP

        self.sock.bind((self.udp_ip,self.udp_port))

        self.NodeInfoMap = dict()
        self.ipMap = dict()
        self.forwarding_sock = socket.socket(socket.AF_INET, # Internet
                      socket.SOCK_DGRAM) # UDP
        self.json_encoder = json.JSONEncoder()
        t = threading.Thread(target=self.getInfoFromAdmin)
        t.daemon = True
        t.start()

    def getInfoFromAdmin(self):
        admin_sock = socket.socket(socket.AF_INET, # Internet
                      socket.SOCK_DGRAM)
        admin_sock.bind(("0.0.0.0",8911))
        print "Starting Thread"
        while True:
            try:
                data, addr = admin_sock.recvfrom(8192)
                received_dict = json.loads(data)
                nodeList = received_dict['nodeList']
                print nodeList
                i = 0
                while i < len(nodeList):
                    self.ipMap[nodeList[i]['port']] = nodeList[i]['ip']
                    print self.ipMap[nodeList[i]['port']]
                    i = i + 1
            except Exception as e:
                print "Exception Error with instruction: ",e


    def getInfo(self):
        while True:
            try:
                data, addr = self.sock.recvfrom(8192) # buffer size is 1024 bytes
                #print "received message:", data
                self.getLocation(data)
            except Exception as e:
                pass
    def getLocation(self,info):
        pos = info.find("Location Message from GUI")
        while pos != -1:
            print "-------------------------------------------------------"
            nodeInfo = info[pos:pos+456]
            info = info[pos+100:-1]
            pos = info.find("Location Message from GUI")
            self.updateIndividualNodeInfo(nodeInfo)


    def updateIndividualNodeInfo(self,info):
        location = dict()
        print info
        pos = info.find("node:")
        port = int(info[pos+5])
        pos = info.find("lat")
        location['lat'] = float(info[pos+4:pos+18])
        pos = info.find("lon")
        location['lon'] = float(info[pos+4:pos+18])
        self.NodeInfoMap[port] = location
        self.sendInfoToNode(port)

    def sendInfoToNode(self,port):
        to_send = self.json_encoder.encode(self.NodeInfoMap[port])
        if port in self.ipMap:
            self.forwarding_sock.sendto(to_send,(self.ipMap[port],7891))
            print "Sending to: ", self.ipMap[port]
            print self.NodeInfoMap[port]



def main():
    rfnest = RFNestInfo()
    rfnest.getInfo()

if __name__ == '__main__':
    main()
