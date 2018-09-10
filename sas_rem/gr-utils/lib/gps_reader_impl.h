/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_UTILS_GPS_READER_IMPL_H
#define INCLUDED_UTILS_GPS_READER_IMPL_H

#include <utils/gps_reader.h>
#include <iostream>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h> 

namespace gr {
  namespace utils {

    class gps_reader_impl : public gps_reader
    {
     private:
      const int recv_buffer_len = 8192 * 2;
      int tcp_client_sock, status, recv_len;
      char *recv_buffer, *pch, *prev;
      float longitude,latitude;
      socklen_t clientlen;
      struct sockaddr_in tcp_server_addr, tcp_client_addr;
     public:
      gps_reader_impl(int port);
      ~gps_reader_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace utils
} // namespace gr

#endif /* INCLUDED_UTILS_GPS_READER_IMPL_H */

