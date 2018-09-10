#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
# Copyright 2017 <+YOU OR YOUR COMPANY+>.
# 
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

import numpy as np
from gnuradio import gr
from scipy import stats


class ks_test(gr.sync_block):
    """
    docstring for block ks_test
    """
    def __init__(self, N):
        self.lin = np.linspace(0,N-1,num=N, dtype=int)
        self.N = N
        self.buf = np.zeros(shape = (N,), dtype=np.complex64)
        self.ctr=0
        gr.sync_block.__init__(self,
            name="ks_test",
            in_sig=[(np.complex64   , self.N),(np.complex64   , self.N)],
            out_sig=[(np.complex64  , self.N)])
        

    def work(self, input_items, output_items):
        in0 = input_items[0]
        in1 = input_items[1]
        out = output_items[0]
        self.buf=np.take(in1.flatten(), self.lin)
        #print "Extracted noise"
        x = np.take(in0.flatten(), self.lin)
        #print "Extracted data"
        [D1,p1] = stats.ks_2samp(np.absolute(x), np.absolute(self.buf))
        #[D2,p2] = stats.ks_2samp(x.real, self.buf.real)
        if p1 < 0.05 and D1> 1.36*(np.sqrt(2.0/self.N)):
            print ('Signal found, p is ', p1, ' and D is ',D1, 'at sample', self.nitems_read(0))
            self.ctr=self.ctr+1
            print self.ctr
        
        out[:] = in0
        return len(output_items[0])

