import server_connection
import json
import datetime
import time

class Cbsd(object):
    _fccId = "cbd561"
    _cbsdCategory = "A"
    _userId = "cbd1"
    _state = "Unregistered"
    _cbsdSerialNumber = "hask124ba"
    _cbsdInfo = "yap";
    _cbsd_registered = False
    _grant_state = None
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
    _grantRenew = False
    _relinquishGrant = False
    _reliquishmentRequestObj = None


    def __init__(self, fccId, cbsdCategory,userId,cbsdSerialNumber,cbsdInfo):
        self._fccId = fccId
        self._cbsdCategory = cbsdCategory
        self._userId = userId
        self._cbsdSerialNumber = cbsdSerialNumber
        self._cbsdInfo = cbsdInfo
        self._registrationRequestObj = self._create_registration_request_obj()

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
            print('registered!')
            self._cbsd_registered = True
            self._state = "Registered"


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
                self._grant_state = "Idle"
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

    def set_grantResponseObj(self, grantReponse):
        self._grantResponseObj = grantReponse
        responseCode = grantReponse['grantResponse'] \
                                                ['response']['responseCode']

        if(responseCode=="0"):
            self._grantId = grantReponse['grantResponse']['grantId']
            self._heartbeatInterval = grantReponse['grantResponse']['heartbeatInterval']
            self._grantExpireTime = grantReponse['grantResponse']['grantExpireTime']
            self._grant_state = "GRANTED"
            self.get_grantExpireTime_seconds(self._grantExpireTime)


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
        for i in range(0,len(self._available_channels)):
            self._available_channels.pop(len(self._available_channels)-1)
