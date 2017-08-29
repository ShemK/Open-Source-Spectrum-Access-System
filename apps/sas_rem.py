#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Sas Rem
# Generated: Mon Aug 28 21:31:23 2017
##################################################

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"

from PyQt4 import Qt
from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio import qtgui
from gnuradio.analog import cpm
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import sas
import signal_exciter
import sip
import sys
import utils
from gnuradio import qtgui


class sas_rem(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Sas Rem")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Sas Rem")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "sas_rem")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 4e6
        self.num_channels = num_channels = 10
        self.hop_time = hop_time = 1
        self.freq = freq = 1.000e9
        self.fft_len = fft_len = 4096
        self.N = N = 2048

        ##################################################
        # Blocks
        ##################################################
        self.utils_psd_cvf_0 = utils.psd_cvf(samp_rate,  fft_len, firdes.WIN_BLACKMAN_hARRIS, 0.8)
        self.signal_exciter_random_signal_0 = None
        sig_params = signal_exciter.sig_params()
        sig_params.type             = signal_exciter.OFDM
        sig_params.mod              = signal_exciter.QAM
        sig_params.order            = 2
        sig_params.offset           = 0.0
        sig_params.fftsize          = 64
        sig_params.cp_len           = 3
        sig_params.active_carriers  = 13
        sig_params.syms_per_frame   = 1
        sig_params.pilot_per_frame  = False
        sig_params.pilot_count      = 4

        _signal_exciter_random_signal_0_pilot_loc = signal_exciter.SizeVector(sig_params.pilot_count)
        _signal_exciter_random_signal_0_temp = ([0, 4, 8, 12])
        for ii in range(4):
          _signal_exciter_random_signal_0_pilot_loc[ii] = _signal_exciter_random_signal_0_temp[ii]
        sig_params.pilot_locations  = _signal_exciter_random_signal_0_pilot_loc
        sig_params.samp_overlap     = 0

        _signal_exciter_random_signal_0_taper = signal_exciter.FloatVector(sig_params.samp_overlap)
        _signal_exciter_random_signal_0_temp = ([])
        for ii in range(0):
          _signal_exciter_random_signal_0_taper[ii] = _signal_exciter_random_signal_0_temp[ii]
        sig_params.taper            = _signal_exciter_random_signal_0_taper
        sig_params.sps              = 1
        sig_params.pulse_len        = 1

        _signal_exciter_random_signal_0_pulse_shape = signal_exciter.FloatVector(sig_params.pulse_len)
        _signal_exciter_random_signal_0_temp = ([1])
        for ii in range(1):
          _signal_exciter_random_signal_0_pulse_shape[ii] = _signal_exciter_random_signal_0_temp[ii]
        sig_params.pulse_shape      = _signal_exciter_random_signal_0_pulse_shape
        sig_params.backoff          = 10.97
        sig_params.add_sync         = False
        sig_params.frac_symb_offset = 0.

        sig_params_signal_exciter_random_signal_0_ = sig_params
        self.signal_exciter_random_signal_0 = signal_exciter.random_signal(sig_params_signal_exciter_random_signal_0_, -1)
        self.signal_exciter_random_gate_0 = signal_exciter.random_gate(samp_rate, 0.2, 1.5, 0.5, 2, 2017)
        self.sas_psql_insert_0 = sas.psql_insert(fft_len, num_channels)
        self.sas_ed_threshold_0 = sas.ed_threshold(fft_len, num_channels, 200)
        self.qtgui_vector_sink_f_0 = qtgui.vector_sink_f(
            fft_len,
            0,
            1.0,
            "x-Axis",
            "y-Axis",
            "",
            1 # Number of inputs
        )
        self.qtgui_vector_sink_f_0.set_update_time(0.10)
        self.qtgui_vector_sink_f_0.set_y_axis(-140, 10)
        self.qtgui_vector_sink_f_0.enable_autoscale(False)
        self.qtgui_vector_sink_f_0.enable_grid(False)
        self.qtgui_vector_sink_f_0.set_x_axis_units("")
        self.qtgui_vector_sink_f_0.set_y_axis_units("")
        self.qtgui_vector_sink_f_0.set_ref_level(0)

        labels = ['', '', '', '', '',
                  '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]
        for i in xrange(1):
            if len(labels[i]) == 0:
                self.qtgui_vector_sink_f_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_vector_sink_f_0.set_line_label(i, labels[i])
            self.qtgui_vector_sink_f_0.set_line_width(i, widths[i])
            self.qtgui_vector_sink_f_0.set_line_color(i, colors[i])
            self.qtgui_vector_sink_f_0.set_line_alpha(i, alphas[i])

        self._qtgui_vector_sink_f_0_win = sip.wrapinstance(self.qtgui_vector_sink_f_0.pyqwidget(), Qt.QWidget)
        self.top_layout.addWidget(self._qtgui_vector_sink_f_0_win)
        self.qtgui_freq_sink_x_0 = qtgui.freq_sink_c(
        	1024, #size
        	firdes.WIN_BLACKMAN_hARRIS, #wintype
        	0, #fc
        	samp_rate, #bw
        	"", #name
        	1 #number of inputs
        )
        self.qtgui_freq_sink_x_0.set_update_time(0.10)
        self.qtgui_freq_sink_x_0.set_y_axis(-140, 10)
        self.qtgui_freq_sink_x_0.set_y_label('Relative Gain', 'dB')
        self.qtgui_freq_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, 0.0, 0, "")
        self.qtgui_freq_sink_x_0.enable_autoscale(False)
        self.qtgui_freq_sink_x_0.enable_grid(False)
        self.qtgui_freq_sink_x_0.set_fft_average(1.0)
        self.qtgui_freq_sink_x_0.enable_axis_labels(True)
        self.qtgui_freq_sink_x_0.enable_control_panel(False)

        if not True:
          self.qtgui_freq_sink_x_0.disable_legend()

        if "complex" == "float" or "complex" == "msg_float":
          self.qtgui_freq_sink_x_0.set_plot_pos_half(not True)

        labels = ['', '', '', '', '',
                  '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
                  1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
                  "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
                  1.0, 1.0, 1.0, 1.0, 1.0]
        for i in xrange(1):
            if len(labels[i]) == 0:
                self.qtgui_freq_sink_x_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_freq_sink_x_0.set_line_label(i, labels[i])
            self.qtgui_freq_sink_x_0.set_line_width(i, widths[i])
            self.qtgui_freq_sink_x_0.set_line_color(i, colors[i])
            self.qtgui_freq_sink_x_0.set_line_alpha(i, alphas[i])

        self._qtgui_freq_sink_x_0_win = sip.wrapinstance(self.qtgui_freq_sink_x_0.pyqwidget(), Qt.QWidget)
        self.top_layout.addWidget(self._qtgui_freq_sink_x_0_win)
        self.low_pass_filter_0 = filter.fir_filter_ccf(1, firdes.low_pass(
        	1e-2, samp_rate, 2e6, 1e5, firdes.WIN_HAMMING, 6.76))
        self.blocks_throttle_0_0 = blocks.throttle(gr.sizeof_gr_complex*1, samp_rate,True)
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_gr_complex*1, samp_rate,True)
        self.blocks_add_xx_0 = blocks.add_vcc(1)
        self.analog_noise_source_x_0 = analog.noise_source_c(analog.GR_GAUSSIAN, 8e-5, 0)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_noise_source_x_0, 0), (self.blocks_throttle_0_0, 0))
        self.connect((self.blocks_add_xx_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.qtgui_freq_sink_x_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.utils_psd_cvf_0, 0))
        self.connect((self.blocks_throttle_0_0, 0), (self.blocks_add_xx_0, 1))
        self.connect((self.low_pass_filter_0, 0), (self.blocks_add_xx_0, 0))
        self.connect((self.sas_ed_threshold_0, 0), (self.qtgui_vector_sink_f_0, 0))
        self.connect((self.sas_ed_threshold_0, 0), (self.sas_psql_insert_0, 0))
        self.connect((self.signal_exciter_random_gate_0, 0), (self.low_pass_filter_0, 0))
        self.connect((self.signal_exciter_random_signal_0, 0), (self.signal_exciter_random_gate_0, 0))
        self.connect((self.utils_psd_cvf_0, 0), (self.sas_ed_threshold_0, 0))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "sas_rem")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.utils_psd_cvf_0.set_samp_rate(self.samp_rate)
        self.qtgui_freq_sink_x_0.set_frequency_range(0, self.samp_rate)
        self.low_pass_filter_0.set_taps(firdes.low_pass(1e-2, self.samp_rate, 2e6, 1e5, firdes.WIN_HAMMING, 6.76))
        self.blocks_throttle_0_0.set_sample_rate(self.samp_rate)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)

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

    from distutils.version import StrictVersion
    if StrictVersion(Qt.qVersion()) >= StrictVersion("4.5.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()
    tb.start()
    tb.show()

    def quitting():
        tb.stop()
        tb.wait()
    qapp.connect(qapp, Qt.SIGNAL("aboutToQuit()"), quitting)
    qapp.exec_()


if __name__ == '__main__':
    main()
