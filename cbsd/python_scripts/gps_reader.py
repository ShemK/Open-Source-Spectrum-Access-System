import gps
import os
import time
from gps import *
import threading

#session = gps(verbose=0)
# change verbose to 1 if need more output


class GPSReader(threading.Thread):


    def __init__(self,cbsd=None,port=2947,v=0):
        threading.Thread.__init__(self)
        self.port = port
        self.session = gps(port=self.port,verbose=v)
        self.session.stream(WATCH_ENABLE|WATCH_NEWSTYLE)
        self.location = dict()
        self.cbsd = cbsd
        self.stop = False

    def run(self):
        for report in self.session:
            #session.read()
            #print "start----------------------------------"
            #print report
            location  = report.get('lon')
            if location != None:
                self.location['longitude'] = report.get('lon')
                self.location['latitude'] = report.get('lat')
                if self.cbsd!=None:
                    self.cbsd.add_installation_parameters('longitude',self.location['longitude'])
                    self.cbsd.add_installation_parameters('latitude',self.location['latitude'])
                    self.cbsd.update_location(self.location)
            if self.stop == True:
                break
        print "GPS Stopped"

    def stop_gps(self):
        self.stop = True

def main():
    gpsReader = GPSReader(port=8354,v = 1)
    gpsReader.start()
    gpsReader.join()
if __name__ == '__main__':
    main()
