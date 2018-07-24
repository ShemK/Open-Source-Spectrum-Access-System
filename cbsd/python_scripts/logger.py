import csv
import os
import time
import datetime

class logger(object):

    def __init__(self,filename, log_path = None,file_type = 'csv'):
        self.file_type = file_type
        if log_path != None:
            dest_log_path =  log_path
            command = "mkdir -p " + dest_log_path
            os.system(command)
            filename = dest_log_path + "/" + filename

        print "Log file: "
        print filename
        if file_type == 'csv':
            self.filename = filename + '.csv'
            self.file_handler = open(self.filename,'w')
            self.writer = csv.writer(self.file_handler )
        else:
            self.filename = filename
            self.file_handler  = open(self.filename,'w')
            self.writer = csv.writer(self.file_handler )

    def write(self,key,value,log = True):
        if log:
            print "Writing to file ======================================",log, "key :", key, " value = ", value
            self.writer.writerow([time.time(),key,value])

    def close():
        self.file_handler.close()
# used to test if it is working
def main():
    Log = logger('H', 'test')
    Log.write('Hello', 'Hi')
    Log.write('Hello2', 'Hi22')

if __name__ == '__main__':
    main()
