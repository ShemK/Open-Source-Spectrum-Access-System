import psycopg2 as pg
import pandas.io.sql as psql
from pandas import DataFrame
import numpy as np
import json
import signal,os
import time
import REMAnalysis as rem
import math
import logging

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
            try:
                df = psql.read_sql("select * from registered_cbsds", self.conn)
                logging.info("Reading Database for Available CBSDS")
                if df.equals(self.cbsd_dataframe):
                    #do nothing if no new cbsd is found
                    logging.info("No new CBSDs")
                    pass
                else:

                    if(self.cbsd_dataframe == None):
                        logging.info("Getting CBSDs for the first time")
                        self.cbsd_dataframe = df
                        dim = df.shape
                        row_num = dim[0]
                        for i in range(0,row_num):
                            self.create_cbsd_threads(df,i)
                    else:
                        # search for the new cbsd
                        logging.info("New CBSD found! Creating Thread")
                        self.cbsd_dataframe = df
                        dim = df.shape
                        row_num = dim[0]
                        for i in range(0,row_num):
                            temp_cbsd = rem.Cbsd(df,i)
                            if temp_cbsd.fccId not in self.analysis_threads:
                                create_cbsd_threads(df,i)
            except Exception as e:
                logging.error("error reading the cbsds %s",e)
                self.stop_threads()
            time.sleep(10)


    def create_cbsd_threads(self,df,i):
        self.cbsd = rem.Cbsd(df,i)
        cbsd_thread = rem.REMAnalysis(self.cbsd,self.conn)
        self.analysis_threads[self.cbsd.fccId] = cbsd_thread
        self.analysis_threads[self.cbsd.fccId].start()

    def stop_program(self,signum,frame):
        self.stop_threads()
        self.stop = True

    def stop_threads(self):
        for key,cbsd_thread in self.analysis_threads.items():
            logging.debug("Stopping thread for %s",key)
            cbsd_thread.stop()


def main():

    sql = ServerPsql("rem","localhost","wireless","wireless")
    sql.connect()
    sql.get_current_cbsd_info()


if __name__ == '__main__':
    logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.INFO)
    main()

#data = [1,2,3,4,5]
#df2 = DataFrame(data,columns=['a', 'b', 'c', 'd', 'e'])
#print df2