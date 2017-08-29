import psycopg2 as pg
import pandas.io.sql as psql 
import numpy as np
import matplotlib.pyplot as plt
import math 
import sklearn.preprocessing as sktpre
from scipy import stats
import burst_detection as bd 


def zero_runs(a):
        # Create an array that is 1 where a is 0, and pad each end with an extra 0.
        iszero = np.concatenate(([0], np.equal(a, 0).view(np.int8), [0]))
        absdiff = np.abs(np.diff(iszero))
        # Runs start and end where absdiff is 1.
        ranges = np.where(absdiff == 1)[0].reshape(-1, 2)
        return ranges

def main():
        connection = pg.connect("dbname = rem user = wireless password = wireless")

        df = psql.read_sql("select occ, noise_floor, timetag from spectruminfo order by timetag DESC LIMIT 1000", connection)

        tempocc = df['occ'].values
        tempnf = df['noise_floor'].values


        occ = np.zeros((df.shape[0],16))
        nf = np.zeros((df.shape[0],16))


        for i in range (0, len(occ)-1):
                occ[i,:] = np.copy(np.array(tempocc[i]))
                nf[i,:] = np.copy(np.array(tempnf[i]))

        


        fitness = np.zeros((16,1))
        plt.subplot(411)
        for i in range(195,210):
                plt.plot(occ[i,:])
        
        plt.subplot(412)
        plt.plot(occ[:,6])
        #plt.plot(occ[:,6])
        plt.subplot(413)
        plt.plot(occ[:,13])
        
        plt.subplot(414)
        plt.plot(occ[:,12])

        for i in range(0,16):
                thr = np.mean(nf[:,i])
                print 10.0/np.fabs(thr)
                #print np.mean(occ[:,i])
                occ[:,i] = stats.threshold(occ[:,i],threshmax = 10.0/np.fabs(thr), newval=1)
                occ[:,i] = stats.threshold(occ[:,i],threshmin = 0.9, newval=0)

        
        plt.subplot(413)
        plt.plot(occ[:,13])

        plt.subplot(412)
        plt.plot(occ[:,8])

        plt.subplot(414)
        plt.plot(occ[:,1])

        print bd.enumerate_bursts(occ[:,8], 'burstLabel')
        # print zero_runs(occ[:,8])
        #plt.hist(np.histogram(occ[:,0]), bins = [0, 1])
        

        plt.show()


if __name__ == "__main__":
        main()