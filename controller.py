import random
import json
import socket
import time
import signal,os
import threading

class Controller(object):

    channels = []
    channel_num = 5
    stop = False
    def __init__(self,random_distribution,period,port):
        self.random_distribution = random_distribution
        self.radio_port = port
        self.period = period
        self.messageHandler = TransportHandler('0.0.0.0',port)
        self.json_encoder = json.JSONEncoder()
        self.json_decoder = json.JSONDecoder()
        self.instructionListener = InstructionListener(self,self.messageHandler)
        self.instructionListener.start()
        self.grouped = None

    def generate_random_channels(self):
        random.seed()
        self.channels = []
        while len(self.channels) < self.channel_num:
            newChannel = random.randint(355,370) * 10e6
            if newChannel not in self.channels:
                self.channels.append(newChannel)

    def package_channel_list(self):
        self.generate_random_channels()
        self.channel_info = {'channels':self.channels,'grouped':self.grouped}

    def send_channel_command(self):
        self.package_channel_list()
        data = self.json_encoder.encode(self.channel_info)
        self.messageHandler.send(data)

    def start_control(self):
        while not self.stop:
            if self.random_distribution == "uniform":
                self.send_channel_command()
                time.sleep(self.period)
            elif self.random_distribution == "poisson":
                print "Poisson"
                self.send_channel_command()
                time.sleep(self.period)

    def signal_handler(self,signum,frame):
        print 'Signal handler called with signal', signum
        self.stop = True

    def analyze_instruction(self,instruction):
        self.random_distribution = instruction['random_distribution']
        self.period = instruction['max_time']
        self.grouped = instruction['grouped']
        pass

class TransportHandler(object):

    def __init__(self,addr,port):
        self.port  = port
        self.addr = addr
        self.sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        self.my_port = 8911
        self.sock.bind(("0.0.0.0",self.my_port))
        #self.sock.setblocking(0)
        self.sock.settimeout(1.0)

    def send(self,info):
        self.sock.sendto(info, (self.addr, self.port))
        print "Data Sent", info

    def recv(self):
        try:
    #        print "Receiving"
            data,addr = self.sock.recvfrom(1024)
            return data
        except Exception as e:
        #    print "JSON Error ",e
            pass


class InstructionListener(threading.Thread):
    def __init__(self,controller,messageHandler):
        threading.Thread.__init__(self)
        self.controller = controller
        self.messageHandler = messageHandler
        self.stop  = False

    def run(self):
        while self.controller.stop == False:
            instruction = self.messageHandler.recv()
            if instruction != None:
                print "Instruction: ", instruction
                json_inst = self.controller.json_decoder.decode(instruction)
                self.controller.analyze_instruction(json_inst)

        print "Done"


def main():
    newController = Controller('uniform',5,7800)
    signal.signal(signal.SIGINT,newController.signal_handler)
    newController.start_control()

if __name__ == '__main__':
    main()
