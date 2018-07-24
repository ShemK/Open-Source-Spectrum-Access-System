import threading
import os
import sys
import socket
import json
import time
import optparse

scripts = os.path.dirname(os.path.abspath(__file__))
scripts = scripts + '/../cbsd/config_scripts'
sys.path.insert(0, scripts)
import config_editor

scripts = os.path.dirname(os.path.abspath(__file__))
scripts = scripts + '/../cbsd/python_scripts'
sys.path.insert(0, scripts)

from logger import logger



class PrimaryUser(object):

    def __init__(self,pu_type):
        self.pu_type = pu_type
        self.radio = radioThread(self)
        self.channel_list = []
        self.json_decoder = json.JSONDecoder()
        self.configEditor = config_editor.ConfigEditor()
        self.ip = "localhost"
        self.primaryLogger = None

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
        self.configEditor.add_to_output("run_time",self.run_time)
        self.configEditor.add_to_output("num_nodes",1)
        config_path = os.path.dirname(os.path.abspath(__file__))
        config_path = config_path + "/../crts/scenarios/sas_scenarios/sas_"+self.pu_type+".cfg"
        print "Written to : ", config_path
        self.configEditor.write_config_file(config_path)

    def start_radio(self):
        self.radio.start()
        self.radio.join()

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
                self.run_time = self.cornet_config['run_time']


            if 'log' in self.cornet_config and 'log_path' in self.cornet_config:
                self.primaryLogger = logger('primary',self.cornet_config['log_path'] + '/pu')

        else:
            print "No SAS Provided, assuming localhost"
            self.sas_ip = "localhost"

    def log(self,key,value):
        if self.primaryLogger != None:
            self.primaryLogger.write(key,value)

class radioThread(threading.Thread):

    def __init__(self,pu):
        threading.Thread.__init__(self)
        self.pu = pu
        self.threadStopped = False

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
        self.pu.log('Starting PU',self.pu.pu_type)
        self.pu.log('run_time',self.pu.run_time)
        self.pu.log('tx_rate',self.pu.tx_rate)
        self.pu.log('tx_freq',self.pu.tx_freq)
        os.system(x_path)
        self.threadStopped = True
    def stop_thread(self):
        pass

def run_pu(pu,options):
    if(options.tx_freq > 800 and options.tx_freq < 1000):
        print "----------------Manual frequency-----------------"
        pu.get_command()
        pu.channel_list[0] = options.tx_freq*1e6
    else:
        print "----------------Auto frequency-----------------"
        pu.get_command()
    pu.create_node_file()
    pu.start_radio()


def main():
    pu = PrimaryUser("interferer")
    parser = optparse.OptionParser()
    parser.add_option('-f', '--freq',action="store", dest="tx_freq", type = "float", help="tx frequency")
    parser.add_option('-l', '--loop',action="store_true", dest="loop", default=False, help="loop forever")
    options, args = parser.parse_args()
    if(options.loop):
        while(options.loop):
            if not pu.radio.is_alive():
                run_pu(pu,options)
    else:
        run_pu(pu,options)
        print pu.radio.is_alive()
    #time.sleep(120)

if __name__ == '__main__':
    main()
