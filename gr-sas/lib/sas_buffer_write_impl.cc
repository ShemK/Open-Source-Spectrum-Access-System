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
#include "sas_buffer_write_impl.h"

using namespace boost::interprocess;
namespace gr {
  namespace sas {

    sas_buffer_write::sptr
    sas_buffer_write::make(int N)
    {
      return gnuradio::get_initial_sptr
        (new sas_buffer_write_impl(N));
    }
    
    void sas_buffer_write_impl::handle_frequency(pmt::pmt_t msg)
    {
      shared_struct.frequency = pmt::to_double(msg);
      std::cout<<"Receiving data at center frequency of "<<shared_struct.frequency<<std::endl;
    }

    void sas_buffer_write_impl::handle_samp_rate(pmt::pmt_t msg)
    {
      shared_struct.sample_rate = pmt::to_float(msg);
      std::cout<<"Receiving data at bandwidth of "<<shared_struct.sample_rate<<std::endl;

    }
    
    sas_buffer_write_impl::sas_buffer_write_impl(int N)
      : gr::sync_block("sas_buffer_write",
              gr::io_signature::make(1, 1, N*sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0)),d_N(N)
    {
      shared_struct.sample = nitems_read(0);
      shared_struct.frequency = 1e9;
      shared_struct.sample_rate = 4e6;
      shared_struct.num_samples= N;
      message_port_register_in(pmt::intern("center_freq"));
      message_port_register_in(pmt::intern("samp_rate"));
      set_msg_handler(pmt::mp("center_freq"), boost::bind(&sas_buffer_write_impl::handle_frequency, this, _1));
      set_msg_handler(pmt::mp("samp_rate"), boost::bind(&sas_buffer_write_impl::handle_samp_rate, this, _1));

    }

    /*
     * Our virtual destructor.
     */
    sas_buffer_write_impl::~sas_buffer_write_impl()
    {
    }

    int
    sas_buffer_write_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      size_t abs_N, end_N;
      abs_N = nitems_read(0);
      end_N = abs_N + (uint64_t)(noutput_items);
      d_tags.clear();
      get_tags_in_range(d_tags, 0, abs_N, end_N);
          
      

      for(d_tags_itr = d_tags.begin(); d_tags_itr != d_tags.end(); d_tags_itr++) {

          if(pmt::symbol_to_string(d_tags_itr->key) == "rx_rate")
            {         
                  shared_struct.sample_rate=pmt::to_float(d_tags_itr->value);
            } 

          if(pmt::symbol_to_string(d_tags_itr->key) == "rx_freq")
            {         
                  shared_struct.frequency=pmt::to_float(d_tags_itr->value);
            }   
      }


      try{
          shared_memory_object shm (open_or_create, "sas_buffer", read_write);
          shm.truncate(sizeof(shared_struct));
          mapped_region region (shm, read_write);

          shared_struct.sample = nitems_read(0);
          memcpy(shared_struct.data,in,shared_struct.num_samples*sizeof(std::complex<float>));
          memcpy(region.get_address(), &shared_struct,sizeof(shared_struct));
        }
        catch(std::exception const& e)
        {
          memset(shared_struct.data,-200,d_N*sizeof(gr_complex));
          std::cout<<"No data\n";
        }

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace sas */
} /* namespace gr */

