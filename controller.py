import random
import json
import socket
import time
import signal,os

class Controller(object):

    channels = []
    channel_num = 5
    stop = False
    def __init__(self,random_distribution,period,port):
        self.random_distribution = random_distribution
        self.radio_port = port
        self.period = period
        self.messageHandler = TransportHandler('localhost',port)
        self.json_encoder = json.JSONEncoder()

    def generate_random_channels(self):
        random.seed()
        self.channels = []
        while len(self.channels) < self.channel_num:
            newChannel = random.randint(355,370) * 10e6
            if newChannel not in self.channels:
                self.channels.append(newChannel)

    def package_channel_list(self):
        self.generate_random_channels()
        self.channel_info = {'channels':self.channels}

    def send_channel_command(self):
        self.package_channel_list()
        data = self.json_encoder.encode(self.channel_info)
        self.messageHandler.send(data)

    def start_control(self):
        while not self.stop:
            if self.random_distribution == "uniform":
                self.send_channel_command()
                time.sleep(self.period)

    def signal_handler(self,signum,frame):
        print 'Signal handler called with signal', signum
        self.stop = True


class TransportHandler(object):

    def __init__(self,addr,port):
        self.port  = port
        self.addr = addr
        self.sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

    def send(self,info):
        self.sock.sendto(info, (self.addr, self.port))
        print "Data Sent", info


def main():
    newController = Controller('uniform',2,7800)
    signal.signal(signal.SIGINT,newController.signal_handler)
    newController.start_control()

if __name__ == '__main__':
    main()
