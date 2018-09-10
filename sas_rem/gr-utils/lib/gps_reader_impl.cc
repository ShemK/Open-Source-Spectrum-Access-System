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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "gps_reader_impl.h"

namespace gr {
  namespace utils {

    gps_reader::sptr
    gps_reader::make(int port)
    {
      return gnuradio::get_initial_sptr
        (new gps_reader_impl(port));
    }

    pmt::pmt_t pack_latlong(float latitude, float longitude)
    {
      pmt::pmt_t msg = pmt::make_vector(2,pmt::PMT_NIL);
      pmt::vector_set(msg,0,pmt::from_float(latitude));
      pmt::vector_set(msg,1,pmt::from_float(longitude));
      return msg;
    }
    /*
     * The private constructor
     */
    gps_reader_impl::gps_reader_impl(int port)
      : gr::sync_block("gps_reader",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, sizeof(float)))
    {
      recv_buffer = new char[recv_buffer_len];
      memset(&tcp_server_addr, 0, sizeof(tcp_server_addr));
      tcp_server_addr.sin_family = AF_INET;
      tcp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
      tcp_server_addr.sin_port = htons(port);

      clientlen = sizeof(tcp_client_addr);
      tcp_client_sock = socket(AF_INET, SOCK_STREAM, 0);
      connect(tcp_client_sock,(struct sockaddr *)&tcp_server_addr, sizeof(tcp_server_addr));
      message_port_register_out(pmt::intern("latlong"));
    }

    /*
     * Our virtual destructor.
     */
    gps_reader_impl::~gps_reader_impl()
    {
      close(tcp_client_sock);
    }

    int
    gps_reader_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      
      float * out = (float *) output_items[0];
       
        fd_set read_fds;
        struct timeval timeout;
        timeout.tv_sec = 0;
          timeout.tv_usec = 2000;
          int p = 0;
          FD_ZERO(&read_fds);
          FD_SET(tcp_client_sock, &read_fds);
          p = select(tcp_client_sock + 1, &read_fds, NULL, NULL, &timeout);
        // read data from socket - blocks until data is received
        
        if(p>0)
        {
        int recv_len = recvfrom(tcp_client_sock, recv_buffer, recv_buffer_len, 0,
                (struct sockaddr *)&tcp_server_addr, &clientlen);

        pch = strtok (recv_buffer,",");
        prev = new char[sizeof(pch)];
           while (pch != NULL)
          {
        
                if(pch[0]=='N') {
                   longitude = atof(prev);      
                      } 
                if(pch[0]=='S')
                  {
                    longitude = -1*atof(prev); 
                  }
                if(pch[0]=='E') {
                      latitude = atof(prev);      
                  }
                if(pch[0]=='W')
                  {
                      latitude = -1*atof(prev); 
                  }
    
                 memcpy(prev,pch,sizeof(pch));
                 std::cout<<pch<<"\t";
                   pch = strtok (NULL, ",");
          }

    //printf("longtitude: %f",longitude);
    //printf("latitude: %f",latitude); 
    //status = close(tcp_client_sock);
    message_port_pub(pmt::intern("latlong"),pack_latlong(latitude,longitude));
      // Tell runtime system how many output items we produced.  
        }
        memset(&out,0,sizeof(float));
    return noutput_items;
    }

  } /* namespace utils */
} /* namespace gr */

