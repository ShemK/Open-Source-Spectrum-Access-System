import psycopg2 as pg
import pandas.io.sql as psql
from pandas import DataFrame
import numpy as np
import json
import signal,os
import time
import REMAnalysis as rem
import math


class ServerPsql(object):
    """docstring fo ServerPsql."""
    cbsd_dataframe = None
    conn = None
    cbsd = None
    analysis_threads = dict()
    def __init__(self, dbname, host, user, password):
        self.dbname = dbname
        self.host = host
        self.user = user
        self.password = password
        signal.signal(signal.SIGINT, self.stop_program)
        signal.signal(signal.SIGTERM, self.stop_program)
        self.stop = False

    def connect(self):
        try:
            self.conn =  pg.connect(dbname = self.dbname ,host = self.host , user = self.user , password = self.password)
        except Exception as e:
            self.conn = None
            raise

    def get_current_cbsd_info(self):

        while not self.stop:
            df = psql.read_sql("select * from registered_cbsds", self.conn)

            if df.equals(self.cbsd_dataframe):
            #do nothing if no new cbsd is found
                pass
            else:

                if(self.cbsd_dataframe == None):
                    self.cbsd_dataframe = df
                    dim = df.shape
                    row_num = dim[0]
                    for i in range(0,row_num):
                        self.create_cbsd_threads(df,i)
                else:
                # search for the
                    pass
                    #print df

            time.sleep(10)


    def create_cbsd_threads(self,df,i):
        self.cbsd = rem.Cbsd(df,i)
        cbsd_thread = rem.REMAnalysis(self.cbsd,self.conn)
        self.analysis_threads[self.cbsd.cbsdId] = cbsd_thread
        self.analysis_threads[self.cbsd.cbsdId].start()

    def stop_program(self,signum,frame):
        self.stop_threads()
        self.stop = True

    def stop_threads(self):
        keys = self.analysis_threads.keys()
        for i in range(0,len(self.analysis_threads)):
            print "key: ", keys[i]
            self.analysis_threads[keys[i]].stop()

sql = ServerPsql("rem","localhost","wireless","wireless")
sql.connect()
sql.get_current_cbsd_info()
#data = [1,2,3,4,5]
#df2 = DataFrame(data,columns=['a', 'b', 'c', 'd', 'e'])
#print df2
