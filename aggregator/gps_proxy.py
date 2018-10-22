#!/usr/bin/env python

import socket
import json
import atexit
from decimal import *
import time

def changeLocation(input_data,lat,lon):
    changed_data = "";
    pos = input_data.find("lon")
    class_pos = input_data.find("class")
    speed_pos = input_data.find("speed")
    end_bracket = input_data.find("}")
    if pos != -1 and speed_pos != -1 and class_pos != -1:
        #print pos,speed_pos,class_pos
        if class_pos > end_bracket:
            speed_pos = input_data.find("speed",class_pos)
        if speed_pos == -1:
            return input_data

        input_data = input_data[class_pos-1:speed_pos+12]
        input_list = input_data.split(',')
        lat_list = input_list[5].split(':')
        lat_list[1] = str(lat)
        input_list[5] = ':'.join(lat_list)
        lon_list = input_list[6].split(':')
        lon_list[1] = str(lon)
        input_list[6] = ':'.join(lon_list)
        changed_data = ','.join(input_list)
        #changed_data = '{'+input_data+'}\r\n'
        changed_data = "{"+ changed_data + "}\r\n"
        #print "Additional data: ", input_data[speed_pos:speed_pos+15]
        return changed_data
    else:
        return input_data

getcontext().prec = 9

count = 0
known_gps = None
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

sock.bind(("127.0.0.1",8354))

sock.listen(1)

server_conn, addr = sock.accept()
server_conn.settimeout(0.1)
client_sock.connect(("localhost",2947))
client_sock.settimeout(1)

side_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
side_sock.bind(("0.0.0.0",7891))
side_sock.settimeout(1)

rfMessage = None
rfObj = dict()

def closeSockets():
    client_sock.close()
    sock.close()
    side_sock.close()
    print "Closing All Sockets"

atexit.register(closeSockets)

while True:
    try:
        gps_data = client_sock.recv(512)
        print "gps_data_len:", len(gps_data)
        known_gps = gps_data
    except Exception as e:
        print "Error with gps data"
        print e
        gps_data = None
        pass

    try:
        rfMessage = side_sock.recv(1024)
        print "Message From RFNEST ",rfMessage
        rfObj = json.loads(rfMessage)
        print "Message From RFNEST",rfObj
    except Exception as e:
        pass

    try:
        if gps_data != None:
            if(bool(rfObj)):
                known_gps = changeLocation(known_gps,rfObj['lat'],rfObj['lon'])
            print known_gps
            print "Start: " , known_gps[0:10]
            if known_gps[0] == "{":
                server_conn.send(known_gps)
        else:
            print "NULL GPS Data"

    except Exception as e:
        print "=========================================================="
        print "Connection to client program lost"
        print e
        server_conn.close()
        sock.listen(1)
        server_conn, addr = sock.accept()
        server_conn.settimeout(0.1)
        known_gps = None
        gps_data = None
        client_sock.close()
        #time.sleep(1)
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect(("localhost",2947))
        client_sock.settimeout(1)

    try:
        data = server_conn.recv(2048)
    except Exception as e:
        data = None
        pass

    if data != None:
        client_sock.send(data)
        print "Info from client: ", data, " len: ",len(data)
