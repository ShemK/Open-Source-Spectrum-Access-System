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
import pynmea2
import pmt
import json

from pmt import string_to_symbol as PSTR
from pmt import from_double as PDBL
from pmt import vector_set as PMT_SET

from datetime import datetime as dt
from datetime import date as dtdate

def gpsd_parser_core(self, gpsd_str):
	G = json.loads(gpsd_str)
	if 'TPV' not in G['class']:
		return
    
	D = pmt.make_f64vector(2,PDBL(0))
	PMT_SET(D, 1, PDBL(G['lon']))
	PMT_SET(D, 0, PDBL(G['lat']))
	self.message_port_pub(pmt.intern('gps_msg'), D)

def nmea_parser_core(self, nmea_str):
    fixobj = pynmea2.parse(nmea_str)
    nmea_id = fixobj.sentence_type
    
    if nmea_id not in ['GGA', 'GLL', 'RMC', 'VTG']:
		raise AttributeError("Unparsed Sentence")
    
    D = pmt.make_vector(2, PDBL(0))
    try:
		PMT_SET(D, 0, PDBL(fixobj.latitude))
		PMT_SET(D, 1, PDBL(fixobj.longitude))
    except AttributeError:
		pass
    
    self.message_port_pub(pmt.intern('gps_msg'), D)
