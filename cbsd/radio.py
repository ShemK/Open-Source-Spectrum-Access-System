import sys
import os
import time
import datetime
import socket
import signal
import commands
import optparse
import math

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
import information_parser

#def stop():

newCbsd = None
stop_radio = False
stop_crts = False
manual = False

my_server_connection = None

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

def getCommand():
    try:
        newCbsd.get_command()
    except Exception as e:
        print "Issue communicating with controller "
        print e
        stop_radio = True
        sys.exit()

def register():
        global newCbsd
        global my_server_connection
        getCommand()
        link = "http://"+newCbsd.sas_ip+"/spectrumAccessSystem/start.php"
        print link
        print "--------------------------Starting-----------------------------"
        my_server_connection = server_connection.Server_connection(link)
        newCbsd.add_registration_parameters("callSign","CB987")
        newCbsd.add_registration_parameters("airInterface","Antenna")

        print("Request: sending registration request")
        # get response string
        json_decoder = json.JSONDecoder()

        newCbsd.sendRegistrationRequest(my_server_connection)

def run_radio():
        global stop_radio
        global newCbsd
        global my_server_connection

        json_encoder = json.JSONEncoder()
        json_request =  json_encoder.encode(newCbsd.get_registrationRequestObj())
        newCbsd.clear_channels()

        #change state of cbsd
        getCommand()
        link = "http://"+newCbsd.sas_ip+"/spectrumAccessSystem/start.php"
        my_server_connection = server_connection.Server_connection(link)

        print "CBSD STATE: ",newCbsd.get_cbsd_state()
        informationParser = information_parser.InformationParser(newCbsd.sas_ip,9749) # will need to move this to be part of cbsd

        groupedIDs = []
        if newCbsd.grouped != None:
            for i in range(0,len(newCbsd.grouped)):
                groupedIDs.append(informationParser.get_nodeID(newCbsd.grouped[i]))
                print "grouped: ", groupedIDs[i]
        # TODO: Add connection to internal database
        # send channel inquiry in order of importance
        if(newCbsd.get_cbsd_state() == "REGISTERED"):
            configEditor = config_editor.ConfigEditor()

            '''
            Inquire about the spectrum
            '''
            informationParser.addStatus("type","SU")
            informationParser.addStatus("state","REGISTERED")

            informationParser.sendStatus()

            if newCbsd.manual:
                try:
                    print "Please Use Low Frequency Above 3550MHz and below 3650MHz"
                    lowFrequency = math.floor(float(input("Low Frequency: "))/10)*10e6;
                    print "User Input in Hz: ",lowFrequency
                    if lowFrequency < 3550e6 or lowFrequency > 3650e6:
                        print "------------------Wrong User Low Frequency-----------------"
                        stop_radio = True
                        newCbsd.stop_cbsd()
                        return
                    else:
                        newCbsd.clear_inquired_channels()
                        newCbsd.clear_channels()
                        newCbsd.add_inquired_channels(lowFrequency,lowFrequency+10e6)
                except Exception as e:
                    print "----------------------------Wrong User Input-----------------"
                    stop_radio = True
                    newCbsd.stop_cbsd()
                    return

            newCbsd.sendSpectrumInquiry(my_server_connection)
            informationParser.addStatus("type","SU")
            informationParser.addStatus("state","SPECTRUM INQUIRY")

            #informationParser.addStatus("channels",newCbsd.get_inquired_channels())

            if len(groupedIDs) > 0:
                informationParser.addStatus("group",groupedIDs)

            informationParser.sendStatus()



            #    Get Grant Request


            newCbsd.sendGrantRequest(my_server_connection)

            #    start sending heartbeats

            print "GRANT STATE: ", newCbsd.get_grant_state()
            if newCbsd.get_grant_state() == "GRANTED" or newCbsd.get_grant_state() == "AUTHORIZED":
                # NOTE: this information is already being sent by crts
                informationParser.addStatus("type","SU")
                informationParser.addStatus("state","GRANT AUTHORIZED")
                freq_range = newCbsd.get_operationFrequencyRange()
                lowFrequency = float(freq_range['lowFrequency'])
                highFrequency = float(freq_range['highFrequency'])

                print "lowFrequency", lowFrequency

                informationParser.addStatus("lowFrequency",lowFrequency)
                informationParser.addStatus("highFrequency",highFrequency)
                if len(groupedIDs) > 0:
                    informationParser.addStatus("group",groupedIDs)
                informationParser.sendStatus()

            my_heartbeat_Thread = newCbsd.startSendingHeartBeats(my_server_connection)



            #    start radio interface


            start_radio_Thread = cbsd_thread.cbsd_thread(newCbsd,my_server_connection,\
                                                        "start_radio",0,config_editor = configEditor,\
                                                        heartbeat_thread = my_heartbeat_Thread);

            # start grant timer

            newCbsd.startGrant(my_server_connection,start_radio_Thread)
            if newCbsd.get_grant_state() != "IDLE":
                print "Starting Radio"
                start_radio_Thread.start()

            if newCbsd.my_heartbeat_Thread!=None:
                sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
                my_port = 7816
                sock.bind(("localhost",my_port))
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
            informationParser.addStatus("state","GRANT RELINQUISHED")
            informationParser.addStatus("type","SU")
            informationParser.sendStatus()

        else:
            informationParser.addStatus("state","UNREGISTERED")
            informationParser.addStatus("type","SU")

            if len(groupedIDs) > 0:
                informationParser.addStatus("group",groupedIDs)

            informationParser.sendStatus()

        print "Stopping All Radio Functionalities"
        stop_crts()
        time.sleep(5)

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
    parser = optparse.OptionParser()
    parser.add_option('-m', '--manual',action="store_true", dest="manual", default=False, help="get user input in regard to parameters")
    parser.add_option('-l', '--lower', action="store", dest="lowFrequency", type="float",default=-1, help="Initial Low Frequency in MHz")
    parser.add_option('-u', '--upper', action="store", dest="highFrequency", type="float", default=-1, help="Initial High Frequency in MHz")

    options, args = parser.parse_args()
    '''
    if(options.manual):
        options.lowFrequency = options.lowFrequency *1e6
        options.highFrequency = options.highFrequency *1e6
        try:
            if options.lowFrequency < 3500e6 or options.lowFrequency > 3750e6:
                print "Please Use Low Frequency Above 3500MHz and below 3750MHz"
                options.lowFrequency = float(input("Low Frequency: "))*1e6;
                if options.lowFrequency < 3500e6 or options.lowFrequency > 3750e6:
                    print "Wrong Low Frequency"
                    return
            if options.highFrequency < 3500e6 or options.lowFrequency > 3750e6:
                print "Please Use High Frequency Above 3500e6 and below 3750MHz"
                options.lowFrequency = math.floor(float(input("High Frequency: "))/10)*10e6;
                print "Your High Frequency: ",options.lowFrequency
                if options.lowFrequency < 3500e6 or options.lowFrequency > 3750e6:
                    print "Wrong High Frequency"
                    return

        except Exception as e:
            print "Error with user inputs"
            return

    '''
    signal.signal(signal.SIGUSR1, handler)
    signal.signal(signal.SIGUSR2, handler)
    signal.signal(signal.SIGALRM, handler)
    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGQUIT, handler)
    global stop_radio
    global newCbsd
    global manual
    manual = options.manual


    newCbsd = cbsd.Cbsd("cbd561","A","cbd1","hask124ba","yap")
    newCbsd.manual = manual

    register()

    while stop_radio == False:
        print "Stop: ", stop_radio
        run_radio()

if __name__ == '__main__':
    main()
