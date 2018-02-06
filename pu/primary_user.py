import threading
import os
import sys

class PrimaryUser(object):

    def __init__(self,pu_type):
        self.pu_type = pu_type
        self.radio = radioThread(self)
        self.channel_list = []

    def initializeParameters(self, parameters):
        pass

    def start_radio(self):
        self.radio.start()
        pass

    def get_command(self):
        sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM) # UDP
        sock.bind(('localhost', 7800))
        data, addr = sock.recvfrom(1024)
        json_command = self._json_decoder.decode(data)
        print json_command
        self.channel_list = json_command['channels']
        self.cornet_config = json_command['cornet_config']
        if self.cornet_config != None:
            self.init_sc = self.cornet_config['scenario_controller']
        else:
            print "No SAS Provided, assuming localhost"
            self.sas_ip = "127.0.0.1"

class radioThread(threading.Thread):

    def __init__(self,pu):
        threading.Thread.__init__(self)
        self.pu = pu

    def run(self):
        if self.pu.pu_type == "RADAR":
            print "PU is a radar"
            self.run_crts("radar")
        elif self.pu.pu_type == "INTERFERER":
            print "PU is an interferer"
            self.run_crts("interferer")

    def run_crts(self,config_file):
        #self.create_config_file()
        x_path = os.path.dirname(os.path.abspath('__file__'))
        x_path = x_path + "/../crts/crts_controller -s sas_scenarios/"
        print x_path
        #os.system(x_path)

    def stop_thread(self):
        pass


def main():
    pu = PrimaryUser("INTERFERER")
    pu.start_radio()

if __name__ == '__main__':
    main()
