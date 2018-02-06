import io,libconf
import socket
import struct
import fcntl
import os
'''
    Object used to read the configuration files to create dictionaries
    Object can also write a new configuration file
'''
class ConfigEditor(object):
    #   initialize ConfigEditor
    def __init__(self):
        #self.read_config_file('basic.cfg')
        self.output = dict()
        pass

    #   read a configuration file and return a dictionary
    #   if failure, return empty dict
    def read_config_file(self,file_path):
        try:
            with io.open(file_path) as f:
                input_config  = libconf.load(f)
                return input_config
        except Exception as e:
            input_config = dict()
            print 'Failed to read file ',e
            #raise
            return input_config

    #       write current stored dict to file
    def write_config_file(self,file_path):
        try:
            f = open(file_path,'w')
            libconf.dump(self.output,f)
            self.output = dict() # erase output dictionary
        except Exception as e:
            print 'Failed to write to file ',e
            raise

    #  add dict information to the potential output dictionary
    def add_to_output(self,key,new_config):
        self.output[key] = new_config
        #self.write_config_file(self.output,'hello.cfg')

    #   read basic information that every node should have
    def create_basic_node_configuration(self):
        config_path = os.path.dirname(os.path.abspath(__file__))
        config_path = config_path + '/basic_node.cfg'
        return self.read_config_file(config_path)

    #   set the attributes for a node dictionary and return a new dict
    def set_node_attribute(self,node,key,value):
        temp_node = node.copy()#dict(node) # create a new copy of dictionary
        temp_node[key] = value
        return temp_node

    def get_ip_address(self,ifname):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        return socket.inet_ntoa(fcntl.ioctl(
            s.fileno(),
            0x8915,  # SIOCGIFADDR
            struct.pack('256s', ifname[:15])
            )[20:24])



# Test
def main():
    configEditor = ConfigEditor()
    my_ip_address = configEditor.get_ip_address('eth0')
    nodeId = my_ip_address.split('.')
    nodeId[3] = str(int(nodeId[3])-1)
    target_address = '.'.join(nodeId)
    node = configEditor.create_basic_node_configuration()
    tx_freq = 450e6
    rx_freq = 458e6
    #node = configEditor.set_node_attribute(node,'rx_rate',4e6)
    node1 = configEditor.set_node_attribute(node,'server_ip',my_ip_address)
    node1 = configEditor.set_node_attribute(node1,'rx_freq',rx_freq)
    node1 = configEditor.set_node_attribute(node1,'tx_freq',tx_freq)
    node1 = configEditor.set_node_attribute(node1,'target_ip','10.0.0.2')
    node1 = configEditor.set_node_attribute(node1,'crts_ip','10.0.0.3')
    node2 = configEditor.set_node_attribute(node,'server_ip',target_address)
    node2 = configEditor.set_node_attribute(node2,'rx_freq',tx_freq)
    node2 = configEditor.set_node_attribute(node2,'tx_freq',rx_freq)
    node2 = configEditor.set_node_attribute(node2,'target_ip','10.0.0.3')
    node2 = configEditor.set_node_attribute(node2,'crts_ip','10.0.0.2')
    print node1.server_ip
    print node2.server_ip
    configEditor.add_to_output('node1',node1)
    configEditor.add_to_output('node2',node2)
    configEditor.add_to_output('num_nodes',2)
    configEditor.add_to_output('run_time',30.0)
    configEditor.write_config_file('/users/shemk/sas_crts/scenarios/sas_scenarios/test.cfg')
    #signal.signal(signal.SIGINT,newController.signal_handler)

if __name__ == '__main__':
    main()
