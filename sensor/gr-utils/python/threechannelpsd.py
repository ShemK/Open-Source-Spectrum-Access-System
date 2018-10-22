# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Threechannelpsd
# Generated: Fri Apr 28 11:08:24 2017
##################################################

from gnuradio import gr
from gnuradio.filter import firdes
import utils


class threechannelpsd(gr.hier_block2):

    def __init__(self, alpha=0.8, fft_len=1024, samp_rate=32000):
        gr.hier_block2.__init__(
            self, "Threechannelpsd",
            gr.io_signaturev(3, 3, [gr.sizeof_gr_complex*1, gr.sizeof_gr_complex*1, gr.sizeof_gr_complex*1]),
            gr.io_signaturev(3, 3, [gr.sizeof_float*fft_len, gr.sizeof_float*fft_len, gr.sizeof_float*fft_len]),
        )

        ##################################################
        # Parameters
        ##################################################
        self.alpha = alpha
        self.fft_len = fft_len
        self.samp_rate = samp_rate

        ##################################################
        # Blocks
        ##################################################
        self.utils_psd_cvf_0_9_1 = utils.psd_cvf(samp_rate,  fft_len, firdes.WIN_BLACKMAN_hARRIS, alpha)
        self.utils_psd_cvf_0_9_0 = utils.psd_cvf(samp_rate,  fft_len, firdes.WIN_BLACKMAN_hARRIS, alpha)
        self.utils_psd_cvf_0_9 = utils.psd_cvf(samp_rate,  fft_len, firdes.WIN_BLACKMAN_hARRIS, alpha)

        ##################################################
        # Connections
        ##################################################
        self.connect((self, 0), (self.utils_psd_cvf_0_9, 0))
        self.connect((self, 1), (self.utils_psd_cvf_0_9_0, 0))
        self.connect((self, 2), (self.utils_psd_cvf_0_9_1, 0))
        self.connect((self.utils_psd_cvf_0_9, 0), (self, 0))
        self.connect((self.utils_psd_cvf_0_9_0, 0), (self, 1))
        self.connect((self.utils_psd_cvf_0_9_1, 0), (self, 2))

    def get_alpha(self):
        return self.alpha

    def set_alpha(self, alpha):
        self.alpha = alpha
        self.utils_psd_cvf_0_9_1.set_average(self.alpha)
        self.utils_psd_cvf_0_9_0.set_average(self.alpha)
        self.utils_psd_cvf_0_9.set_average(self.alpha)

    def get_fft_len(self):
        return self.fft_len

    def set_fft_len(self, fft_len):
        self.fft_len = fft_len
        self.utils_psd_cvf_0_9_1.set_fft_len( self.fft_len)
        self.utils_psd_cvf_0_9_0.set_fft_len( self.fft_len)
        self.utils_psd_cvf_0_9.set_fft_len( self.fft_len)

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.utils_psd_cvf_0_9_1.set_samp_rate(self.samp_rate)
        self.utils_psd_cvf_0_9_0.set_samp_rate(self.samp_rate)
        self.utils_psd_cvf_0_9.set_samp_rate(self.samp_rate)
