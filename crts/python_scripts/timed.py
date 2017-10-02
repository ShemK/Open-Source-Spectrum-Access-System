#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Transmitter
# Generated: Fri Feb  3 22:07:42 2017
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
from gnuradio import uhd
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from gnuradio.qtgui import Range, RangeWidget
from optparse import OptionParser
import sys
import time
from gnuradio import qtgui
import server_connection
import json
import cbsd
import cbsd_thread
import datetime

class transmitter(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Transmitter")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Transmitter")
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

        self.settings = Qt.QSettings("GNU Radio", "transmitter")
        self.restoreGeometry(self.settings.value("geometry").toByteArray())

        ##################################################
        # Variables
        ##################################################
        self.txgain = txgain = -50
        self.samp_rate = samp_rate = 500000
        self.freq = freq = 870000000

        ##################################################
        # Blocks
        ##################################################
        self._txgain_range = Range(0, 5, 1, 0, 200)
        self._txgain_win = RangeWidget(self._txgain_range, self.set_txgain, "txgain", "counter_slider", float)
        self.top_layout.addWidget(self._txgain_win)
        self.uhd_usrp_sink_0 = uhd.usrp_sink(
        	",".join(("", "")),
        	uhd.stream_args(
        		cpu_format="fc32",
        		channels=range(1),
        	),
        )
        self.uhd_usrp_sink_0.set_samp_rate(samp_rate)
        self.uhd_usrp_sink_0.set_center_freq(freq, 0)
        self.uhd_usrp_sink_0.set_gain(txgain, 0)
        self.uhd_usrp_sink_0.set_antenna('TX/RX', 0)
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_gr_complex*1, samp_rate,True)
        self.analog_sig_source_x_0 = analog.sig_source_c(samp_rate, analog.GR_COS_WAVE, 1000, 1, 0)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_sig_source_x_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.blocks_throttle_0, 0), (self.uhd_usrp_sink_0, 0))

    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "transmitter")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_txgain(self):
        return self.txgain

    def set_txgain(self, txgain):
        self.txgain = txgain
        self.uhd_usrp_sink_0.set_gain(self.txgain, 0)


    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.uhd_usrp_sink_0.set_samp_rate(self.samp_rate)
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)
        self.analog_sig_source_x_0.set_sampling_freq(self.samp_rate)

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.uhd_usrp_sink_0.set_center_freq(self.freq, 0)


def main():
    newCbsd = cbsd.Cbsd("cbd561","A","cbd1","hask124ba")
    newCbsd.add_registration_parameters("callSign","CB987")
    json_encoder = json.JSONEncoder()
    json_request =  json_encoder.encode(newCbsd.get_registrationRequestObj())
    my_server_connection = server_connection.Server_connection("http://172.29.70.54/spectrumAccessSystem/start.php")
    print "\n---------------------------------------------------------\n"
    print("Sending Registration Request")
    # get response string
    response = my_server_connection.send_request(json_request)

    # create json decoder object
    json_decoder = json.JSONDecoder()
    # get dictionary
    json_response = json_decoder.decode(response)

    #change state of cbsd
    newCbsd.set_registrationResponseObj(json_response)

    print("Registration Accepted, Radio Authenticated")
    # send channel inquiry in order of importance
    newCbsd.add_inquired_channels(880,890)
    newCbsd.add_inquired_channels(890,900)
    newCbsd.add_inquired_channels(880,890)
    newCbsd.add_inquired_channels(860,870)


    '''
    Inquire about the spectrum
    '''

    print "\n---------------------------------------------------------\n"

    print("Request: sending spectrum Inquiry ")

    print "Sending Inquiry for the Following Channels:"
    inquired_channels = newCbsd.get_inquired_channels()

    for i in newCbsd.get_inquired_channels():
        print i

    print "\n---------------------------------------------------------\n"

    json_request = json_encoder.encode(newCbsd.get_spectrumInquiryRequestObj())
    response = my_server_connection.send_request(json_request)
        # get dictionary
    json_response = json_decoder.decode(response)

    newCbsd.set_spectrumInquiryResponseObj(json_response)
    print "Below are the Channels Available:"

    for i in newCbsd.get_available_channels():
        print i

    print "\n---------------------------------------------------------\n"

    '''
        Get Grant Request
    '''

    json_request = json_encoder.encode(newCbsd.get_grantRequestObj())

    print "Requesting Grant for the following Channel"
    print newCbsd.get_operationFrequencyRange()

    response = my_server_connection.send_request(json_request)
    # get dictionary
    json_response = json_decoder.decode(response)
    newCbsd.set_grantResponseObj(json_response)

    operationFrequency = newCbsd.get_operationFrequencyRange()

    print "Channel Grant Accepted"

    print "\n---------------------------------------------------------\n"

    print "Sending Initial Heartbeat"
    freq = operationFrequency['lowFrequency']

    heartbeatInterval = newCbsd.get_heartbeatInterval()
    json_request = json_encoder.encode(newCbsd.get_heartbeatRequestObj())
    response = my_server_connection.send_request(json_request)
    #print("Initial Heartbeat:", response)
    my_heartbeat_Thread = cbsd_thread.cbsd_thread(newCbsd,my_server_connection,\
                                        "heartbeat",heartbeatInterval,json_r = json_request)

    print "Heartbeat to be send every ",heartbeatInterval,"s"
    my_heartbeat_Thread.start()

    current_time = time.mktime(datetime.datetime.utcnow().timetuple())
#    print("server_time: ",newCbsd.get_grantExpireTime())
#    print("client time: ", datetime.datetime.utcnow().timetuple())
    grantInterval = newCbsd.get_grantExpireTime_seconds() - current_time
    print "Grant Ends in: ", grantInterval,"s"
    my_grant_Thread = cbsd_thread.cbsd_thread(newCbsd,my_server_connection,\
                                            "grant",grantInterval,heartbeat_thread = my_heartbeat_Thread)
    my_grant_Thread.start()

    freq = newCbsd.get_operationFrequencyRange()
    low_freq = freq['lowFrequency']
    print "\n---------------------------------------------------------\n"
    print "Setting Up Transmission"
    from distutils.version import StrictVersion
    if StrictVersion(Qt.qVersion()) >= StrictVersion("4.5.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)
    freq = int(low_freq)*1000000
    tb = transmitter()
    tb.set_freq(freq)
    tb.start()
    print "\n---------------------------------------------------------\n"
    time.sleep(grantInterval)

    def quitting():
	#print("Freq:",tb.get_freq())
        tb.stop()
        tb.wait()
    quitting()
    #qapp.connect(qapp, Qt.SIGNAL("aboutToQuit()"), quitting)
    #qapp.exec_()


if __name__ == '__main__':
    main()
