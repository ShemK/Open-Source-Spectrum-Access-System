import datetime
import time
t = "2017-Feb-05UTC07:24:510"
time1 = '2017-Feb-05UTC07:24:51'
print t[0:22]
ep = datetime.datetime.strptime(time1,"%Y-%b-%d%Z%H:%M:%S")
print ep
epoch = datetime.datetime.utcfromtimestamp(time.time())
print epoch
print time.mktime(epoch.timetuple())
print time.mktime(ep.timetuple())

def unix_time_millis(dt):
    return (dt - epoch).total_seconds() * 1000.0
