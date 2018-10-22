import pmt
import socket
import fcntl
import struct
import os

class InformationParser(object):

    def __init__(self,dest_addr,dest_port):
        self.nodeID = 0#self.nodeConfiguration[node]['nodeID']
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
            #print value
            self.message = pmt.dict_add(self.message, pmt.string_to_symbol(key),pmt.from_double(value))
            #check  = pmt.dict_ref(self.message, pmt.string_to_symbol(key), pmt.PMT_NIL)
            #print check
            #check = pmt.to_double(check)
            #print check
        elif isinstance( value, (tuple)) or isinstance( value, (list)):
            if isinstance( value[0], ( int, long ) ):
                self.message = pmt.dict_add(self.message,pmt.string_to_symbol(key),pmt.init_u16vector(len(value),value))
            elif isinstance( value[0], ( int, float ) ):
                self.message = pmt.dict_add(self.message,pmt.string_to_symbol(key),pmt.init_f64vector(len(value),value))

    def sendStatus(self):
        serialized_pmt = pmt.serialize_str(self.message)
        #print self.message
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

##USAGE Example
def main():
    informationParser = InformationParser('127.0.0.1',9749)
    psd_list = []
    for i in range(0,10):
     psd_list.append(2)

    informationParser.addStatus("as",psd_list)

    informationParser.cleanup()

if __name__ == '__main__':
    main()
