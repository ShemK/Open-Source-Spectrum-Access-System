from Engine import Engine
import math
import numpy as np
import time
import datetime

class ProtoEngine(Engine):

    def __init__(self, dbname, host, user, password, thread_count = 1):
        Engine.__init__(self,dbname, host, user, password, thread_count)
        self.pu_table = dict()

    def start(self):
        Engine.start(self)
        self.sanitize_data()



    ## Temp Decision
    ## Try to get one single value from a batch of values
    def sanitize_data(self):
        for sensor_id, sensor in self.sensor_map.items():
            if sensor.state == "ACTIVE" and sensor.spectrum_info.size > 0:
                spectrum_info = sensor.spectrum_info;
                try:
                    #print spectrum_info.shape
                    #print spectrum_info
                    min_info = spectrum_info.min(axis=0)
                    calculated_info = spectrum_info.quantile(q=0.5,axis=0) #+ #spectrum_info.quantile(q=0.5,axis=0))/2
                    calculated_info = calculated_info.sort_index()
                    min_info = min_info.sort_index()
                    sensor.calculated_info = calculated_info

                    self.calculate_profile(sensor_id,sensor.calculated_info)
                    #print "---------------------------------------------------------------------"
                except Exception as e:
                    print "====================================================================="
                    print "Error with sensor data for sensor ",sensor_id
                    print e
                    #print spectrum_info
                    #print spectrum_info.shape

    def calculate_profile(self,sensor_id,calculated_info):
        #print '----------------- Sensor: ',sensor_id,' -------------------'
        #print calculated_info
        calc_info = calculated_info.to_dict()
        noise_floor = -73.6 - 10*math.log10(4096/2);
        avg_noise_floor = self.sensor_map[sensor_id].averageNoise()
        #print self.sensor_map[sensor_id].avg_noise
        for key in sorted(calc_info.iterkeys()):
            value  = calc_info[key]
            if not math.isnan(value):

                #psd = (value *math.fabs(avg_noise_floor[key])) + avg_noise_floor[key]
                psd = (value *math.fabs(noise_floor)) + noise_floor
                if psd == 0:
                    psd = (value *math.fabs(noise_floor)) + noise_floor - 20
                    #print "$$", key ," : ",avg_noise_floor[key], " / ", noise_floor
                #else:
                    #print key ," : ",avg_noise_floor[key], " = ",psd

                usrp_gain = 30
                nearest_pathloss =  -10 - psd
                near_pathloss = -10 - psd
                farthest_pathloss = -10 - psd
                self.sensor_map[sensor_id].calc_psd[key] = psd
                nearest_dist = (math.pow(10,(nearest_pathloss/20))*3e8)/(4*math.pi*key)
                near_dist = (math.pow(10,(near_pathloss/20))*3e8)/(4*math.pi*key)
                farthest_dist = (math.pow(10,(farthest_pathloss/20))*3e8)/(4*math.pi*key)

                self.sensor_map[sensor_id].update_thresholds(nearest_dist,near_dist,farthest_dist,key)
                #print len(self.potential)
                self.sensor_map[sensor_id].updatePerceivedPU(key,nearest_dist)

                self.updatePossiblePuDistance(sensor_id)

        if len(self.sensor_map[sensor_id].cbsd_pu) > 0:
            for fccId,cbsd in self.sensor_map[sensor_id].cbsd_pu.iteritems():
                if len(cbsd.getPotentialPuFrequencies()) > 0:
                    self.sendPotentialPUInfo(sensor_id,fccId,cbsd.getPotentialPuFrequencies())

    def sendPotentialPUInfo(self,sensor_id,fccId,potential_pu_frequencies):
        self.informationParser.addStatus('type','SAS')
        self.informationParser.addStatus('status','potential_pu')
        self.informationParser.addStatus('sensor_id',sensor_id)
        self.informationParser.addStatus('fccId',sensor_id)
        self.informationParser.addStatus('frequencies',potential_pu_frequencies.keys())
        self.informationParser.sendStatus()


    def updatePossiblePuDistance(self,sensor_id):
        if self.sensor_map[sensor_id].distUpdate:
            for key,value in self.sensor_map[sensor_id].potential.iteritems():
                if key in self.pu_table:
                    self.pu_table[key][sensor_id] = value
                    if len(self.pu_table[key]) > 2:
                        self.calculatePUPosition(key)
                else:
                    self.pu_table[key] = dict()
                    self.pu_table[key][sensor_id] = value

            for key,value in self.pu_table.iteritems():
                if (key not in self.sensor_map[sensor_id].potential) and (sensor_id in self.pu_table[key]):
                    self.pu_table[key].pop(sensor_id,None)
            self.sensor_map[sensor_id].distUpdate = False

    def calculatePUPosition(self, freq):
        print "----------------------------------------------------------"
        print "Sensors Involved: ", self.pu_table[freq].keys()
        x_array = list()
        y_array = list()
        r = list()
        n = 3
        for sensor_id,value in self.pu_table[freq].iteritems():
            x_array.append(self.sensor_map[sensor_id].latitude)
            y_array.append(self.sensor_map[sensor_id].longitude)
            r.append(value)

        R = 6378137
        x = list(x_array)
        y = list(y_array)
        A = np.zeros(shape=(n-1,2))
        for i in range(0,n):
            x_array[i] = math.radians(x_array[i])
            y_array[i] = math.radians(y_array[i])
            x[i] = R*math.cos(x_array[i])*math.cos(y_array[i])
            y[i] = R*math.cos(x_array[i])*math.sin(y_array[i])

        for i in range(0,n-1):
            for j in range(0,2):
                if j == 0:
                    A[i,j] = 2*(x[n-1] - x[i])
                else:
                    A[i,j] = 2*(y[n-1] - y[i])


        #y = [1,2,4]
        b = np.zeros(shape=(n-1,1))

        for i in range(0,n-1):
            b[i,0] = math.pow(r[i],2) - math.pow(r[n-1],2) - math.pow(x[i],2) - math.pow(y[i],2) + math.pow(x[n-1],2) + math.pow(y[n-1],2)

        #https://www3.nd.edu/~cpoellab/teaching/cse40815/Chapter10.pdf

        A = np.asmatrix(A)
        b =  np.asmatrix(b)
        A_ch = (A.getT()*A)
        A_ch = A_ch.getI()
        A_ch = A_ch*A.getT()
        loc = A_ch*b

        #print loc
        lon = math.atan2(loc[1,0],loc[0,0])
        lat = math.acos(loc[0,0]/(R*math.cos(lon)))
        print math.degrees(lat), " ," ,math.degrees(lon)
        self.informationParser.addStatus('type','SAS')
        self.informationParser.addStatus('status','pu_location')
        self.informationParser.addStatus('longitude',math.degrees(lon))
        self.informationParser.addStatus('latitude',math.degrees(lat))
        self.informationParser.addStatus('frequency',freq)
        self.informationParser.addStatus('sensors',self.pu_table[freq].keys())
        self.informationParser.addStatus('distances',self.pu_table[freq].values())
        current_time = time.mktime(datetime.datetime.utcnow().timetuple())
        self.informationParser.addStatus('time',current_time)
        self.informationParser.sendStatus()

    def updateRem(self,sensor_id):
        self.sensor_map[sensor_id].calculated_info


def main():
    proto_engine = ProtoEngine("rem","localhost","wireless","wireless")
    while True:
        proto_engine.start()

if __name__ == '__main__':
    main()
