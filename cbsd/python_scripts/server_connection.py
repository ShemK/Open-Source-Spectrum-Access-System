#!/usr/lib/python

import sys
import os
import httplib


class Server_connection(object):
    """
        Object used to create an Http Connection to a server
        The object can make requests and retrieve responses
        from the server
    """
    _link = ""
    _http_connection = None
    _connection_established = False
    _url = None
    def __init__(self, url):
        self._url = url
        temp = url.split("//")
        temp2 = temp[1].split("/")
        self._link = temp2[0]
        try:
            self._http_connection = httplib.HTTPConnection(self._link)
            self._connection_established = True
        except Exception :
            self._connection_established = False
            print("Connection Failed")

    def send_request(self,message):
        if(self._connection_established == True):
            headers = {"Content-Type":"application/json"}
            print "sending"
            print message
            self._http_connection.request("POST",self._url,message,headers)
            http_response = self._http_connection.getresponse()
            reply = http_response.read()
            #print("raw reply:")
            #print(reply)

        else:
            print("The connection failed. Request cannot be made")
            self._connection_established = False
            reply = -1
        return reply

    def close_connection(self):
        self._http_connection.close()

    def is_connected(self):
        return self._connection_established
