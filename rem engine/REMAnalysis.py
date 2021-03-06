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

def print_full(x):
    pd.set_option('display.max_columns', len(x))
    print(x)
    pd.reset_option('display.max_columns')

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
        self.min_distance = 200
        self.stop_thread = False
        self.unavailable_frequencies = []
        self.neighbors = []
        self.occupied_frequencies = 0

    def update_cbsd_info(self):
            logging.debug("Fetching Updated CBSD information for %s",self.cbsd.fccId)
            try:
                df = psql.read_sql("select * from registered_cbsds", self.conn)
                dim = df.shape
                row_num = dim[0]
                for i in range(0,row_num):
                    if df.loc[i]['fccId'] == self.cbsd.fccId:
                        self.cbsd.cbsdId = df.loc[i]['cbsdId']

                sql_query = 'select * from cbsdinfo_'+ self.cbsd.fccId+' WHERE "available" = 0;'
                df = psql.read_sql(sql_query, self.conn)

                dim = df.shape
                row_num = dim[0]
                print "Row Num ",row_num
                for i in range(0,row_num):
                    lowFrequency = df.loc[i]['lowFrequency']
                    if self.occupied_frequencies==0:
                        self.update_neighbors(self.occupied_frequencies,1)
                    if self.occupied_frequencies != lowFrequency:
                        self.update_neighbors(self.occupied_frequencies,1)

                    self.occupied_frequencies = lowFrequency
                    self.update_neighbors(lowFrequency,2)
                    print "occupied for ",self.cbsd.fccId,": ",self.occupied_frequencies
                if row_num == 0 and self.occupied_frequencies != 0:
                    self.update_neighbors(self.occupied_frequencies,1)
                    self.occupied_frequencies = 0

            except Exception as e:
                print "Error reading registered_cbsds: ",e

    def update_table(self, lowFrequency, value):
        try:
            sql_query = 'UPDATE cbsdinfo_'+self.cbsd.cbsdId + ' SET available = '+ str(value) + \
                                        ' WHERE "lowFrequency" = ' + str(lowFrequency) + ';'
            cur = self.conn.cursor()
            cur.execute(sql_query)
            self.conn.commit()
            print sql_query
        except Exception as e:
            print "Error Updating DB: ",e


    def run(self):
        while not self.stop_thread:
            self.update_cbsd_info()
            self.get_nearest_nodes()
            if len(self.nearest_sensors) > 0:
                pass
                self.make_decision()
            time.sleep(1)
        pass

    def stop(self):
        self.stop_thread = True

    def get_nearest_nodes(self):
        logging.debug("Fetching info about nearest sensors for %s",self.cbsd.fccId)
        nodeInfo = psql.read_sql("select * from nodeinfo", self.conn)
        # remove nodes with no time stamp
        nodeInfo = nodeInfo[nodeInfo.last_active.notnull()]
        # get index list and iterate through it
        index_list = nodeInfo.index.tolist()
        for i in index_list:
            dist = self.calculate_distance(nodeInfo,i)
            last_active = nodeInfo.loc[i]['last_active']
            current_time = time.mktime(datetime.datetime.utcnow().timetuple())
            active_time = last_active.to_pydatetime()
            active_time = time.mktime(active_time.timetuple())

            if(dist!=None):
                if(dist < self.min_distance):
                    sensor_id = np.asscalar(nodeInfo.loc[i]['nodeid'])
                    logging.info("")
                    if sensor_id not in self.nearest_sensors:
                        if sensor_id!=1:
                            logging.info("sensor_id not found: %d", sensor_id)
                            self.nearest_sensors[sensor_id] = sensor(sensor_id,dist,self.conn)
                            self.nearest_sensors[sensor_id].last_active = active_time
                            self.nearest_sensors[sensor_id].clock_diff = current_time - active_time
                            self.nearest_sensors[sensor_id].fetch_channel_info()
                            self.nearest_sensors[sensor_id].state = "ACTIVE"
                    else:
                        logging.info("sensor_id found: %d", sensor_id)
                        inactivity = current_time - active_time - self.nearest_sensors[sensor_id].clock_diff
                        if inactivity < 3:
                            logging.info("Sensor State: ACTIVE")
                            self.nearest_sensors[sensor_id].state = "ACTIVE"
                            self.nearest_sensors[sensor_id].fetch_channel_info()
                        else:
                            logging.info("Sensor State: PASSIVE")
                            self.nearest_sensors[sensor_id].state = "PASSIVE"
                            #reset data

                            self.nearest_sensors[sensor_id].spectrum_info = DataFrame()

        if len(self.nearest_sensors) == 0:
            logging.info("No sensor found near withing a distance of %d for cbsd %s", self.min_distance, self.cbsd.fccId)

    def make_decision(self):
        if len(self.nearest_sensors) > 0:
            keys = self.nearest_sensors.keys()
            for i in range(0,len(self.nearest_sensors)):
            #print type(self.nearest_sensors)
            #for sensor_id,sensor in self.nearest_sensors:
                sensor_id = keys[i]
                sensor = self.nearest_sensors[sensor_id];
                spectrum_info = sensor.spectrum_info;
                if sensor.state == "ACTIVE":
                    try:
                        #spectrum_info = spectrum_info.drop_duplicates(keep='first')
                        min_info = spectrum_info.min(axis=0)
                        calculated_info = spectrum_info.quantile(q=0.75,axis=0)
                        #calculated_info = spectrum_info.max(axis=0)
                        calculated_info = calculated_info.sort_index()
                        min_info = min_info.sort_index()
                        #calculated_info = calculated_info.dropna(axis = 0)
                        #min_info = min_info.dropna(axis=0)
                        sensor.calculated_info = calculated_info
                        #logging.debug("Current Sensor Info from %s : %s",sensor_id, calculated_info)
                        #print calculated_info
                        print "Min: ", len(sensor.normal_info)
                        print "quantile: ", len(calculated_info)
                        #print spectrum_info
                        if(len(sensor.normal_info) == 0):
                            sensor.normal_info = calculated_info

                        elif len(sensor.normal_info) == len(calculated_info):
                            diff = calculated_info - sensor.normal_info

                            sensor.check_availability(self.cbsd,diff)

                            print "Diff: ", diff.max()


                            if diff.max() > 0.2:
                                above_thresh = diff[(diff > 0.2)]
                                logging.info("Found channels with interference")
                                #print_full(spectrum_info)

                                #logging.info("Found channels oci")
                                #print calculated_info

                                unavailable_temp = above_thresh.index.tolist()
                                for m in range(0,len(unavailable_temp)):
                                    print "--------------",unavailable_temp[m],"------------"
                                    print spectrum_info[unavailable_temp[m]]

                                # TODO: Need to append to list
                                for p in range(0,len(unavailable_temp)):
                                    if len(sensor.unavailable_frequencies) == 0:
                                        sensor.unavailable_frequencies.append(unavailable_temp[p])
                                    else:
                                        if unavailable_temp[p] not in sensor.unavailable_frequencies:
                                            sensor.unavailable_frequencies.append(unavailable_temp[p])

                                logging.info("%s",str(sensor.unavailable_frequencies))
                                for k in range(0,len(sensor.unavailable_frequencies)):
                                    lowFrequency = float(sensor.unavailable_frequencies[k])
                                    print "Frequency occupied ",sensor_id," for now: ",lowFrequency
                                    lowFrequency = math.floor(float(lowFrequency)/10e6)*10e6
                                    sensor.update_table(self.cbsd,lowFrequency,0)
                            else:
                                if diff.max() < 0.05: # for sus that might make it normal
                                    sensor.normal_info = calculated_info
                            print "unavailable channels: ",sensor.unavailable_frequencies
                        elif len(sensor.normal_info) < len(calculated_info):
                            for ii in range(0, len(calculated_info) - len(sensor.normal_info)):
                                ## Try to add any extra minimum values
                                ## This is due to the varying nature of data read from DB
                                ind = calculated_info.index.tolist()[-1*(ii+1)]
                                s1 = pd.Series([calculated_info.get(ind)],index=[ind])
                                sensor.normal_info = sensor.normal_info.append(s1)
                        elif len(sensor.normal_info) > len(calculated_info):
                            ## Try to pop any extra minimum values
                            for ii in range(0,  len(sensor.normal_info) - len(calculated_info)):
                                ind = sensor.normal_info.index.tolist()[-1*(ii+1)]
                                sensor.normal_info.pop(ind)
                    except Exception as e:
                        print "Error with sensor data for sensor ",sensor_id,e
        else:
            pass

    def check_availability(self,diff):
        for i in range(0,len(self.unavailable_frequencies)):
            if diff[self.unavailable_frequencies[i]] < 0.005:
                lowFrequency = float(self.unavailable_frequencies[i])
                print "Frequency", lowFrequency
                lowFrequency = math.floor(float(lowFrequency)/10e6)*10e6
                self.update_table(lowFrequency,1)
                self.unavailable_frequencies.pop(i)

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
            #print "---------------------------------Distance: ", dist
        return dist

    def calculate_neigbor_distance(self,new_cbsd):
        if (new_cbsd.latitude!=None) and (new_cbsd.longitude!=None):
            dist = (self.cbsd.latitude - new_cbsd.latitude)**2 + (self.cbsd.longitude - new_cbsd.latitude)**2
            dist = math.sqrt(dist)
        return dist
    def update_neighbors(self,lowFrequency,occupied):
        for neighbor in self.neighbors:
            neighbor.update_from_neighbors(lowFrequency,occupied)

    def update_from_neighbors(self,lowFrequency,occupied):
        try:
            sql_query = 'UPDATE cbsdinfo_'+self.cbsd.fccId + ' SET available = '+ str(occupied) + \
                                        ' WHERE "lowFrequency" = ' + str(lowFrequency) + ';'
            cur = self.conn.cursor()
            cur.execute(sql_query)
            self.conn.commit()
            print sql_query
        except Exception as e:
            print "Error Updating neighbors"

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
        self.state = "PASSIVE"
        self.last_active = 0
        self.clock_diff = 0
        self.unavailable_frequencies = []
        self.occupied_frequencies = []
    def fetch_channel_info(self):
        table_name = "channelinfo_" + str(self.sensor_id)
        #print "Table Name: ",table_name
        try:
            self.channelInfo = psql.read_sql("select startfreq, occ from " + table_name
                            +" where startfreq > 800e6 and startfreq < 1000e6", self.conn)
        except Exception as e:
            print "Error reading from database: ",e

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

        spectrum_info_shape = self.spectrum_info.shape
        row_num = spectrum_info_shape[0]

        #print row_num
        #print self.spectrum_info
        if(row_num > 10):
            #print self.spectrum_info
            self.spectrum_info = DataFrame()
            self.spectrum_info = temp

    def check_availability(self,cbsd, diff):
        for i in range(0,len(self.unavailable_frequencies)):
            if diff[self.unavailable_frequencies[i]] < 0.005:
                lowFrequency = float(self.unavailable_frequencies[i])
                print "Frequency channel free at ",self.sensor_id,"for now: ", lowFrequency
                lowFrequency = math.floor(float(lowFrequency)/10e6)*10e6
                self.update_table(cbsd,lowFrequency,1)
                self.unavailable_frequencies.pop(i)

    def update_table(self, cbsd, lowFrequency, value):
        try:
            sql_query = 'UPDATE cbsdinfo_'+cbsd.fccId + ' SET pu_absent = '+ str(value) + \
                                        ' WHERE "lowFrequency" = ' + str(lowFrequency) + ';'
            cur = self.conn.cursor()
            cur.execute(sql_query)
            self.conn.commit()
            print sql_query
        except Exception as e:
            raise
