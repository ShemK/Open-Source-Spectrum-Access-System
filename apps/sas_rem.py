#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Sas Rem
# Generated: Wed Jul 12 15:51:49 2017
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
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import sas
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
        self.fft_len = fft_len = 2048
        self.N = N = 364

        ##################################################
        # Blocks
        ##################################################
        self.utils_psd_cvf_0 = utils.psd_cvf(samp_rate,  fft_len, firdes.WIN_BLACKMAN_hARRIS, 0.8)
        self.sas_send_data_0 = sas.send_data(6000, '127.0.0.1')
        self.sas_sas_buffer_0 = sas.sas_buffer(N)
        self.sas_psql_insert_0 = sas.psql_insert(fft_len, 1)
        self.sas_ed_threshold_0 = sas.ed_threshold(fft_len, 1, 30)
        self.blocks_vector_to_stream_0 = blocks.vector_to_stream(gr.sizeof_gr_complex*1, N)
        self.blocks_message_debug_0 = blocks.message_debug()
        self.analog_noise_source_x_0 = analog.noise_source_f(analog.GR_GAUSSIAN, 1, 0)

        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.sas_ed_threshold_0, 'decision'), (self.sas_psql_insert_0, 'decision'))
        self.msg_connect((self.sas_ed_threshold_0, 'decision'), (self.sas_send_data_0, 'occ'))
        self.msg_connect((self.sas_ed_threshold_0, 'noise_floor'), (self.sas_send_data_0, 'noise_floor'))
        self.msg_connect((self.sas_psql_insert_0, 'nodeid'), (self.blocks_message_debug_0, 'print'))
        self.msg_connect((self.sas_psql_insert_0, 'ip'), (self.sas_send_data_0, 'ip'))
        self.msg_connect((self.sas_psql_insert_0, 'mac'), (self.sas_send_data_0, 'mac'))
        self.msg_connect((self.sas_psql_insert_0, 'nodeid'), (self.sas_send_data_0, 'nodeid'))
        self.msg_connect((self.sas_sas_buffer_0, 'center_freq'), (self.sas_psql_insert_0, 'center_freq'))
        self.msg_connect((self.sas_sas_buffer_0, 'center_freq'), (self.sas_send_data_0, 'center_freq'))
        self.msg_connect((self.sas_sas_buffer_0, 'samp_rate'), (self.sas_send_data_0, 'bw'))
        self.msg_connect((self.sas_sas_buffer_0, 'samp_rate'), (self.utils_psd_cvf_0, 'samp_rate'))
        self.connect((self.analog_noise_source_x_0, 0), (self.sas_send_data_0, 0))
        self.connect((self.blocks_vector_to_stream_0, 0), (self.utils_psd_cvf_0, 0))
        self.connect((self.sas_ed_threshold_0, 0), (self.sas_psql_insert_0, 0))
        self.connect((self.sas_sas_buffer_0, 0), (self.blocks_vector_to_stream_0, 0))
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
