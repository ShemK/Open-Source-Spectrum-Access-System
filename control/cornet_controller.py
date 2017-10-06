import io,libconf
import socket
import struct
import fcntl
import sys
import os
import json

class CornetController(object):
    #   initialize ConfigEditor
    def __init__(self):
        self.cornet_setup = dict()
        self.json_encoder = json.JSONEncoder()
        self.messageHandler = TransportHandler("0.0.0.0",8911)



    #   read a configuration file and return a dictionary
    #   if failure, return empty dict
    def read_config_file(self,file_path):
        try:
            with io.open(file_path) as f:
                input_config  = libconf.load(f)
                self.cornet_setup = input_config
                return input_config
        except Exception as e:
            input_config = dict()
            print 'Failed to read file ',e
            #raise
            return input_config

    def start_secondary_users(self):
        self.secondary_users = self.cornet_setup.SU.nodeList
        i = 0
        while i < len(self.secondary_users):
            try:
                self.start_su(self.secondary_users[i])
            except Exception as e:
                print "Error With Sensors: ",e
            i = i+1
        pass

    def start_su(self,su):
        self.messageHandler.set_addr(su.ip)
        data = self.json_encoder.encode(su)
        self.messageHandler.send(data)
        #FIXME : Need to make it more universal. Go to the folder with the script
        command = "ssh " + su.ip + " python crts_sas/crts/radio.py >/dev/null 2>&1 &"
        os.system(command)
        pass

    def stop_su(self,su):
        command = "ssh " + su.ip + " python crts_sas/crts/stop_cbsd.py >/dev/null 2>&1 &"
        pass

    def start_sensors(self):
        self.sensors = self.cornet_setup.SENSOR.nodeList
        i = 0
        while i < len(self.sensors):
            try:
                print self.sensors[i].ip
                self.start_sensor_phy(self.sensors[i].ip)
            except Exception as e:
                print "Error With Sensors: ",e
            i = i+1

    def stop_sensors(self):
        self.sensors = self.cornet_setup.SENSOR.nodeList
        i = 0
        while i < len(self.sensors):
            try:
                print self.sensors[i].ip
                self.stop_sensor_phy(self.sensors[i].ip)
            except Exception as e:
                print "Error With Sensors: ",e
            i = i+1

    def start_sensor_phy(self,ip):
        command = "ssh " + ip + " sudo systemctl restart sdr_phy.service"
        print command
        os.system(command)

    def stop_sensor_phy(self,ip):
        command = "ssh " + ip + " sudo systemctl stop sdr_phy.service"
        os.system(command)

    def get_sensor_status(self,ip):
        command = "ssh " + ip + " sudo systemctl status sdr_phy.service"
        os.system(command)

    def start_sas_nodes(self):
        self.sas_nodes= self.cornet_setup.SAS.nodeList
        i = 0
        while i < len(self.sas_nodes):
            try:
                self.start_sas(self.sas_nodes[i].ip)
            except Exception as e:
                print "Error With SAS Nodes: ",e
            i = i+1
        passsock.bind((UDP_IP, UDP_PORT))
    def start_sas(self,ip):
        command = "ssh " + ip + " 'sudo systemctl restart aggregator.service'"
        os.system(command)

class TransportHandler(object):

    def __init__(self,addr,port):
        self.port  = port
        self.addr = addr
        self.sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        self.my_port = 7180
        self.sock.bind(("0.0.0.0",self.my_port))

    def set_addr(self, addr):
        self.addr = addr

    def send(self,info):
        self.sock.sendto(info, (self.addr, self.port))
        print "Data Sent", info

    def recv(self):
        try:
            data = self.sock.recvfrom(1024)
            #return self.json_decoder.decode(data)
            return data
        except Exception as e:
            print "JSON Error ",e

    def analyze_instruction(self,instruction):
        pass


def main():
    cornetController = CornetController()
    p = cornetController.read_config_file('cornet.cfg')
    #cornetController.start_secondary_users()
    cornetController.start_sensors()
    #cornetController.start_sas_nodes()
    #cornetController.stop_sensors()
    #command = "ssh 192.168.1.19 sudo systemctl restart sdr_phy.service"
    #os.system(command)
    #cornetController.start_sensor_phy("192.168.1.19")
    #cornetController.get_sensor_status("192.168.1.19")

if __name__ == '__main__':
    main()
