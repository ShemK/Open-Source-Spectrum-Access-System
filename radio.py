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




def main():

    # TODO: the ability for the user to choose input parameters
    # TODO: To check if the input from the socket is a JSON Object

    newCbsd = cbsd.Cbsd("cbd561","A","cbd1","hask124ba")
    newCbsd.add_registration_parameters("callSign","CB987")
    json_encoder = json.JSONEncoder()
    json_request =  json_encoder.encode(newCbsd.get_registrationRequestObj())
    my_server_connection = server_connection.Server_connection("http://128.173.95.235/spectrumAccessSystem/start.php")
    print("Request: sending registration request")
    # get response string
    response = my_server_connection.send_request(json_request)

    # create json decoder object
    json_decoder = json.JSONDecoder()
    # get dictionary
    json_response = json_decoder.decode(response)

    #change state of cbsd
    newCbsd.set_registrationResponseObj(json_response)



    # send channel inquiry in order of importance

    newCbsd.add_inquired_channels(890,900)
    newCbsd.add_inquired_channels(880,890)
    newCbsd.add_inquired_channels(860,870)
    newCbsd.add_inquired_channels(870,880)

    '''
    Inquire about the spectrum
    '''

    print "\n---------------------------------------------------------\n"

    print("Request: sending spectrum Inquiry ")

    print "Sending Inquiry for the Following Channels:"
    inquired_channels = newCbsd.get_inquired_channels()

    for i in newCbsd.get_inquired_channels():
        print i

    print "\n---------------------------------------------------------\n"
    # TODO: Need to make functions for different scenarios
    # TODO: Switch statement will do

    json_request = json_encoder.encode(newCbsd.get_spectrumInquiryRequestObj())
    response = my_server_connection.send_request(json_request)
        # get dictionary
    json_response = json_decoder.decode(response)

    newCbsd.set_spectrumInquiryResponseObj(json_response)
    print "Below are the Channels Available:"

    for i in newCbsd.get_available_channels():
        print i

    print "\n---------------------------------------------------------\n"

    '''
        Get Grant Request
    '''

    json_request = json_encoder.encode(newCbsd.get_grantRequestObj())

    print "Requesting Grant for the following Channel"
    print newCbsd.get_operationFrequencyRange()

    response = my_server_connection.send_request(json_request)
    # get dictionary
    json_response = json_decoder.decode(response)
    newCbsd.set_grantResponseObj(json_response)

    operationFrequency = newCbsd.get_operationFrequencyRange()

    print "Channel Grant Accepted"

    print "\n---------------------------------------------------------\n"

    print "Sending Initial Heartbeat"
    freq = operationFrequency['lowFrequency']

    heartbeatInterval = newCbsd.get_heartbeatInterval()
    json_request = json_encoder.encode(newCbsd.get_heartbeatRequestObj())
    response = my_server_connection.send_request(json_request)
    #print("Initial Heartbeat:", response)
    my_heartbeat_Thread = cbsd_thread.cbsd_thread(newCbsd,my_server_connection,\
                                        "heartbeat",heartbeatInterval,json_r = json_request)

    print "Heartbeat to be sent every",heartbeatInterval,"s"
    my_heartbeat_Thread.start()

    current_time = time.mktime(datetime.datetime.utcnow().timetuple())
#    print("server_time: ",newCbsd.get_grantExpireTime())
#    print("client time: ", datetime.datetime.utcnow().timetuple())
    grantInterval = newCbsd.get_grantExpireTime_seconds() - current_time
    print "Grant Ends in: ", grantInterval,"s"

    start_radio_Thread = cbsd_thread.cbsd_thread(newCbsd,my_server_connection,\
                                            "start_radio",0);
#    start_radio_Thread.start()


    my_grant_Thread = cbsd_thread.cbsd_thread(newCbsd,my_server_connection,\
                                            "grant",grantInterval,heartbeat_thread = my_heartbeat_Thread,\
                                            start_radio = start_radio_Thread)
    my_grant_Thread.start()

    myfifo = "/tmp/myfifo"

    f = open(myfifo,'w')
    f.write(str(freq))
    f.close()

    sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    while time.mktime(datetime.datetime.utcnow().timetuple()) < newCbsd.get_grantExpireTime_seconds():
        sock.sendto("asfjasjkfakjsfjkasfjasfjjjjjjjjjjasassasa",("10.0.10.5",5819))


  #  os.kill(os.getpid(), signal.SIGQUIT)
  #  os.kill(os.getpid(), signal.SIGINT)
  #
  #  os.kill(os.getpid(), signal.SIGTERM)

if __name__ == '__main__':
    main()
