import sys
import os
import time
import datetime
import socket
import signal
import commands

scripts = os.path.dirname(os.path.abspath(__file__))
scripts = scripts + '/python_scripts'
sys.path.insert(0, scripts)

import server_connection
import cbsd
import json
import cbsd_thread

scripts = os.path.dirname(os.path.abspath(__file__))
scripts = scripts + '/config_scripts'
sys.path.insert(0, scripts)
import config_editor

#def stop():
newCbsd = cbsd.Cbsd("cbd561","A","cbd1","hask124ba","yap")
stop_radio = False
stop_crts = False

def handler(signum, frame):
    print 'Signal handler called with signal', signum
    global stop_crts
    if signum == 14:
        os.kill(os.getpid(), signal.SIGINT)
        time.sleep(10)
        #stop_crts == True
    else:
        #if stop_crts == False:
        global stop_radio
        stop_radio = True
        print "Stop_sig: ", stop_radio
        newCbsd.stop_cbsd()
        if newCbsd.get_grant_state != "IDLE":
            newCbsd.start_radio_thread.stopRadio()
            newCbsd.my_heartbeat_Thread.stopThread()
        #else:
        #    stop_crts = False
            #newCbsd.my_heartbeat_Thread.stop_thread

def run_radio():
        global stop_radio
        json_encoder = json.JSONEncoder()
        json_request =  json_encoder.encode(newCbsd.get_registrationRequestObj())
        newCbsd.clear_channels()

        try:
            newCbsd.get_command()
        except Exception as e:
            print "Issue communicating with controller"

        sas_ip = newCbsd.sas_ip

        link = "http://"+sas_ip+"/spectrumAccessSystem/start.php"
        print link
        print "--------------------------Starting-----------------------------"
        my_server_connection = server_connection.Server_connection(link)
        newCbsd.add_registration_parameters("callSign","CB987")
        newCbsd.add_registration_parameters("airInterface","Antenna")

        print("Request: sending registration request")
        # get response string
        json_decoder = json.JSONDecoder()

        newCbsd.sendRegistrationRequest(my_server_connection)

        #change state of cbsd

        print "CBSD STATE: ",newCbsd.get_cbsd_state()

        # TODO: Add connection to internal database
        # send channel inquiry in order of importance
        if(newCbsd.get_cbsd_state() == "REGISTERED"):
            '''
            newCbsd.add_inquired_channels(890e6,900e6)
            newCbsd.add_inquired_channels(880e6,890e6)
            newCbsd.add_inquired_channels(860e6,870e6)
            newCbsd.add_inquired_channels(870e6,880e6)
            newCbsd.add_inquired_channels(870e6,880e6)
            '''
        configEditor = config_editor.ConfigEditor()
        '''
        Inquire about the spectrum
        '''

        newCbsd.sendSpectrumInquiry(my_server_connection)

        '''
            Get Grant Request
        '''

        newCbsd.sendGrantRequest(my_server_connection)
        '''
            start sending heartbeats
        '''

        newCbsd.startSendingHeartBeats(my_server_connection)


        '''
            start radio interface
        '''

        start_radio_Thread = cbsd_thread.cbsd_thread(newCbsd,my_server_connection,\
                                                    "start_radio",0,config_editor = configEditor);
        '''
         start grant timer
        '''
        newCbsd.startGrant(my_server_connection,start_radio_Thread)
        if newCbsd.get_grant_state != "IDLE":
            print "Starting Radio"
            start_radio_Thread.start()

        #start_radio_Thread.join()
        #if(newCbsd.grantTimeLeft!=None):
        #    time.sleep(newCbsd.grantTimeLeft)
        if newCbsd.my_heartbeat_Thread!=None:
            sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
            my_port = 7816
            sock.bind(("0.0.0.0",my_port))
            sock.settimeout(1.0)
            data = None
            while data == None and stop_radio == False:
                try:
                    data = sock.recvfrom(1024)
                except Exception as e:
                    if not newCbsd.my_heartbeat_Thread.isAlive():
                        break
                    pass
            print "Received something: ",data
            sock.close()
            #newCbsd.my_heartbeat_Thread.join()
        #server_connection.close()
        stop_crts()
        time.sleep(30)

def stop_crts():
    pids = commands.getoutput("ps -ef | grep crts_controller | grep -v grep | awk '{print $2}'").split()
    for pid in pids:
        print pid
        cmd = 'kill -2 '
        cmd += pid
        commands.getoutput(cmd)


def main():

    # TODO: the ability for the user to choose input parameters
    # TODO: To check if the input from the socket is a JSON Object

    signal.signal(signal.SIGUSR1, handler)
    signal.signal(signal.SIGUSR2, handler)
    signal.signal(signal.SIGALRM, handler)
    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGQUIT, handler)
    global stop_radio
    while stop_radio == False:
        print "Stop: ", stop_radio
        run_radio()



if __name__ == '__main__':
    main()
