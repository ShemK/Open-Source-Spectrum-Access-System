import io,libconf
import socket
import struct
import fcntl
import sys
import os
import json
import subprocess # going to replace os
import time
import optparse

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
            print 'Failed to read file:',file_path
            print e
            sys.exit()

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
        self.checkLogBoolean(su)
        data = self.json_encoder.encode(su)
        self.messageHandler.send(data)
        #FIXME : Need to make it more universal. Go to the folder with the script
        #command = "ssh " + su.ip + " python crts_sas/crts/radio.py >/dev/null 2>&1 &"
        #command = "ssh " + su.ip + " python crts_sas/crts/radio.py"
        #os.system(command)
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
                print "Error With Sensors "
                print e
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
        active = self.checkIfActive('sdr_phy.service',ip)
        if not active:
            time.sleep(3)
            os.system(command)
            active = self.checkIfActive('sdr_phy.service',ip)
            if not active:
                print "Failed to start sensor at " + ip

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

    def start_sas(self,ip):
        command = "ssh " + ip + " 'sudo systemctl restart aggregator.service'"
        os.system(command)


    def start_primary_users(self):
        self.primary_users = self.cornet_setup.PU.nodeList
        i = 0
        while i < len(self.primary_users):
            try:
                pu = self.primary_users[i]
                self.messageHandler.set_addr(pu.ip)
                self.checkLogBoolean(pu)
                data = self.json_encoder.encode(pu)
                self.messageHandler.send(data)
            except Exception as e:
                print "Error With Sensors: ",e
            i = i+1


    def checkIfActive(self,service,ip):
        try:
            command = "ssh " + ip + " sudo systemctl status " + service
            time.sleep(2) # wait for two seconds before getting update
            p = subprocess.check_output(command,shell=True)
            print p
            return True # need to place in if statement below
            if p.strip() == 'active':
                print service + " at " + ip + " is Active"
            else:
                return False

        except Exception as e:
            print service + " at " + ip + " did not start"
            return False

    # Dictionaries are passed by object reference in python
    def checkLogBoolean(self,input_dict):
        if('log' in input_dict and input_dict.log == True):
            if('log_path' in self.cornet_setup):
                input_dict['log_path'] = self.cornet_setup.log_path
            else:
                print 'You want to log information but no log path provided'
                sys.exit()

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
    parser = optparse.OptionParser()
    parser.add_option('-p', '--primary',action="store_true", dest="primary", default=False, help="start primary_users")
    parser.add_option('-s', '--sensor',action="store_true", dest="sensor", default=False, help="start sensors")
    parser.add_option('-c', '--secondary',action="store_true", dest="secondary", default=False, help="start secondary_users")
    parser.add_option('-d', '--stop_sensors',action="store_true", dest="stop_sensors", default=False, help="stop sensors")

    options, args = parser.parse_args()

    cornetController = CornetController()
    p = cornetController.read_config_file('cornet.cfg')
    if(options.primary):
        cornetController.start_primary_users()
    if(options.sensor):
        cornetController.start_sensors()
    if(options.secondary):
        cornetController.start_secondary_users()
    if(options.stop_sensors):
        cornetController.stop_sensors()


if __name__ == '__main__':
    main()
