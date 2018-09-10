# Copyright 2011 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
install(FILES
    utils_log10_vfvf.xml
    utils_psd_cvf.xml
    utils_pipe_sink.xml
    utils_shmem_write.xml
    utils_shmem_read.xml
    utils_reader.xml 
    utils_streampsdvolk.xml
    utils_gps_reader.xml
    utils_ks_test.xml
    utils_anderson.xml
    utils_shapiro.xml
    utils_skewness.xml
    utils_fbpsd_cvf.xml
    utils_nmea_reader.xml
    utils_cstates_read.xml
    utils_uhd_control.xml
    utils_liquid_buffer.xml
    utils_multichan_ed.xml DESTINATION share/gnuradio/grc/blocks
)
