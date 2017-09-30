import threading
import server_connection
import time
import sys
import os
import json

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


    def run(self):
        if self.interval_type == "heartbeat":
            while(self.stop_thread == False):
                time.sleep(self.my_Interval)
                print("Sending Heartbeat")
                try:
                    self.response = self.my_server_connection.send_request(self.json_request)
                    # TODO: we need to check type of response and make good decision
                    # TODO: The cbsd instance needs to be added to the thread
                except ValueError:
                    print "Wrong response Received"
                    self.stop_thread = False

            self.stop_thread = False
            print "\n---------------------------------------------------------\n"
            print "Transmission Done, Grant Relinquished"
        elif self.interval_type == "grant":
            if(self.my_Interval > 0):
                time.sleep(self.my_Interval)
            self.heartbeat_thread.stopThread()
            self.start_radio.stopThread()
            json_encoder = json.JSONEncoder()

            print "Sending relinquishmentRequest"

            self.json_request = json_encoder.encode(self.my_cbsd.get_relinquishmentRequestObj())
            self.response = self.my_server_connection.send_request(self.json_request)
            #print self.response
            #closing connection
            self.my_server_connection.close_connection()
            self.stopThread()
            #quit()
           # sys.exit()

        elif self.interval_type == "start_radio":
        #    print("Nothing!")
            print("Thread Id:", os.getpid())
            os.system("./radio_interface 854")


    def stopThread(self):
        self.stop_thread = True

    def get_response(self):
        return self.response
