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
#include "liquid_buffer_impl.h"


using namespace boost::interprocess;

namespace gr {
  namespace utils {

    liquid_buffer::sptr
    liquid_buffer::make(int N)
    {
      return gnuradio::get_initial_sptr
        (new liquid_buffer_impl(N));
    }


    pmt::pmt_t liquid_buffer_impl::pack_frequency(float cfreq)
    {
      pmt::pmt_t msg = pmt::from_float(cfreq);
      return msg;
    }

    pmt::pmt_t liquid_buffer_impl::pack_samp_rate(float samp_rate)
    {
      pmt::pmt_t msg = pmt::from_float(samp_rate);
      return msg;
    }

    pmt::pmt_t liquid_buffer_impl::pack_number_of_samps(float number_of_samps)
    {
      pmt::pmt_t msg = pmt::from_float(number_of_samps);
      return msg;
      
    }

    pmt::pmt_t liquid_buffer_impl::pack_samp_number(float samp_number)
    {
      pmt::pmt_t msg = pmt::from_float(samp_number);
      return msg;
    }
    /*
     * The private constructor
     */
    liquid_buffer_impl::liquid_buffer_impl(int N)
      : gr::sync_block("liquid_buffer",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, N*sizeof(gr_complex))),d_N(N)
    {
      sample = 0;
      frequency = 1e9;
      sample_rate = 4e6;
      num_samples= 200;
      memset(buf_struct.data,-200.0,num_samples*sizeof(gr_complex));
      memset(shared_struct.data,-200.0,num_samples*sizeof(gr_complex));
      message_port_register_out(pmt::intern("center_freq"));
      message_port_register_out(pmt::intern("samp_rate"));
      message_port_register_out(pmt::intern("samp_number"));
      message_port_register_out(pmt::intern("number_of_samps"));
    }

    /*
     * Our virtual destructor.
     */
    liquid_buffer_impl::~liquid_buffer_impl()
    {
    }

    int
    liquid_buffer_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
          gr_complex *out = (gr_complex *) output_items[0];

          //open shared memory buffer, copy
        
          shared_memory_object shm (open_only, "liquid_buffer", read_only);
          mapped_region region(shm, read_only);          
          memcpy(&shared_struct, region.get_address(),sizeof(shared_struct));
         
          //check if metadata changed
          if(shared_struct.frequency != frequency)
          {
          message_port_pub(pmt::intern("center_freq"),liquid_buffer_impl::pack_frequency(shared_struct.frequency));
          }   
          if(shared_struct.sample_rate != sample_rate)
          {
          message_port_pub(pmt::intern("samp_rate"),liquid_buffer_impl::pack_samp_rate(shared_struct.sample_rate));
          }
          if(shared_struct.num_samples != num_samples)
          {
          }

          //copy values from liquid
          num_samples = shared_struct.num_samples;
          sample_rate = shared_struct.sample_rate;
          frequency = shared_struct.frequency;
          //message_port_pub(pmt::intern("number_of_samps"),liquid_buffer_impl::pack_number_of_samps(shared_struct.num_samples));

          //if new sample, copy data
          if(shared_struct.sample != sample && num_samples>=d_N)
          {
          message_port_pub(pmt::intern("samp_number"),liquid_buffer_impl::pack_samp_number(shared_struct.sample));
          sample = shared_struct.sample;
          memcpy(&buf_struct, region.get_address(), sizeof(buf_struct));
          memcpy(out,shared_struct.data,d_N*sizeof(std::complex<float>));   
          }
          else
          {
          memcpy(out,buf_struct.data,d_N*sizeof(std::complex<float>));
          }

      return noutput_items;
    }

  } /* namespace utils */
} /* namespace gr */

