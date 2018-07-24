from Engine import Engine
import math

class ProtoEngine(Engine):

    def __init__(self, dbname, host, user, password, thread_count = 1):
        Engine.__init__(self,dbname, host, user, password, thread_count)


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
                    calculated_info = spectrum_info.quantile(q=0.75,axis=0)
                    calculated_info = calculated_info.sort_index()
                    min_info = min_info.sort_index()
                    sensor.calculated_info = calculated_info

                    self.calculate_profile(sensor_id,sensor.calculated_info)
                    #print "---------------------------------------------------------------------"
                except Exception as e:
                    #print "====================================================================="
                    print "Error with sensor data for sensor ",sensor_id,e
                    #print spectrum_info
                    #print spectrum_info.shape

    def calculate_profile(self,sensor_id,calculated_info):
        #print '----------------- Sensor: ',sensor_id,' -------------------'
        #print calculated_info
        calc_info = calculated_info.to_dict()
        noise_floor = -73.6 - 10*math.log10(4096/2);
        for key in sorted(calc_info.iterkeys()):
            value  = calc_info[key]
            if not math.isnan(value):

                psd = (value *math.fabs(noise_floor)) + noise_floor
                usrp_gain = 30
                nearest_pathloss =  -15 - psd
                near_pathloss = -15 - psd
                farthest_pathloss = -15- psd

                nearest_dist = (math.pow(10,(nearest_pathloss/20))*3e8)/(4*math.pi*key)
                near_dist = (math.pow(10,(near_pathloss/20))*3e8)/(4*math.pi*key)
                farthest_dist = (math.pow(10,(farthest_pathloss/20))*3e8)/(4*math.pi*key)

                self.sensor_map[sensor_id].update_thresholds(nearest_dist,near_dist,farthest_dist,key)
                #print len(self.potential)
                if key in self.sensor_map[sensor_id].potential:
                    if psd < -90:
                        print key, " lost a potential -------------- dist ",near_dist, " at ",sensor_id
                        self.sensor_map[sensor_id].potential.pop(key,None)

                if key > 800e6 and key < 1000e6 and psd > -90:
                    #print key," : ",value, " : ",psd, " : ",nearest_dist, " : ",farthest_dist
                    if key not in self.sensor_map[sensor_id].potential:
                        #print key," : ",value, " : ",psd, " : ",nearest_dist, " : ",farthest_dist
                        self.sensor_map[sensor_id].potential[key] = nearest_dist
                        print key, " gained a potential ++++++++++++++ dist ",nearest_dist, " at ",sensor_id
                    elif self.sensor_map[sensor_id].potential[key] != nearest_dist:
                        self.sensor_map[sensor_id].potential[key] = nearest_dist
                        print key, " Updated Distance: ", nearest_dist , " at ", sensor_id



    def updatePossiblePuDistance(self,sensor_id, frequency, nearest_dist, farthest_dist):
        pass

    def updateRem(self,sensor_id):
        self.sensor_map[sensor_id].calculated_info


def main():
    proto_engine = ProtoEngine("rem","localhost","wireless","wireless")
    while True:
        proto_engine.start()

if __name__ == '__main__':
    main()
