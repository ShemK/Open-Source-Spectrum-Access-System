import threading
import server_connection
import gps_reader
import time
import sys
import os
import json
import commands
import datetime

class cbsd_thread(threading.Thread):
    my_Interval = None
    my_server_connection = None
    interval_type = None
    json_request = None
    stop_thread = False
    my_cbsd = None
    heartbeat_thread = None
    grant_state = None
    response = None
    start_radio = None
    def __init__(self,my_cbsd,my_server_connection,\
                    interval_type,myInterval,**args):
        threading.Thread.__init__(self)
        self.my_server_connection = my_server_connection
        self.my_Interval = myInterval
        self.interval_type = interval_type
        self.json_request = args.get('json_r',None)
        self.heartbeat_thread = args.get('heartbeat_thread',None)
        self.my_cbsd = my_cbsd
        self.grant_state = args.get('grant_state',None)
        self.start_radio = args.get('start_radio',None)
        self.configEditor = args.get('config_editor',None)

    def run(self):
        if self.interval_type == "heartbeat":
            json_decoder = json.JSONDecoder()
            json_encoder = json.JSONEncoder()
            while(self.stop_thread == False):
                current_time = time.mktime(datetime.datetime.utcnow().timetuple())
                if (self.my_cbsd.get_grantExpireTime_seconds() - current_time) < 0:
                    self.stop_thread = True

                time.sleep(self.my_Interval)
                print("Sending Heartbeat")
                try:
                    self.response = self.my_server_connection.send_request(self.json_request)
                    self.my_cbsd.analyze_heartbeatResponseObj(json_decoder.decode(self.response))
                    # TODO: we need to check type of response and make good decision
                    # TODO: The cbsd instance needs to be added to the thread -- done
                except ValueError as e:
                    print "Wrong response Received",e
                    self.stop_thread = True

            print "\n---------------------------------------------------------\n"
            print "Transmission Done, Grant Relinquished"

            print "Sending relinquishmentRequest"
            #self.start_radio.stopThread()
            self.json_request = json_encoder.encode(self.my_cbsd.get_relinquishmentRequestObj())
            self.response = self.my_server_connection.send_request(self.json_request)
            #print self.response
            #closing connection
            #self.my_server_connection.close_connection()
            self.stopRadio()

        elif self.interval_type == "grant":
            #if(self.my_Interval > 0):
            #    time.sleep(self.my_Interval)
            #self.heartbeat_thread.stopThread()
            #self.start_radio.stopThread()
            pass

            #quit()
           # sys.exit()

        elif self.interval_type == "start_radio":
        #    print("Nothing!")
            print("Thread Id:", os.getpid())
            if self.my_cbsd.grouped != None:
                self.create_config_file()
                x_path = os.path.dirname(os.path.abspath(__file__))
                x_path = x_path + "/../../crts/crts_controller -s sas_scenarios/test"
                os.system(x_path)
                pass

    def stopRadio(self):
        pids = commands.getoutput("ps -ef | grep crts_controller | grep -v grep | awk '{print $2}'").split()
        for pid in pids:
            print pid
            cmd = 'kill -15 '
            cmd += pid
            #print cmd
            #kill the process
            commands.getoutput(cmd)


    def stopThread(self):
        self.stop_thread = True

    def get_response(self):
        return self.response

    def create_config_file(self):
        nodes = []
        freq = self.my_cbsd.get_operationFrequencyRange()
        chan_list = self.assign_channels()
        for i in range(0,len(self.my_cbsd.grouped)):
            node = self.configEditor.create_basic_node_configuration()
            my_ip_address = self.my_cbsd.grouped[i]
            nodeId = my_ip_address.split('.')
            nodeId[3]
            crts_ip = '10.0.0.' + nodeId[3]

            lowFrequency = chan_list[i]['lowFrequency']
            highFrequency = chan_list[i]['highFrequency']
            bw = highFrequency - lowFrequency

            center_frequency = lowFrequency + bw/2;
            #print 'lowFrequency' , lowFrequency
            #print 'highFrequency', highFrequency
            if i%2 == 0:
                target_ip_address = self.my_cbsd.grouped[i+1]
                nodeId = target_ip_address.split('.')
                target_ip = '10.0.0.' + nodeId[3]
                target_bw = chan_list[i+1]['highFrequency'] - chan_list[i+1]['lowFrequency']
                target_freq = chan_list[i+1]['lowFrequency'] + target_bw/2;
            else:
                target_ip_address = self.my_cbsd.grouped[i-1]
                nodeId = target_ip_address.split('.')
                target_ip = '10.0.0.' + nodeId[3]
                target_bw = chan_list[i-1]['highFrequency'] - chan_list[i-1]['lowFrequency']
                target_freq = chan_list[i-1]['lowFrequency'] + target_bw/2;
            #node = configEditor.set_node_attribute(node,'rx_rate',4e6)
            node_temp = self.configEditor.set_node_attribute(node,'server_ip',my_ip_address)
            nodes.append(node_temp)

            nodes[i] = self.configEditor.set_node_attribute(node_temp,'rx_freq',center_frequency)
            nodes[i] = self.configEditor.set_node_attribute(nodes[i],'tx_freq',target_freq)
            nodes[i]  = self.configEditor.set_node_attribute(nodes[i],'target_ip',target_ip)
            nodes[i]  = self.configEditor.set_node_attribute(nodes[i],'crts_ip',crts_ip)
            nodes[i]  = self.configEditor.set_node_attribute(nodes[i],'tx_rate',1e6)
            nodes[i]  = self.configEditor.set_node_attribute(nodes[i],'rx_rate',1e6)
            node_string = 'node' + str(i+1)
            self.configEditor.add_to_output(node_string,nodes[i])
        self.configEditor.add_to_output('num_nodes',len(self.my_cbsd.grouped))
        self.configEditor.add_to_output('run_time',self.my_cbsd.grantTimeLeft-5)

        if not self.my_cbsd.init_sc  == None:
            self.configEditor.add_to_output('scenario_controller',self.my_cbsd.init_sc)

        config_path = os.path.dirname(os.path.abspath(__file__))
        config_path = config_path + "/../../crts/scenarios/sas_scenarios/test.cfg"
        print "Written to : ", config_path
        self.configEditor.write_config_file(config_path)

    def assign_channels(self):
        freq_range = self.my_cbsd.get_operationFrequencyRange()
        lowFrequency = freq_range['lowFrequency']
        highFrequency = freq_range['highFrequency']
        num_nodes = len(self.my_cbsd.grouped)
        bandwidth = highFrequency - lowFrequency
        if self.my_cbsd.cornet_config.has_key('guard_band'):
            guard_band = self.my_cbsd.cornet_config['guard_band']*1e6
        else:
            guard_band = 2e6

        node_channel_list = [];
        '''
        # FIXME : Fix issues with more than 2 node
        lowest_guard_band = bandwidth/num_nodes
        lowest_channel = 4e6#lowest_guard_band - guard_band/2

        current_channel = lowest_channel
        lowest_channel = 0
        chan_bw = current_channel - lowest_channel
        for i in range(0,num_nodes):
            node = dict()
            node['highFrequency'] = current_channel + lowFrequency
            node['lowFrequency'] = lowest_channel + lowFrequency
            node_channel_list.append(node.copy())
            current_channel = current_channel + guard_band + chan_bw
            lowest_channel = current_channel - chan_bw
        '''
        node = dict()
        node['highFrequency'] = 5e6 + lowFrequency
        node['lowFrequency'] =  3e6  + lowFrequency

        node_channel_list.append(node.copy())

        node['highFrequency'] = 8e6 + lowFrequency
        node['lowFrequency'] =  6e6  + lowFrequency

        node_channel_list.append(node.copy())

        return node_channel_list
