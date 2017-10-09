import gps
import os
import time
from gps import *
import threading

#session = gps(verbose=0)
# change verbose to 1 if need more output


class GPSReader(threading.Thread):


    def __init__(self,cbsd=None,v=0):
        threading.Thread.__init__(self)
        self.session = gps(verbose=v)
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
                self.cbsd.add_installation_parameters('longitude',self.location['longitude'])
                self.cbsd.add_installation_parameters('latitude',self.location['latitude'])
            if self.stop == True:
                break
        print "GPS Stopped"

    def stop_gps(self):
        self.stop = True

def main():
    gpsReader = GPSReader()
    gpsReader.start()
if __name__ == '__main__':
    main()
