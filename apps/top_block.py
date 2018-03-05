#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Top Block
# Generated: Thu Mar  1 21:41:34 2018
##################################################

from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import uhd
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import sas
import time


class top_block(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "Top Block")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 20e6
        self.freq = freq = 3560e6
        self.N = N = 2048

        ##################################################
        # Blocks
        ##################################################
        self.uhd_usrp_source_0 = uhd.usrp_source(
        	",".join(("", "")),
        	uhd.stream_args(
        		cpu_format="fc32",
        		channels=range(1),
        	),
        )
        self.uhd_usrp_source_0.set_samp_rate(samp_rate)
        self.uhd_usrp_source_0.set_center_freq(1009e6, 0)
        self.uhd_usrp_source_0.set_gain(20, 0)
        self.uhd_usrp_source_0.set_antenna('RX2', 0)
        self.uhd_usrp_source_0.set_bandwidth(samp_rate, 0)
        self.sas_uhd_control_0 = sas.uhd_control(1, samp_rate, freq, 20)
        self.sas_sas_buffer_write_0 = sas.sas_buffer_write(N)
        self.blocks_stream_to_vector_0 = blocks.stream_to_vector(gr.sizeof_gr_complex*1, N)
        self.blocks_null_sink_2 = blocks.null_sink(gr.sizeof_float*1)

        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.sas_uhd_control_0, 'bandwidth'), (self.sas_sas_buffer_write_0, 'samp_rate'))
        self.msg_connect((self.sas_uhd_control_0, 'center_freq'), (self.sas_sas_buffer_write_0, 'center_freq'))
        self.msg_connect((self.sas_uhd_control_0, 'control'), (self.uhd_usrp_source_0, 'command'))
        self.connect((self.blocks_stream_to_vector_0, 0), (self.sas_sas_buffer_write_0, 0))
        self.connect((self.sas_uhd_control_0, 0), (self.blocks_null_sink_2, 0))
        self.connect((self.uhd_usrp_source_0, 0), (self.blocks_stream_to_vector_0, 0))

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.uhd_usrp_source_0.set_samp_rate(self.samp_rate)
        self.uhd_usrp_source_0.set_bandwidth(self.samp_rate, 0)

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq

    def get_N(self):
        return self.N

    def set_N(self, N):
        self.N = N


def main(top_block_cls=top_block, options=None):

    tb = top_block_cls()
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
