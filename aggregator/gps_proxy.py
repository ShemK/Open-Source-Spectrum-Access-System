#!/usr/bin/env python

import socket
import json
import atexit
from decimal import *

def changeLocation(input_data,lat,lon):
    pos = input_data.find("lon")
    class_pos = input_data.find("class")
    speed_pos = input_data.find("speed")
    if pos != -1 and speed_pos != -1 and class_pos != -1:
        input_data = input_data[1:-3]
        input_list = input_data.split(',')
        lat_list = input_list[5].split(':')
        lat_list[1] = str(lat)
        input_list[5] = ':'.join(lat_list)
        lon_list = input_list[6].split(':')
        lon_list[1] = str(lon)
        input_list[6] = ':'.join(lon_list)
        input_data = ','.join(input_list)
        input_data = '{'+input_data+'}\r\n'
        return input_data
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
client_sock.settimeout(0.1)

side_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
side_sock.bind(("0.0.0.0",7891))
side_sock.settimeout(0.1)

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
        gps_data = None
        pass

    try:
        rfMessage = side_sock.recv(1024)
        print "Message From RFNEST",rfMessage
        rfObj = json.loads(rfMessage)
        print "Message From RFNEST",new_data
    except Exception as e:
        pass

    try:
        if gps_data != None:
            if(bool(rfObj)):
                known_gps = changeLocation(known_gps,rfObj['lat'],rfObj['lon'])
            print known_gps
            server_conn.send(known_gps)



    except Exception as e:
        print e

        sock.listen(1)
        server_conn, addr = sock.accept()
        server_conn.settimeout(0.1)
        known_gps = None
        gps_data = None
        client_sock.close()
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect(("localhost",2947))
        client_sock.settimeout(0.1)

    try:
        data = server_conn.recv(2048)
    except Exception as e:
        data = None
        pass

    if data != None:
        client_sock.send(data)
        print "Info from client: ", data
