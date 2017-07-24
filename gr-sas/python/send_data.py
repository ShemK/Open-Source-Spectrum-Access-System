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
    def __init__(self, port, host, num_split):
        self.center_freq = 1e9
        self.bw = 2e6
        self.occ = 1
        self.noise_floor  = -200.0
        self.host = host
        self.port =  port
        self.num_split = num_split
        self.nodeid = 1
        self.latitude = 0
        self.longitude = 0
        self.mac = ""
        self.ip = ""
        gr.sync_block.__init__(self,
            name="send_data",
            in_sig=[numpy.float32],
            out_sig=None)
        self.message_port_register_in(pmt.intern("center_freq"))
        self.message_port_register_in(pmt.intern("occ"))
        self.message_port_register_in(pmt.intern("bw"))
        self.message_port_register_in(pmt.intern("noise_floor"))
        self.message_port_register_in(pmt.intern("nodeid"))
        self.message_port_register_in(pmt.intern("mac"))
        self.message_port_register_in(pmt.intern("ip"))
        self.message_port_register_in(pmt.intern("gps"))


        self.set_msg_handler(pmt.intern("center_freq"), self.center_freq_handler_method)
        self.set_msg_handler(pmt.intern("occ"), self.occ_handler_method)
        self.set_msg_handler(pmt.intern("bw"), self.bw_handler_method)
        self.set_msg_handler(pmt.intern("noise_floor"), self.noise_floor_handler_method)
        self.set_msg_handler(pmt.intern("nodeid"), self.nodeid_handler_method)
        self.set_msg_handler(pmt.intern("mac"), self.mac_handler_method)
        self.set_msg_handler(pmt.intern("ip"), self.ip_handler_method)
        self.set_msg_handler(pmt.intern("gps"), self.gps_handler_method)

    def work(self, input_items, output_items):
        in0 = input_items[0]


        return len(input_items[0])
    def center_freq_handler_method(self, msg):
        self.center_freq = pmt.to_double(msg)

    def bw_handler_method(self, msg):
        self.bw =  pmt.to_double(msg)

    def occ_handler_method(self, msg):
        occvec = [] 
        
        for i in range(0,self.num_split):
            occvec.append(pmt.to_double(pmt.vector_ref(msg,i)))

        pmt_to_send  = pmt.make_dict()
        

        curtime = strftime("%Y-%m-%d %H:%M:%S", gmtime())
        attributes = pmt.make_dict()
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("center_freq"),pmt.from_double(self.center_freq))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("occ"),pmt.init_f32vector(self.num_split, occvec))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("bandwidth"),pmt.from_double(self.bw))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("timetag"),pmt.intern(curtime))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("noise_floor"),pmt.from_double(self.noise_floor))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("nodeid"),pmt.from_long(self.nodeid))

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

    def nodeid_handler_method(self, msg):
        self.nodeid = pmt.to_long(msg)
        pmt_to_send  = pmt.make_dict()

        attributes = pmt.make_dict()
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("nodeID"),pmt.from_long(self.nodeid))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("nodeMac"),pmt.intern(self.mac))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("nodeIp"),pmt.intern(self.ip))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("latitude"),pmt.from_double(self.latitude))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("longitude"),pmt.from_double(self.longitude))
        attributes = pmt.dict_add(attributes, pmt.string_to_symbol("nodeType"),pmt.from_long(1));

        command = pmt.make_dict()
        command = pmt.dict_add(command, pmt.string_to_symbol("attributes"),attributes)

        pmt_to_send = pmt.dict_add(pmt_to_send, pmt.string_to_symbol("NodeParam"),command)

        serialized_pmt = pmt.serialize_str(pmt_to_send)

        #print pmt_to_send

        UDP_IP = self.host
        UDP_PORT = self.port

        sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        sock.sendto(serialized_pmt, (UDP_IP, UDP_PORT))
        print "sending node data", pmt_to_send

    def gps_handler_method(self, msg):
        self.latitude =  pmt.to_double(pmt.vector_ref(msg,0))
        self.longitude =  pmt.to_double(pmt.vector_ref(msg,1))

    def mac_handler_method(self, msg):
        self.mac =  pmt.symbol_to_string(msg)

    def ip_handler_method(self, msg):
        self.ip =  pmt.symbol_to_string(msg)
