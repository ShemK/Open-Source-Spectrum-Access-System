import socket
import pmt

# this python script is used to forward information from internal sockets to external sockets

#MAILING SERVICE TO BE USED TO TRANSFER DATA FROM ONE PORT TO ANOTHER
# Ideally, it should connect to a source port and the receive data to be forwarded
# Currently, it is just being used to transfer data from the aggregator to visualization tool

class MailingService(object):

    def __init__(self,dest_addr,dest_port, service='udp'):

        self.dest_addr = dest_addr
        self.dest_port = dest_port
        self.service = service
        self.port = 9749 # All ports will need to be placed in a configuration file

        if self.service == 'udp':
            self.input_sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
            self.input_sock.bind(('0.0.0.0', self.port)) # NOTE: Will need to sent this to internal network only
            self.output_sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

    def forward(self):
        data, addr = self.input_sock.recvfrom(4096)
        print "Received Something",pmt.deserialize_str(data)
        self.output_sock.sendto(data, (self.dest_addr, self.dest_port))

def main():
    mailingService = MailingService('172.29.118.218',4680)
    while 1:
        mailingService.forward()

if __name__ == '__main__':
    main()
