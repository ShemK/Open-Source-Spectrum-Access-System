import sys
import os
import time
import datetime
import socket
import signal

scripts = os.getcwd()
scripts = scripts + '/python_scripts'
sys.path.insert(0, scripts)

import server_connection
import cbsd
import json
import cbsd_thread

scripts = os.getcwd()
scripts = scripts + '/config_scripts'
sys.path.insert(0, scripts)
import config_editor

def main():

    # TODO: the ability for the user to choose input parameters
    # TODO: To check if the input from the socket is a JSON Object

    newCbsd = cbsd.Cbsd("cbd561","A","cbd1","hask124ba","yap")
    newCbsd.add_registration_parameters("callSign","CB987")
    newCbsd.add_registration_parameters("airInterface","Antenna")
    json_encoder = json.JSONEncoder()
    json_request =  json_encoder.encode(newCbsd.get_registrationRequestObj())
    my_server_connection = server_connection.Server_connection("http://127.0.0.1/spectrumAccessSystem/start.php")
    print("Request: sending registration request")
    # get response string
    json_decoder = json.JSONDecoder()
    newCbsd.sendRegistrationRequest(my_server_connection)

    #change state of cbsd

    print "CBSD STATE: ",newCbsd.get_cbsd_state()

    # TODO: Add connection to internal database
    # send channel inquiry in order of importance
    if(newCbsd.get_cbsd_state() == "REGISTERED"):
        newCbsd.get_command();
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

    start_radio_Thread.start()

    #time.sleep(10)

    """
    myfifo = "/tmp/myfifo"

    f = open(myfifo,'w')
    f.write(str(freq))
    f.close()

    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    while time.mktime(datetime.datetime.utcnow().timetuple()) < newCbsd.get_grantExpireTime_seconds():
        sock.sendto("asfjasjkfakjsfjkasfjasfjjjjjjjjjjasassasa",("10.0.10.5",5819))
    """

  #  os.kill(os.getpid(), signal.SIGQUIT)
  #  os.kill(os.getpid(), signal.SIGINT)
  #
  #  os.kill(os.getpid(), signal.SIGTERM)

if __name__ == '__main__':
    main()
