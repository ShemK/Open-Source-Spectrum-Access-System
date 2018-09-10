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
        self.potential = dict()
        self.cbsd_pu = dict()
        self.longitude = 0
        self.latitude = 0
        self.distUpdate = False
        self.last_loc_update = 0
        self.calc_psd = dict()
        self.spectrum_noise = DataFrame()
        self.avg_noise = dict()
        #self.fetch_pu_info()

    def fetch_channel_info(self):
        #print "Table Name: ",self.table_name
        conditions = {'startfreq': ' > 800e6 AND "startfreq" < 1000e6'}
        self.channelInfo = self.dbReader.fetch_data(['startfreq','occ','noise_floor'],self.table_name,conditions,'ORDER BY startfreq')
        if self.channelInfo.size > 0:
            transposed_info = self.channelInfo.transpose()
            cols = None
            info = None
            noise_floor = None
            for row in transposed_info.itertuples():
                if(row[0] == 'startfreq'):
                    cols = list(row[1:-1])
                if(row[0] == 'occ'):
                    info = list(row[1:-1])
                if(row[0]) == 'noise_floor':
                    noise_floor = list(row[1:-1])

            #current_time = time.mktime(datetime.datetime.utcnow().timetuple())
            current_time  = datetime.datetime.utcnow()
            ind = [current_time]
            temp = DataFrame(info,columns = ind,index = cols)
            temp = temp.transpose()
            temp_noise = DataFrame(noise_floor,columns = ind,index = cols)
            temp_noise = temp_noise.transpose()
            #print "Current Time: ",current_time
            #print temp_noise
            if self.spectrum_info.size == 0 :
                self.spectrum_info = temp
                self.spectrum_noise = temp_noise
            else:
                try:
                    self.spectrum_info = self.spectrum_info.append(temp, ignore_index = False)
                    self.spectrum_noise = self.spectrum_noise.append(temp_noise, ignore_index = False)
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

            #print self.spectrum_info
            if(row_num > 3):
                #print self.spectrum_info
                ind_to_drop = self.spectrum_info.axes[0][0]
                self.spectrum_info = self.spectrum_info.drop(ind_to_drop)
                ind_to_drop = self.spectrum_noise.axes[0][0]
                self.spectrum_noise = self.spectrum_noise.drop(ind_to_drop)
            self.fetch_pu_info()

    def update_thresholds(self,nearest,near,furthest,startfreq):
        table_name = self.table_name + '_pu';
        input_data = {'nearest':nearest,'near':near,'furthest':furthest}
        self.dbReader.update_data(input_data,table_name,{'startfreq':startfreq})

    def fetch_pu_info(self):
        query = 'SELECT' + " pu_frequencies" + ',' +'registered_cbsds."fccId"' + ', last_active, distance'+' FROM sensorcbsdconnection INNER JOIN registered_cbsds ON sensorcbsdconnection."fccId" = registered_cbsds."fccId" WHERE  "nodeid"  = ' + str(self.sensor_id)
        self.potential_pu = self.dbReader.fetchQuery(query)
        self.potential_pu  = self.potential_pu[self.potential_pu.last_active.notnull()]
        #self.potential_pu = self.potential_pu[self.potential_pu.pu_frequencies.notnull()]
        index_list = self.potential_pu.index.tolist()

        for i in index_list:
            fccId = self.potential_pu.loc[i]['fccId']
            if fccId not in self.cbsd_pu:
                self.cbsd_pu[fccId] = CBSD(fccId,self.potential_pu.loc[i]['distance'],self.dbReader,self.sensor_id)

            self.cbsd_pu[fccId].setLastActive(np.asscalar(self.potential_pu.loc[i]['last_active']))
            self.cbsd_pu[fccId].set_distance(self.potential_pu.loc[i]['distance'])
            #print np.asscalar(self.potential_pu.loc[i]['last_active'])

            freq_list = np.asarray(self.potential_pu.loc[i]['pu_frequencies'])
            freq_list = freq_list.tolist()
            self.cbsd_pu[fccId].addPotentialPuFrequencies(freq_list)

    def updatePerceivedPU(self,key,value):
        check =  self.spectrum_info.to_dict()
        check2 = self.calculated_info.to_dict()
        if key in self.potential:
            #print key, " : ", check[key]
            #print "Chosen: ", check2[key]
            pass
        if value > 200 and key in self.potential:
            print key, " lost a potential -------------- dist ",value, " at ", self.sensor_id, " psd = ", self.calc_psd[key]
            #print check[key]
            self.potential.pop(key,None)
            self.distUpdate = True
        elif key not in self.potential and value < 200:
            self.potential[key] = value
            #print check[key]
            print key, " gained a potential ++++++++++++++ dist ",value, " at ", self.sensor_id, " psd = ", self.calc_psd[key]
            self.distUpdate = True
        elif key in self.potential and self.potential[key] != value:
            self.potential[key] = value
            #print check[key]
            print key, " Updated Distance: ", value , " at ", self.sensor_id, " psd = ", self.calc_psd[key]
            self.distUpdate = True

    def averageNoise(self):
        noise_dict = self.spectrum_noise.to_dict()
        if len(noise_dict) > 0:
            for freq,value in noise_dict.iteritems():
                if len(noise_dict[freq]) > 0:
                    self.avg_noise[freq] = 0
                    for time_stamp, noise_dbm in noise_dict[freq].iteritems():
                        self.avg_noise[freq] = self.avg_noise[freq] + self.dbm_to_mw(noise_dbm)
                    self.avg_noise[freq] = self.avg_noise[freq]/len(noise_dict[freq])
                    self.avg_noise[freq] = self.mw_to_dbm(self.avg_noise[freq])
        return self.avg_noise
    def mw_to_dbm(self,mW):
        return 10.*math.log10(mW)
    def dbm_to_mw(self,dBm):
        return 10**((dBm)/10.)

    def test(self):
        pass
print
class CBSD():
    def __init__(self,fcc_id,distance,dbReader,sensor_id = None):
        self.fcc_id = fcc_id
        self.distance = distance
        self.dbReader = dbReader
        self.pu_frequencies = dict()
        self.last_active = 0
        self.sensor_id = sensor_id
        self.active = True

    def addPotentialPu(self,frequency):
        self.pu_frequencies[frequency] = True

    def addPotentialPuFrequencies(self,freq_list):
        #print self.active = True
        if freq_list == None and self.active == False:
            for key in self.pu_frequencies.copy():
                if self.pu_frequencies[key] == False:
                    self.pu_frequencies.pop(key,None)
                else:
                    self.pu_frequencies[key] = False
            return

        for i in range(len(freq_list)):
            self.addPotentialPu(freq_list[i])

        key_list = self.pu_frequencies.keys()
        for key in key_list:
            if key not in freq_list:
                if self.pu_frequencies[key] == False:
                    self.pu_frequencies.pop(key,None)
                else:
                    self.pu_frequencies[key] = False
        if len(self.pu_frequencies) > 0:
            print self.pu_frequencies
            pass

    def getPotentialPuFrequencies(self):
        return self.pu_frequencies

    def setLastActive(self,last_active):
        self.last_active = last_active
        if abs(self.last_active - self.get_current_time()) > 15000:
            self.active = False
        else:
            self.active = True

    def get_current_time(self):
        current_time = time.mktime(datetime.datetime.utcnow().timetuple())
        return current_time

    def set_distance(self, distance):
        self.distance = distance
