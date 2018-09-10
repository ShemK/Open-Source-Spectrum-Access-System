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
import pmt
import time

class uhd_control(gr.sync_block):
    """
    docstring for block uhd_control
    """
    def __init__(self):
        self.st = time.time()
        gr.sync_block.__init__(self,
            name="uhd_control",
            in_sig=None,
            out_sig=[np.float32])
        self.message_port_register_out(pmt.intern("control"))

    def work(self, input_items, output_items):
        out = output_items[0]
        self.end = time.time()
        
        if (self.end - self.st) >= 5:
            self.message_port_pub(pmt.intern("control"),self.send_command(1e9,2e6))
            print (self.end - self.st)
            self.st = time.time()
        return len(output_items[0])

    def send_command(self, freq, bw):
        command = pmt.make_dict()
        command = pmt.dict_add(command, pmt.intern("freq"),pmt.from_double(freq))
        command = pmt.dict_add(command, pmt.intern("bandwidth"),pmt.from_double(freq))
        return command