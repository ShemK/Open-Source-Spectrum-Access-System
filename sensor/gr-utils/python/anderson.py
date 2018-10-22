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
class anderson(gr.sync_block):
    """
    docstring for block anderson
    """
    def __init__(self, N):
        self.N = N
        self.buf = np.zeros(shape = (N,), dtype=np.complex64)
        self.ctr=0
        gr.sync_block.__init__(self,
            name="anderson",
            in_sig=[(np.complex64, self.N)],
            out_sig=[(np.complex64, self.N)])
        

    def work(self, input_items, output_items):
        in0 = input_items[0]
        out = output_items[0]
        #print in0.shape
        x = in0.reshape(self.N)
        #print x.shape[0]
        #print self.buf
        #print x.imag
        #print np.append(x, self.buf).shape
        [D1,z,p1] = stats.anderson_ksamp(x.imag, self.buf.imag)
        [D2,z,p2] = stats.anderson_ksamp(x.real, self.buf.real)
        if p1 < 0.05 and p2< 0.05 :
            print ('Not similar, p is ', p1," ",p2, 'at sample', self.nitems_read(0))
            self.ctr=self.ctr+1
            print self.ctr
        self.buf=np.copy(x)
        out[:] = in0
        return len(output_items[0])