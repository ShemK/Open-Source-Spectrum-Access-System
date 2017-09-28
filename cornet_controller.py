import io,libconf
import socket
import struct
import fcntl
import sys
import os

class CornetController(object):
    #   initialize ConfigEditor
    def __init__(self):
        self.cornet_setup = dict()
        pass

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

def main():
    cornetController = CornetController()
    p = cornetController.read_config_file('cornet.cfg')
    cornetController.start_sensors()
    #cornetController.stop_sensors()
    #command = "ssh 192.168.1.19 sudo systemctl restart sdr_phy.service"
    #os.system(command)
    #cornetController.start_sensor_phy("192.168.1.19")
    #cornetController.get_sensor_status("192.168.1.19")

if __name__ == '__main__':
    main()
