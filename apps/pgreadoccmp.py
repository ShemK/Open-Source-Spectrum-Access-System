import psycopg2 as pg
import pandas.io.sql as psql
import numpy as np
import math
from scipy import stats
import time
import threading
from multiprocessing import Process, Queue

# Need to import multiprocessing, threading and the rest of the imported modules

def zero_runs(a):
    # Create an array that is 1 where a is 0, and pad each end with an extra 0.
    iszero = np.concatenate(([0], np.equal(a, 0).view(np.int8), [0]))
    absdiff = np.abs(np.diff(iszero))
    # Runs start and end where absdiff is 1.
    ranges = np.where(absdiff == 1)[0].reshape(-1, 2)
    lens = []
    for j in range(len(ranges)):
        lens.append(ranges[j][1] - ranges[j][0])
    return ranges, lens
        
def run(q,num_channels, num_epochs, data_len):
    connection = pg.connect("dbname = darparem user = wireless password = wireless")
    query = "select occ, noise_floor, timetag from spectruminfo order by timetag DESC limit "+str(data_len)+";"
    df = psql.read_sql(query, connection)

    tempocc = df['occ'].values
    tempnf = df['noise_floor'].values

    occ = np.vstack([np.array(tempocc[i]) for i in range(len(tempocc))])
    nf = np.vstack([np.array(tempnf[i]) for i in range(len(tempocc))])
    occ = np.dstack(np.split(occ, num_epochs, axis = 0))
    cstates = np.zeros((num_channels,num_epochs))
    fit = np.zeros((num_channels,num_epochs))
    for j in range(num_epochs):
        for i in range(num_channels):
            thr = np.mean(nf[:,i])
            cstates1 = stats.threshold(occ[:,i,j],threshmax = 35.0/np.fabs(thr), newval=10)
            cstates2 = stats.threshold(occ[:,i,j],threshmax = 20.0/np.fabs(thr), newval=8)
            cstates3 = stats.threshold(occ[:,i,j],threshmax = 15.0/np.fabs(thr), newval=5)
            cstates4 = stats.threshold(occ[:,i,j],threshmax = 10.0/np.fabs(thr), newval=3)
            cstates5 = stats.threshold(occ[:,i,j],threshmax = 5.0/np.fabs(thr), newval=1)
            cstates[i,j] = np.nanmean(np.maximum.reduce([cstates1,cstates2,cstates3,cstates4,cstates5]))
            occ[:,i,j] = stats.threshold(occ[:,i,j],threshmax = 15.0/np.fabs(thr), newval=1)
            occ[:,i,j] = stats.threshold(occ[:,i,j],threshmin = 0.9, newval=0)
            _ , lens = zero_runs(occ[:,i,j])
            #print "weight for channel ",i," in epoch ", j , cstates[i,j]
            fit[i,j] = np.nanmean(np.asarray(lens))*(10-cstates[i,j])
            
    q.put(fit)

def loop():
    for i in range(100):
        print time.time()


def main():
    q = Queue()
    start_time = time.time()
    data_len = 10000
    num_channels = 16
    num_epochs = 10
    p = Process(target = run, args = (q,num_channels,num_epochs,data_len,))
    l = Process(target = loop)
    p.start()
    #l.start()
    print q.get()

    print time.time() - start_time

if __name__ == "__main__":
        main()
