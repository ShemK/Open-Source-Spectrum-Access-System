#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Sas Rem
# Generated: Thu Mar  1 21:41:48 2018
##################################################

from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import sas
import utils


class sas_rem(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "Sas Rem")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 20e6
        self.num_channels = num_channels = 10
        self.hop_time = hop_time = 1
        self.freq = freq = 1.000e9
        self.fft_len = fft_len = 4096
        self.N = N = 2048

        ##################################################
        # Blocks
        ##################################################
        self.utils_psd_cvf_0 = utils.psd_cvf(samp_rate,  fft_len, firdes.WIN_BLACKMAN_hARRIS, 0.8)
        self.utils_nmea_reader_0 = utils.nmea_reader('localhost', 2947, 'nmea')
        self.sas_send_data_0 = sas.send_data(6000, '192.168.1.21', num_channels, fft_len)
        self.sas_sas_buffer_0 = sas.sas_buffer(N)
        self.sas_psql_insert_0 = sas.psql_insert(fft_len, num_channels)
        self.sas_ed_threshold_0 = sas.ed_threshold(fft_len, num_channels, 200)
        self.blocks_vector_to_stream_0 = blocks.vector_to_stream(gr.sizeof_gr_complex*1, N)
        self.blocks_null_source_0 = blocks.null_source(gr.sizeof_float*1)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_char*1)

        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.sas_ed_threshold_0, 'decision'), (self.sas_send_data_0, 'occ'))
        self.msg_connect((self.sas_ed_threshold_0, 'noise_floor'), (self.sas_send_data_0, 'noise_floor'))
        self.msg_connect((self.sas_psql_insert_0, 'ip'), (self.sas_send_data_0, 'ip'))
        self.msg_connect((self.sas_psql_insert_0, 'mac'), (self.sas_send_data_0, 'mac'))
        self.msg_connect((self.sas_psql_insert_0, 'nodeid'), (self.sas_send_data_0, 'nodeid'))
        self.msg_connect((self.sas_sas_buffer_0, 'center_freq'), (self.sas_psql_insert_0, 'center_freq'))
        self.msg_connect((self.sas_sas_buffer_0, 'samp_rate'), (self.sas_psql_insert_0, 'samp_rate'))
        self.msg_connect((self.sas_sas_buffer_0, 'center_freq'), (self.sas_send_data_0, 'center_freq'))
        self.msg_connect((self.sas_sas_buffer_0, 'samp_rate'), (self.sas_send_data_0, 'bw'))
        self.msg_connect((self.utils_nmea_reader_0, 'gps_msg'), (self.sas_psql_insert_0, 'latlong'))
        self.msg_connect((self.utils_nmea_reader_0, 'gps_msg'), (self.sas_send_data_0, 'gps'))
        self.connect((self.blocks_null_source_0, 0), (self.sas_send_data_0, 0))
        self.connect((self.blocks_vector_to_stream_0, 0), (self.utils_psd_cvf_0, 0))
        self.connect((self.sas_ed_threshold_0, 0), (self.sas_psql_insert_0, 0))
        self.connect((self.sas_sas_buffer_0, 0), (self.blocks_vector_to_stream_0, 0))
        self.connect((self.utils_nmea_reader_0, 0), (self.blocks_null_sink_0, 0))
        self.connect((self.utils_psd_cvf_0, 0), (self.sas_ed_threshold_0, 0))

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.utils_psd_cvf_0.set_samp_rate(self.samp_rate)

    def get_num_channels(self):
        return self.num_channels

    def set_num_channels(self, num_channels):
        self.num_channels = num_channels

    def get_hop_time(self):
        return self.hop_time

    def set_hop_time(self, hop_time):
        self.hop_time = hop_time

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
    tb.wait()


if __name__ == '__main__':
    main()
