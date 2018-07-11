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
import DbReader

class Sensor():

    def __init__(self,sensor_id,distance,dbReader):
        self.sensor_id = sensor_id
        self.distance = distance
        #print "Sensor ID: ", sensor_id
        self.dbReader = dbReader
        self.spectrum_info = DataFrame()
        self.normal_info = DataFrame()
        self.calculated_info = DataFrame()
        self.count = 1;
        self.state = "PASSIVE"
        self.last_active = 0
        self.clock_diff = 0
        self.unavailable_frequencies = []
        self.occupied_frequencies = []
        self.table_name = "channelinfo_" + str(self.sensor_id);
        self.channelInfo = DataFrame()
        self.log_count = 0;
        self.last_active_time = 0;
        self.potential = []

    def fetch_channel_info(self):
        #print "Table Name: ",table_name
        conditions = {'startfreq': ' > 800e6 AND "startfreq" < 1000e6'}
        self.channelInfo = self.dbReader.fetch_data(['startfreq','occ'],self.table_name,conditions,'ORDER BY startfreq')
        if self.channelInfo.size > 0:
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
            #print "Current Time: ",current_time
            #print temp
            if self.spectrum_info.size == 0 :
                self.spectrum_info = temp
            else:
                try:
                    self.spectrum_info = self.spectrum_info.append(temp, ignore_index = False)
                except Exception as e:
                    print "Error Appending:",e

            file_name = self.table_name + '.csv'
            if self.log_count == 0:
                temp.to_csv(file_name,mode='w',sep = '\t')
                self.log_count = 1
            else:
                temp.to_csv(file_name,mode='a',header = False,sep = '\t')


            spectrum_info_shape = self.spectrum_info.shape
            row_num = spectrum_info_shape[0]


            if(row_num > 5):
                #print self.spectrum_info
                ind_to_drop = self.spectrum_info.axes[0][0]
                self.spectrum_info = self.spectrum_info.drop(ind_to_drop)


    def update_thresholds(self,nearest,near,furthest,startfreq):
        table_name = self.table_name + '_pu';
        input_data = {'nearest':nearest,'near':near,'furthest':furthest}
        self.dbReader.update_data(input_data,table_name,{'startfreq':startfreq})

    def test(self):
            pass
