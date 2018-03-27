import threading
import os
import sys
import socket
import json
import time

scripts = os.path.dirname(os.path.abspath(__file__))
scripts = scripts + '/../cbsd/config_scripts'
sys.path.insert(0, scripts)
import config_editor

class PrimaryUser(object):

    def __init__(self,pu_type):
        self.pu_type = pu_type
        self.radio = radioThread(self)
        self.channel_list = []
        self.json_decoder = json.JSONDecoder()
        self.configEditor = config_editor.ConfigEditor()
        self.ip = "127.0.0.1"

    def initializeParameters(self, parameters):
        pass

    def create_node_file(self):
        self.node = self.configEditor.create_configuration('basic_interferer.cfg')
        self.node = self.node['node1'];
        self.node  = self.configEditor.set_node_attribute(self.node,'server_ip',self.ip)
        self.tx_rate = self.node['tx_rate'];
        self.tx_freq = self.channel_list[0];
        self.node  = self.configEditor.set_node_attribute(self.node,'tx_freq',self.tx_freq+self.tx_rate/2)
        self.configEditor.add_to_output("node1",self.node)
        self.configEditor.add_to_output("run_time",60.0)
        self.configEditor.add_to_output("num_nodes",1)
        config_path = os.path.dirname(os.path.abspath(__file__))
        config_path = config_path + "/../crts/scenarios/sas_scenarios/sas_"+self.pu_type+".cfg"
        print "Written to : ", config_path
        self.configEditor.write_config_file(config_path)

    def start_radio(self):
        self.radio.start()
        pass

    def get_command(self):
        sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM) # UDP
        sock.bind(('localhost', 7800))
        data, addr = sock.recvfrom(1024)
        json_command = self.json_decoder.decode(data)
        print json_command
        self.channel_list = json_command['channels']
        self.cornet_config = json_command['cornet_config']
        if self.cornet_config != None:
            key = 'scenario_controller'
            if key in self.cornet_config:
                self.init_sc = self.cornet_config['scenario_controller']
                self.ip = self.cornet_config['ip']

            key = 'pu_type'

            if key in self.cornet_config:
                self.pu_type = self.cornet_config['pu_type']
        else:
            print "No SAS Provided, assuming localhost"
            self.sas_ip = "127.0.0.1"

class radioThread(threading.Thread):

    def __init__(self,pu):
        threading.Thread.__init__(self)
        self.pu = pu

    def run(self):
        if self.pu.pu_type == "radar":
            print "PU is a radar"
            self.run_crts("radar")
        elif self.pu.pu_type == "interferer":
            print "PU is an interferer"
            self.run_crts("interferer")

    def run_crts(self,config_file):
        #self.create_config_file()
        x_path = os.path.dirname(os.path.abspath('__file__'))
        x_path = x_path + "/../crts/crts_controller -s sas_scenarios/sas_"+self.pu.pu_type
        print x_path
        os.system(x_path)

    def stop_thread(self):
        pass


def main():
    pu = PrimaryUser("interferer")
    pu.get_command()
    pu.create_node_file()
    pu.start_radio()
    time.sleep(120)

if __name__ == '__main__':
    main()
