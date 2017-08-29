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

class Cbsd():

    def __init__(self, df,i):
        self.fccId =  df.loc[i]['fccId']
        self.cbsdCategory = df.loc[i]['cbsdCategory']
        self.userId = df.loc[i]['userId']
        self.cbsdSerialNumber = df.loc[i]['cbsdSerialNumber']
        self.cbsdInfo = df.loc[i]['cbsdInfo']
        self.installationParam = df.loc[i]['installationParam']
        self.cbsdId = df.loc[i]['cbsdId']
        self.json_encoder = json.JSONEncoder()
        self.json_decoder = json.JSONDecoder()
        self.installationParam = self.installationParam.replace("\\n","")
        self.installationParam = self.installationParam.replace("\\","")
        self.installationJson = self.json_decoder.decode(self.installationParam)
        self.latitude = self.installationJson['latitude']
        self.longitude = self.installationJson['longitude']
        #print l
    def get_location(self):
        pass

class REMAnalysis(threading.Thread):

    nearest_sensors = dict()
    def __init__(self,cbsd,conn):
        threading.Thread.__init__(self)
        self.cbsd = cbsd
        self.conn = conn
        self.min_distance = 1000000
        self.stop_thread = False


    def initialize_table(self):
        self.channelInfo = psql.read_sql("select startfreq, occ from " + table_name
                            +" where startfreq > 3570e6 and startfreq < 3600e6", self.conn)
    def run(self):
        while not self.stop_thread:
            self.get_nearest_nodes()
            self.make_decision()
            time.sleep(1)
        pass

    def stop(self):
        self.stop_thread = True

    def get_nearest_nodes(self):
        nodeInfo = psql.read_sql("select * from nodeinfo", self.conn)
        dim = nodeInfo.shape
        row_num = dim[0]
        for i in range(0,row_num):
            dist = self.calculate_distance(nodeInfo,i)
            #print "Distance: ",dist
            if(dist!=None):
                if(dist < self.min_distance):
                    sensor_id = nodeInfo.loc[i]['nodeid']
                    if sensor_id not in self.nearest_sensors:
                        if sensor_id!=1:
                            print "sensor_id not found: ", sensor_id
                            self.nearest_sensors[sensor_id] = sensor(sensor_id,dist,self.conn)
                            self.nearest_sensors[sensor_id].fetch_channel_info()
                    else:
                        print "sensor_id found: ", sensor_id
                        self.nearest_sensors[sensor_id].fetch_channel_info()

    def make_decision(self):
        if len(self.nearest_sensors) > 0:
            keys = self.nearest_sensors.keys()
            for i in range(0,len(self.nearest_sensors)):
                spectrum_info = self.nearest_sensors[keys[i]].spectrum_info;
                try:
                    #spectrum_info = spectrum_info.drop_duplicates(keep='first')
                    min_info = spectrum_info.min(axis=0)
                    calculated_info = spectrum_info.quantile(q=0.5,axis=0)
                    #calculated_info = spectrum_info.max(axis=0)
                    calculated_info = calculated_info.sort_index()
                    min_info = min_info.sort_index()
                    calculated_info = calculated_info.dropna(axis = 0)
                    self.nearest_sensors[keys[i]].calculated_info = calculated_info
                    print calculated_info
                    if(len(self.nearest_sensors[keys[i]].normal_info) < len(calculated_info)):
                        self.nearest_sensors[keys[i]].normal_info = calculated_info

                    elif len(self.nearest_sensors[keys[i]].normal_info) == len(calculated_info):
                        diff = calculated_info - self.nearest_sensors[keys[i]].normal_info
                        if diff.max() > 0.005:
                            above_thresh = diff[(diff > 0.005)]
                            print "Above Normal"
                            print above_thresh.index.tolist()
                        else:
                            self.nearest_sensors[keys[i]].normal_info = calculated_info

                except Exception as e:
                    print "Error with sensor data",e
        else:
            pass

    def organize_data(self):
        if(len(self.nearest_sensors) > 0):
            pass


    def calculate_distance(self,nodeInfo,i):
        #print nodeInfo
        sensor_latitude = nodeInfo.loc[i]['latitude']
        sensor_longitude = nodeInfo.loc[i]['longitude']
        dist = None
        if (sensor_latitude!=None) and (sensor_longitude!=None):
            dist = (self.cbsd.latitude - sensor_latitude)**2 + (self.cbsd.longitude - sensor_latitude)**2
            dist = math.sqrt(dist)
        return dist


class sensor():

    def __init__(self,sensor_id,distance,conn):
        self.sensor_id = sensor_id
        self.distance = distance
        #print "Sensor ID: ", sensor_id
        self.conn = conn
        self.spectrum_info = DataFrame()
        self.normal_info = DataFrame()
        self.calculated_info = DataFrame()
        self.count = 1;
    def fetch_channel_info(self):
        table_name = "channelinfo_" + str(self.sensor_id)
        print "Table Name: ",table_name
        self.channelInfo = psql.read_sql("select startfreq, occ from " + table_name
                        +" where startfreq > 3570e6 and startfreq < 3600e6", self.conn)
        transposed_info = self.channelInfo.transpose()
        cols = None
        info = None
        for row in transposed_info.itertuples():
            if(row[0] == 'startfreq'):
                cols = list(row[1:-1])
            if(row[0] == 'occ'):
                info = list(row[1:-1])

        #current_time = time.mktime(datetime.datetime.utcnow().timetuple())
        current_time  = datetime.datetime.utcnow()
        ind = [current_time]
        temp = DataFrame(info,columns = ind,index = cols)
        temp = temp.transpose()
        print "Current Time: ",current_time
        #print temp
        if self.spectrum_info.size == 0 :
            self.spectrum_info = temp
        else:
            try:
                self.spectrum_info = self.spectrum_info.append(temp, ignore_index = False)
            except Exception as e:
                print "Error Appending:",e

        spectrum_info_shape = self.spectrum_info.shape
        row_num = spectrum_info_shape[0]

        #print row_num
        #print self.spectrum_info
        if(row_num > 20):
            #print self.spectrum_info
            self.spectrum_info = DataFrame()
