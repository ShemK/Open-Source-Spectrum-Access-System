import pmt
import socket
import fcntl
import struct
import os

import config_editor

class InformationParser(object):

    def __init__(self,dest_addr,dest_port):
        self.configEditor = config_editor.ConfigEditor()

        config_path = os.path.dirname(os.path.abspath(__file__))
        config_path = config_path + '/nodeInfo.cfg'
        self.nodeConfiguration = self.configEditor.read_config_file(config_path)

        ip = self.get_ip_address('eth0')

        self.nodeID = self.get_nodeID(ip)#self.nodeConfiguration[node]['nodeID']
        self.reset_message()
        self.dest_addr = dest_addr
        self.dest_port = dest_port
        self.output_sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

    def get_nodeID(self,ip_address):
        data = ip_address.split('.')

        node = "Node"+ str(int(data[3])-10)
        return self.nodeConfiguration[node]['nodeID']

    def addStatus(self,key,value):
        if isinstance( value, ( int, long ) ):
            self.message = pmt.dict_add(self.message, pmt.string_to_symbol(key),pmt.from_long(value))
        elif isinstance( value, basestring):
            self.message = pmt.dict_add(self.message, pmt.string_to_symbol(key),pmt.string_to_symbol(value))
        elif isinstance( value, float):
            self.message = pmt.dict_add(self.message, pmt.string_to_symbol(key),pmt.from_double(value))
        elif isinstance( value, (tuple)):
            self.message = pmt.dict_add(self.message,pmt.string_to_symbol(key),pmt.init_u16vector(2,value))
        elif isinstance( value, (list)):
            self.message = pmt.dict_add(self.message,pmt.string_to_symbol(key),pmt.init_u16vector(2,value))

    def sendStatus(self):
        serialized_pmt = pmt.serialize_str(self.message)
        self.output_sock.sendto(serialized_pmt, (self.dest_addr, self.dest_port))
        self.reset_message()

    def reset_message(self):
        self.message = pmt.make_dict()
        self.message = pmt.dict_add(self.message, pmt.string_to_symbol("nodeID"),pmt.from_long(self.nodeID))

    def get_ip_address(self,ifname):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        return socket.inet_ntoa(fcntl.ioctl(
            s.fileno(),
            0x8915,  # SIOCGIFADDR
            struct.pack('256s', ifname[:15]))[20:24])

    def cleanup(self):
        self.output_sock.close()

def main():
    informationParser = InformationParser('127.0.0.1',9749)
    psd_list = []
    for i in range(0,10):
     psd_list.append(2)

    informationParser.addStatus("as",psd_list)

    informationParser.cleanup()

if __name__ == '__main__':
    main()
