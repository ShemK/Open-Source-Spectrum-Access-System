import server_connection
import json
import datetime
import time
import cbsd_thread
import socket
import gps_reader
from gps_reader import *


class Cbsd(object):
    _fccId = "cbd561"
    _cbsdCategory = "A"
    _userId = "cbd1"
    _state = "UNREGISTERED"
    _cbsdSerialNumber = "hask124ba"
    _cbsdInfo = "yap";
    _cbsd_registered = False
    _grant_state = "IDLE"
    _registrationRequestObj = None
    _cbsdId = None
    _registrationResponseObj = None
    _inquired_channels = []
    _spectrumInquiryRequestObj = None
    _available_channels = []
    _spectrumInquireResponseObj = None
    _maxEirp = 100
    _operationFrequencyRange = None
    _grantRequestObj= None
    _grantResponseObj = None
    _heartbeatInterval = 0
    _grantId = None
    _grantExpireTime = None
    _grantExpireTime_seconds = None
    _heartbeatRequestObj = None
    _heartbeatResponseObj = None
    _grantRenew = False
    _relinquishGrant = False
    _reliquishmentRequestObj = None
    _json_encoder = None
    _json_decoder = None
    grantTimeLeft = None
    my_heartbeat_Thread = None
    installationParameters = None
    def __init__(self, fccId, cbsdCategory,userId,cbsdSerialNumber,cbsdInfo):
        self._fccId = fccId
        self._cbsdCategory = cbsdCategory
        self._userId = userId
        self._cbsdSerialNumber = cbsdSerialNumber
        self._cbsdInfo = cbsdInfo
        self._registrationRequestObj = self._create_registration_request_obj()
        self.create_installationParameterObj()
        self._json_encoder = json.JSONEncoder()
        self._json_decoder = json.JSONDecoder()
        self.gpsReader = GPSReader(self)
        self.start_gps()
        self.grouped = None

    def _create_registration_request_obj(self):
        temp_registrationObj = {'registrationRequest': {
        	                            "fccId": self._fccId,
                                        "cbsdCategory": self._cbsdCategory,
                                        "userId": self._userId,
                                        "cbsdSerialNumber": self._cbsdSerialNumber,
                                        "cbsdInfo": self._cbsdInfo
                                        }
                                }
        return temp_registrationObj

    def add_registration_parameters(self,key,value):
        self._registrationRequestObj['registrationRequest'][key] = value

    def get_registrationRequestObj(self):
        return self._registrationRequestObj

    def set_cbsdId(self,cbsdId):
        self._cbsdId = cbsdId

    def set_registrationResponseObj(self,registrationResponse):
        self._registrationResponseObj = registrationResponse

        # \ continues to next line
        responseCode = registrationResponse['registrationResponse'] \
                                            ['response']['responseCode']

        if(responseCode=="0"):
            self._cbsdId = registrationResponse['registrationResponse'] \
                                                ['response']['cbsdId']
            print('REGISTERED!')
            self._cbsd_registered = True
            self._state = "REGISTERED"
        if(responseCode == "103"):
            self._state = "UNREGISTERED"
            self._cbsd_registered = False
            responseMessage = registrationResponse['registrationResponse'] \
                                                ['response']['responseMessage']
            print "Registration Failed: ", responseMessage
        if(responseCode == "202"):
            self._state = "UNREGISTERED"
            self._cbsd_registered = False
            responseMessage = registrationResponse['registrationResponse'] \
                                                ['response']['responseMessage']
            print "Registration Failed: ", responseMessage

    def get_cbsd_state(self):
        return self._state;

    def add_inquired_channels(self,lowFrequency,highFrequency):
        temp_channel = {'lowFrequency':lowFrequency,'highFrequency':highFrequency}
        self._inquired_channels.append(temp_channel)

    def _create_spectrum_request_obj(self):
        if(self._cbsd_registered == True):
            if len(self._inquired_channels) > 0:
                self._spectrumInquiryRequestObj = { 'spectrumInquiryRequest' : {
                                                        'cbsdId':self._cbsdId,
                                                        'inquiredSpectrum':self._inquired_channels
                                                        }
                                                    }
                return True
            else:
                print('CBSD has not selected channels to inquire')
                return False
        else:
            print('CBSD has not been registered')
            return False


    def get_spectrumInquiryRequestObj(self):
        if self._create_spectrum_request_obj() == True:
            return self._spectrumInquiryRequestObj

    def set_spectrumInquiryResponseObj(self,spectrumInquiryResponse):
        self._spectrumInquireResponseObj = spectrumInquiryResponse
        responseCode = spectrumInquiryResponse['spectrumInquiryResponse'] \
                                                ['response']['responseCode']

        if(responseCode=="0"):
            self._available_channels = spectrumInquiryResponse['spectrumInquiryResponse'] \
                                                        ['availableChannel']
            if(len(self._available_channels) > 0):
                print("Hurray, There is a space for you, scoot over")
            else:
                print("Sorry, No space of you bro!")

    def _create_grant_request_obj(self):
        if(self._cbsd_registered == True):
            if len(self._available_channels) > 0:
                self._operationFrequencyRange = self._available_channels[0]
                #self._available_channels.clear()
                self.clear_channels()
            if(self._operationFrequencyRange!=None):
                self._grantRequestObj = { 'grantRequest': {
                                                'cbsdId':self._cbsdId,
                                                'operationParam': {
                                                    'maxEirp':self._maxEirp,
                                                    'operationalFrequencyRange': \
                                                        self._operationFrequencyRange
                                                    }
                                            }
                                        }
                self._grant_state = "IDLE"
                return True
            else:
                print("No Operational Frequency Range Selected")
                return False
        else:
            print("CBSD is not registered")
            return False

    def get_grantRequestObj(self):
        if self._create_grant_request_obj() == True:
            return self._grantRequestObj

    def get_grantExpireTime(self):
        return self._grantExpireTime

    def set_grantResponseObj(self, grantResponse):
        self._grantResponseObj = grantResponse
        responseCode = grantResponse['grantResponse'] \
                                                ['response']['responseCode']

        if(responseCode=="0"):
            self._grantId = grantResponse['grantResponse']['grantId']
            if 'heartbeatInterval' in grantResponse['grantResponse']:
                self._heartbeatInterval = grantResponse['grantResponse']['heartbeatInterval']
            self._grantExpireTime = grantResponse['grantResponse']['grantExpireTime']
            self._grant_state = "AUTHORIZED"
            self.get_grantExpireTime_seconds(self._grantExpireTime)
            #clear channel list
            self._inquired_channels = []
        else:
            self._grant_state = "IDLE"

    def get_grantExpireTime_seconds(self,grantExpireTime=None):
        if grantExpireTime == None:
            return self._grantExpireTime_seconds
        else:
            temp_time = datetime.datetime.strptime(grantExpireTime[0:22],"%Y-%b-%d%Z%H:%M:%S")
            self._grantExpireTime_seconds = time.mktime(temp_time.timetuple())
            return self._grantExpireTime_seconds


    def _create_heart_request_obj(self):
        if((self._grant_state == "GRANTED") \
            or (self._grant_state == "AUTHORIZED")):
            if(self._cbsd_registered == True):
                self._heartbeatRequestObj = { 'heartbeatRequest': {
                                                'cbsdId': self._cbsdId,
                                                'grantId': self._grantId,
                                                'grantRenew': self._grantRenew,
                                                'operationState': self._grant_state
                                                }
                                            }
        else:
            return None

    def analyze_heartbeatResponseObj(self, heartbeatResponse):
        if self._grant_state != "IDLE":
            self._heartbeatResponseObj = heartbeatResponse
            responseCode = heartbeatResponse['heartbeatResponse'] \
                                                    ['response']['responseCode']

            if(responseCode=="0"):
                self._grantId = heartbeatResponse['heartbeatResponse']['grantId']
                if 'heartbeatInterval' in heartbeatResponse['heartbeatResponse']:
                    self._heartbeatInterval = heartbeatResponse['heartbeatResponse']['heartbeatInterval']
                if 'operationParam' in heartbeatResponse['heartbeatResponse']:
                    print "Change Operational parameters"
            else:
                self._grant_state = "IDLE"

        else:
            print "No Grant Available"

    def _create_relinquishment_request_obj(self):
        if((self._grant_state == "GRANTED") \
            or (self._grant_state == "AUTHORIZED")):
            if(self._cbsd_registered == True):
                self._reliquishmentRequestObj = { 'relinquishmentRequest': {
                                                    'cbsdId': self._cbsdId,
                                                    'grantId': self._grantId
                                                    }
                                                }


    def get_relinquishmentRequestObj(self):
        self._create_relinquishment_request_obj()
        return self._reliquishmentRequestObj

    def set_relinquishmentRequest(self):
        self._relinquishGrant = True

    def get_heartbeatRequestObj(self):
        self._create_heart_request_obj()
        return self._heartbeatRequestObj

    def get_heartbeatInterval(self):
        return self._heartbeatInterval

    def get_operationFrequencyRange(self):
        return self._operationFrequencyRange

    def get_inquired_channels(self):
        return self._inquired_channels

    def get_available_channels(self):
        return self._available_channels

    def get_grant_state(self):
        return self._grant_state

    def get_relinquishGrant(self):
        return self._relinquishGrant


    # Clear available channel list
    def clear_channels(self):
        self._available_channels = []
#        for i in range(0,len(self._available_channels)):
#            self._available_channels.pop(len(self._available_channels)-1)



    '''
    Functions that connect with SAS

    '''
    def sendRegistrationRequest(self,my_server_connection):
        if(my_server_connection.is_connected()):
            json_request =  self._json_encoder.encode(self.get_registrationRequestObj())
            print json_request
            response = my_server_connection.send_request(json_request)

            # create json decoder object
            # get dictionary

            try:
                json_response = self._json_decoder.decode(response)
                self.set_registrationResponseObj(json_response)
            except ValueError:
                print "Unexpected Message From SAS: Connection Broken"
                print "Message Received: ", response
        else:
            print "No Connection to SAS"

    def sendSpectrumInquiry(self,my_server_connection):
        if(my_server_connection.is_connected()):
            if(self._cbsd_registered):
                print "\n---------------------------------------------------------\n"

                print("Request: sending spectrum Inquiry ")

                print "Sending Inquiry for the Following Channels:"

                for i in self.get_inquired_channels():
                    print i

                print "\n---------------------------------------------------------\n"
                # TODO: Need to make functions for different scenarios
                # TODO: Switch statement will do

                json_request = self._json_encoder.encode(self.get_spectrumInquiryRequestObj())

                response = my_server_connection.send_request(json_request)

                try:
                    json_response = self._json_decoder.decode(response)
                    self.set_spectrumInquiryResponseObj(json_response)
                    print "Below are the Channels Available:"

                    for i in self.get_available_channels():
                        print i

                    print "\n---------------------------------------------------------\n"

                except ValueError:
                    print "Unexpected Message From SAS: Connection Broken"
                    print "Message Received: ", response
                else:
                    print "No Connection to SAS"
            else:
                print "CBSD is not Registered"

    def sendGrantRequest(self,my_server_connection):
        if(my_server_connection.is_connected()):
            if(len(self._available_channels) > 0):
                try:
                    json_request = self._json_encoder.encode(self.get_grantRequestObj())

                    print "Requesting Grant for the following Channel"
                    print self.get_operationFrequencyRange()

                    response = my_server_connection.send_request(json_request)

                    # get dictionary
                    json_response = self._json_decoder.decode(response)
                    self.set_grantResponseObj(json_response)


                    print "Channel Grant Accepted"

                    print "\n---------------------------------------------------------\n"

                except ValueError:
                    print "Unexpected Message From SAS: Connection Broken"
                    print "Message Received: ", response
            else:
                print "No channels were provided by the SAS"
        else:
            print "No Connection to SAS"

    def startSendingHeartBeats(self, my_server_connection):
        print "Sending Initial Heartbeat"
        if(self._grant_state!="IDLE"):
            operationFrequency = self.get_operationFrequencyRange()

            freq = operationFrequency['lowFrequency']

            heartbeatInterval = self.get_heartbeatInterval()
            if heartbeatInterval!=None:
                json_request = self._json_encoder.encode(self.get_heartbeatRequestObj())
                response = my_server_connection.send_request(json_request)
                self.my_heartbeat_Thread = cbsd_thread.cbsd_thread(self,my_server_connection,\
                                          "heartbeat",heartbeatInterval,json_r = json_request)

                print "Heartbeat to be sent every",heartbeatInterval,"s"
                self.my_heartbeat_Thread.start()

                current_time = time.mktime(datetime.datetime.utcnow().timetuple())
         #     print("server_time: ",newCbsd.get_grantExpireTime())
       #     print("client time: ", datetime.datetime.utcnow().timetuple())
                self.grantTimeLeft = self.get_grantExpireTime_seconds() - current_time
                print "Grant Ends in: ", self.grantTimeLeft,"s"
            else:
                print "No Grants Provided"
        else:
            print "Grant State = IDLE, No Grant Provided"


    def startGrant(self,my_server_connection,start_radio_Thread):
        self.start_radio_thread = start_radio_Thread
        if self._grant_state != "IDLE":
            my_grant_Thread = cbsd_thread.cbsd_thread(self,my_server_connection,\
                        "grant",self.grantTimeLeft,heartbeat_thread = self.my_heartbeat_Thread,\
                        start_radio = start_radio_Thread)
            my_grant_Thread.start()
            self.clear_channels()

    def get_command(self):
        sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM) # UDP
        sock.bind(('localhost', 7800))
        data, addr = sock.recvfrom(1024)
        json_command = self._json_decoder.decode(data)
        print json_command
        channel_list = json_command['channels']
        self.cornet_config = json_command['cornet_config']
        if self.cornet_config != None:
            self.grouped = self.cornet_config['grouped']
            if self.grouped != None:
                self.sas_ip = self.cornet_config['sas']
        else:
            print "No SAS Provided, assuming localhost"
            self.sas_ip = "127.0.0.1"

        # NOTE: Need to make variable bandwidth - 10 MHz is current quasi standard
        i = 0
        while i < len(channel_list):
            self.add_inquired_channels(channel_list[i],channel_list[i]+10e6)
            i = i+1

    def create_installationParameterObj(self):
        #NOTE: Will need to change this to read from a configuration file
        self.installationParameters = {
            'latitude':0,
            'longitude':0,
            'indoorDeployment':'false',
            'heightType':'AGL',
            'antennaAzimuth':271,
            'antennaDowntilt':3,
            'antennaGain':16,
            'antennaBeamwidth':30
        }
        self.add_registration_parameters('installationParam',self.installationParameters)

    def add_installation_parameters(self,key,value):
        self.installationParameters[key] = value
        self.add_registration_parameters('installationParam',self.installationParameters)

    def start_gps(self):
        self.gpsReader.start()

    def stop_gps(self):
        self.gpsReader.stop_gps()

    def stop_cbsd(self):
        self.stop_gps()
