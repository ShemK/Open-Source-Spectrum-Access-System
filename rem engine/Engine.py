import psycopg2 as pg
import pandas.io.sql as psql
from pandas import DataFrame
from pandas import concat
import pandas as pd
import numpy as np
import json
import threading
import time
import datetime
import math
import logging

from DbReader import DbReader
from Sensor import Sensor
import information_parser

'''
    Base Class Utilized to get information from the REM
    Can be inherited by derived classed
    Data from REM can be used by the derived class to make decisions
'''

class Engine:
    def __init__(self, dbname, host, user, password, thread_count = 1):
        self.dbname = dbname
        self.host = host
        self.user = user
        self.password = password
        self.dbReader = DbReader(dbname, host, user, password,)
        self.conn = self.dbReader.connect()
        self.thread_count = thread_count
        self.sensorInfo = None
        self.sensor_map = dict()
        self.informationParser =  information_parser.InformationParser('192.168.1.21',9749)
        if(self.conn == None):
            print "Couldn't connect to database"
            sys.exit()

    def start(self):
        nodeinfo = self.get_sensor_info()
        index_list = nodeinfo.index.tolist()
        # Check if it is the first time fetching data
        #print index_list
        if index_list:
            self.sensorInfo = nodeinfo
            self.update_sensors(nodeinfo)


    def update_sensors(self, nodeinfo):
        index_list = nodeinfo.index.tolist()
        for i in index_list:
            sensor_id = np.asscalar(nodeinfo.loc[i]['nodeid'])
            last_active = nodeinfo.loc[i]['last_active']
            latitude  = nodeinfo.loc[i]['latitude']
            longitude  = nodeinfo.loc[i]['longitude']
            if sensor_id not in self.sensor_map:
                self.sensor_map[sensor_id] = Sensor(sensor_id,0,self.dbReader)

            self.update_sensor_info(sensor_id,last_active)

            current_time = time.mktime(datetime.datetime.utcnow().timetuple())
            if (current_time - self.sensor_map[sensor_id].last_loc_update) > 30:
                self.sensor_map[sensor_id].latitude = latitude
                self.sensor_map[sensor_id].longitude = longitude
                self.informationParser.addStatus('type','SAS')
                self.informationParser.addStatus('status','sensor_location')
                self.informationParser.addStatus('sensor_id',sensor_id)
                self.informationParser.addStatus('latitude',latitude)
                self.informationParser.addStatus('longitude',longitude)
                current_time = time.mktime(datetime.datetime.utcnow().timetuple())
                self.informationParser.addStatus('time',current_time)
                self.informationParser.sendStatus()
                self.sensor_map[sensor_id].last_loc_update = current_time

    def update_sensor_info(self,sensor_id,last_active):
        current_time = time.mktime(datetime.datetime.utcnow().timetuple())
        active_time = last_active.to_pydatetime()
        active_time = time.mktime(active_time.timetuple())
        self.sensor_map[sensor_id].last_active = active_time
        self.sensor_map[sensor_id].clock_diff = current_time - active_time
        #print self.sensor_map[sensor_id].clock_diff
        if(self.sensor_map[sensor_id].clock_diff < 15000):
            self.sensor_map[sensor_id].state = "ACTIVE"
            if active_time != self.sensor_map[sensor_id].last_active_time:
                self.sensor_map[sensor_id].fetch_channel_info()
                self.sensor_map[sensor_id].last_active_time = active_time
        else:
            self.sensor_map[sensor_id].state = "PASSIVE"


    def get_sensor_info(self):
        nodeinfo = self.dbReader.fetch_data(None,"nodeinfo")
        # remove nodes with no time stamp
        nodeinfo  = nodeinfo[nodeinfo.last_active.notnull()]
        # get index list and iterate through it
        return nodeinfo

    def get_current_time(self):
        current_time = time.mktime(datetime.datetime.utcnow().timetuple())
        return current_time


def main():
    engine = Engine("rem","localhost","wireless","wireless")
    while True:
        engine.start()

if __name__ == '__main__':
    main()
