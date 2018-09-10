#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright 2015 Chris Kuethe <chris.kuethe@gmail.com>
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
import socket
import pmt
from nmea_parser_core import nmea_parser_core, gpsd_parser_core

from datetime import datetime as dt
from datetime import date as dtdate
from gnuradio import gr

class nmea_reader(gr.sync_block):
	"""NMEA source block, fed by a gpsd instance"""

	def __init__(self, host='localhost', port=2947, protocol='nmea'):
		"""Set up network socket for NMEA streaming"""
		gr.sync_block.__init__(self, name="nmea_gpsd", in_sig=None, out_sig=[numpy.uint8])

		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.serial = self.sock.makefile('r')
		self.sock.connect((host, port))
		junk = self.serial.readline() # skip the daemon banner
		if protocol == 'nmea':
			self.sock.send('?WATCH={"enable":true,"nmea":true ,"json":false }\n\n')
		elif protocol == 'gpsd':
			self.sock.send('?WATCH={"enable":true,"nmea":false ,"json":true }\n\n')
		else:
			raise ValueError("invalid protocol '%s'" % protocol)
		self.protocol = protocol
		junk = self.serial.readline() # skip devices line
		junk = self.serial.readline() # skip the "WATCH" line

		self.valid = False
		self.nmea_time = self.host_time = dt.fromtimestamp(0)
		self.message_port_register_out(pmt.intern('gps_msg'))

	def work(self, input_items, output_items):
		"""Stream NMEA data for processing"""
		outbuf = output_items[0]
		outstr = self.serial.readline()
		self.host_time = dt.now()

		outlen = len(outstr)
		outbuf[:outlen] = numpy.fromstring(outstr, dtype=numpy.byte)

		try:
			if self.protocol == 'nmea':
				nmea_parser_core(self, outstr)
			if self.protocol == 'gpsd':
				gpsd_parser_core(self, outstr)
		except Exception as e:
			print e
			pass

		# bytes written to output buffer, messages and tags computed
		# if possible. let's go on our merry way...
		return outlen

if __name__ == '__main__':
	print "main()"
	from optparse import OptionParser
	from gnuradio import blocks

	parser = OptionParser(usage="%prog: [options]")
	parser.add_option("-s", "--server", dest="host", type="string", default="localhost",
		help="Set gpsd server [default=%default]", metavar='SERVER')
	parser.add_option("-p", "--port", dest="port", type="int", default=2947,
		help="Set gpsd port [default=%default]")
	parser.add_option("-r", "--raw", dest="raw", action="store_true", default=False,
		help="emit raw protocol [default=%default]")
	parser.add_option("-g", "--gpsd", dest="gpsd", action="store_true", default=False,
		help="emit raw protocol [default=%default]")
	(options, args) = parser.parse_args()

	# create flowgraph
	tb = gr.top_block()

	# GPS that drives this all...
	protocol = 'gpsd' if options.gpsd else 'nmea'
	gps_source = nmea_gpsd(options.host, options.port, protocol)

	# Definitely want message output
	message_sink = blocks.message_debug()
	tb.msg_connect((gps_source, 'gps_msg'), (message_sink, 'print'))

	# Most of the time we'll just chuck out the raw nmea bytes
	null_sink = blocks.null_sink(gr.sizeof_char*1)
	tb.connect((gps_source, 0), (null_sink, 0))

	if options.raw:
		stdout_sink = blocks.file_sink(gr.sizeof_char*1, "/dev/stdout")
		tb.connect((gps_source, 0), (stdout_sink, 0))

	# Kick out the jams!
	tb.start()

	try:
		raw_input('Press Enter to quit: ')
	except KeyboardInterrupt:
		print "\n"
	except EOFError:
		print "\n"

	tb.stop()
	tb.wait()
