#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Sas Rem
# Generated: Tue Jul 25 00:10:43 2017
##################################################

from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import uhd
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import sas
import time
import utils


class sas_rem(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "Sas Rem")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 8e6
        self.freq = freq = 1.004e9
        self.fft_len = fft_len = 2048
        self.N = N = 364

        ##################################################
        # Blocks
        ##################################################
        self.utils_psd_cvf_0 = utils.psd_cvf(samp_rate,  fft_len, firdes.WIN_BLACKMAN_hARRIS, 0.8)
        self.uhd_usrp_source_0 = uhd.usrp_source(
        	",".join(("", "")),
        	uhd.stream_args(
        		cpu_format="fc32",
        		channels=range(1),
        	),
        )
        self.uhd_usrp_source_0.set_samp_rate(samp_rate)
        self.uhd_usrp_source_0.set_center_freq(0, 0)
        self.uhd_usrp_source_0.set_gain(20, 0)
        self.sas_uhd_control_0 = sas.uhd_control(5, samp_rate, freq)
        self.sas_send_data_0 = sas.send_data(6000, '192.168.1.21', 10)
        self.sas_psql_insert_0 = sas.psql_insert(fft_len, 1)
        self.sas_ed_threshold_0 = sas.ed_threshold(fft_len, 10, 200)
        self.blocks_null_sink_2 = blocks.null_sink(gr.sizeof_float*1)
        self.analog_noise_source_x_0 = analog.noise_source_f(analog.GR_GAUSSIAN, 1, 0)

        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.sas_ed_threshold_0, 'decision'), (self.sas_psql_insert_0, 'decision'))
        self.msg_connect((self.sas_ed_threshold_0, 'decision'), (self.sas_send_data_0, 'occ'))
        self.msg_connect((self.sas_ed_threshold_0, 'noise_floor'), (self.sas_send_data_0, 'noise_floor'))
        self.msg_connect((self.sas_psql_insert_0, 'ip'), (self.sas_send_data_0, 'ip'))
        self.msg_connect((self.sas_psql_insert_0, 'mac'), (self.sas_send_data_0, 'mac'))
        self.msg_connect((self.sas_psql_insert_0, 'nodeid'), (self.sas_send_data_0, 'nodeid'))
        self.msg_connect((self.sas_uhd_control_0, 'bandwidth'), (self.sas_send_data_0, 'bw'))
        self.msg_connect((self.sas_uhd_control_0, 'center_freq'), (self.sas_send_data_0, 'center_freq'))
        self.msg_connect((self.sas_uhd_control_0, 'control'), (self.uhd_usrp_source_0, 'command'))
        self.connect((self.analog_noise_source_x_0, 0), (self.sas_send_data_0, 0))
        self.connect((self.sas_ed_threshold_0, 0), (self.sas_psql_insert_0, 0))
        self.connect((self.sas_uhd_control_0, 0), (self.blocks_null_sink_2, 0))
        self.connect((self.uhd_usrp_source_0, 0), (self.utils_psd_cvf_0, 0))
        self.connect((self.utils_psd_cvf_0, 0), (self.sas_ed_threshold_0, 0))

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.utils_psd_cvf_0.set_samp_rate(self.samp_rate)
        self.uhd_usrp_source_0.set_samp_rate(self.samp_rate)

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq

    def get_fft_len(self):
        return self.fft_len

    def set_fft_len(self, fft_len):
        self.fft_len = fft_len
        self.utils_psd_cvf_0.set_fft_len( self.fft_len)

    def get_N(self):
        return self.N

    def set_N(self, N):
        self.N = N


def main(top_block_cls=sas_rem, options=None):

    tb = top_block_cls()
    tb.start()
    try:
        raw_input('Press Enter to quit: ')
    except EOFError:
        pass
    tb.stop()
    tb.wait()


if __name__ == '__main__':
    main()
