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

import numpy
import pmt
from gnuradio import gr
import socket
import random
from time import gmtime, strftime

class send_data(gr.sync_block):
    """
    docstring for block send_data
    """
    def __init__(self, port, host):
        self.center_freq = 1e9
        self.bw = 2e6
        self.occ = 1
        self.noise_floor  = -200.0
        self.host = host
        self.port =  port
        self.nodeid = 0
        gr.sync_block.__init__(self,
            name="send_data",
            in_sig=[numpy.float32],
            out_sig=None)
        self.message_port_register_in(pmt.intern("center_freq"))
        self.message_port_register_in(pmt.intern("occ"))
        self.message_port_register_in(pmt.intern("bw"))
        self.message_port_register_in(pmt.intern("noise_floor"))
        self.set_msg_handler(pmt.intern("center_freq"), self.center_freq_handler_method)
        self.set_msg_handler(pmt.intern("occ"), self.occ_handler_method)
        self.set_msg_handler(pmt.intern("bw"), self.bw_handler_method)
        self.set_msg_handler(pmt.intern("noise_floor"), self.noise_floor_handler_method)

    def work(self, input_items, output_items):
        in0 = input_items[0]
       

        return len(input_items[0])
    def center_freq_handler_method(self, msg):
        self.center_freq = pmt.to_double(msg)

    def bw_handler_method(self, msg): 
        self.bw =  pmt.to_double(msg)

    def occ_handler_method(self, msg):
        self.occ = pmt.to_double(pmt.vector_ref(msg,0))
        pmt_to_send  = pmt.make_dict()

        curtime = strftime("%Y-%m-%d %H:%M:%S", gmtime())
        attributes = pmt.make_dict()
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("center_freq"),pmt.from_double(self.center_freq))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("occ"),pmt.from_double(self.occ))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("bandwidth"),pmt.from_double(self.bw))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("timetag"),pmt.intern(curtime))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("noise_floor"),pmt.from_double(self.noise_floor))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("nodeid"),pmt.from_double(self.nodeid))

        command = pmt.make_dict()
        command = pmt.dict_add(command,pmt.string_to_symbol("table"),pmt.string_to_symbol("spectruminfo"))
        command = pmt.dict_add(command, pmt.string_to_symbol("attributes"),attributes)

        pmt_to_send = pmt.dict_add(pmt_to_send, pmt.string_to_symbol("INSERT"),command)

        serialized_pmt = pmt.serialize_str(pmt_to_send)

        #print pmt_to_send

        UDP_IP = self.host
        UDP_PORT = self.port

        sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM) 
        sock.sendto(serialized_pmt, (UDP_IP, UDP_PORT))
        print "sending data", pmt_to_send
    
    def noise_floor_handler_method(self, msg):
        self.noise_floor = pmt.to_double(msg)
        