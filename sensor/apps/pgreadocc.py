import psycopg2 as pg
import pandas.io.sql as psql
import numpy as np
import matplotlib.pyplot as plt
import math
import sklearn.preprocessing as sktpre
from scipy import stats
import burst_detection as bd
import time
import threading

class fitness_thread(threading.Thread):

        def __init__(self, threadID, num_channels, num_epochs, data_len):
		threading.Thread.__init__(self)
		self.fit = np.zeros((num_channels,num_epochs))
                self.num_channels = num_channels
                self.num_epochs = num_epochs
                self.data_len = data_len
		#self.ifs	

        def zero_runs(self,a):
                # Create an array that is 1 where a is 0, and pad each end with an extra 0.
                iszero = np.concatenate(([0], np.equal(a, 0).view(np.int8), [0]))
                absdiff = np.abs(np.diff(iszero))
                # Runs start and end where absdiff is 1.
                ranges = np.where(absdiff == 1)[0].reshape(-1, 2)
                lens = []
                for j in range(len(ranges)):
                        lens.append(ranges[j][1] - ranges[j][0])
                return ranges, lens
        
        def run(self):
                connection = pg.connect("dbname = darparem user = wireless password = wireless")

                df = psql.read_sql("select occ, noise_floor, timetag from spectruminfo order by timetag DESC limit "+str(self.data_len), connection)

                tempocc = df['occ'].values
                tempnf = df['noise_floor'].values

                occ = np.vstack([np.array(tempocc[i]) for i in range(len(tempocc))])
                nf = np.vstack([np.array(tempnf[i]) for i in range(len(tempocc))])
                occ = np.dstack(np.split(occ, self.num_epochs, axis = 0))
                cstates = np.zeros((self.num_channels,self.num_epochs))
                self.fit = np.zeros((self.num_channels,self.num_epochs))
                for j in range(self.num_epochs):
                        for i in range(self.num_channels):
                                thr = np.mean(nf[:,i])
                                cstates1 = stats.threshold(occ[:,i,j],threshmax = 35.0/np.fabs(thr), newval=10)
                                cstates2 = stats.threshold(occ[:,i,j],threshmax = 20.0/np.fabs(thr), newval=8)
                                cstates3 = stats.threshold(occ[:,i,j],threshmax = 15.0/np.fabs(thr), newval=5)
                                cstates4 = stats.threshold(occ[:,i,j],threshmax = 10.0/np.fabs(thr), newval=3)
                                cstates5 = stats.threshold(occ[:,i,j],threshmax = 5.0/np.fabs(thr), newval=1)
                                cstates[i,j] = np.mean(np.maximum.reduce([cstates1,cstates2,cstates3,cstates4,cstates5]))
                                occ[:,i,j] = stats.threshold(occ[:,i,j],threshmax = 15.0/np.fabs(thr), newval=1)
                                occ[:,i,j] = stats.threshold(occ[:,i,j],threshmin = 0.9, newval=0)
                                _ , lens = self.zero_runs(occ[:,i,j])
                                #print "weight for channel ",i," in epoch ", j , cstates[i,j]
                                self.fit[i,j] = np.mean(np.asarray(lens))*(10-cstates[i,j])
                #print "Finished thread at", time.time()
                #global fit
                #main.fit = self.fit
                return self.fit
               
def main():
        start_time = time.time()

        num_channels = 16
        num_epochs = 10
        fit_thread = fitness_thread(1,num_channels,num_epochs, 100000)
        fit = fit_thread.run()
        # check if it works in parallel
        for i in range(100):
                print "Loopy loop loop"
        """
        fit = []
        for i in range(10):
                fit_thread = fitness_thread(i,num_channels,num_epochs, 10000)
                fit.append(fit_thread.run())
        #print fit
        """
        print fit
        print time.time() - start_time

if __name__ == "__main__":
        main(),
