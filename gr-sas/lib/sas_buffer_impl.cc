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
#include "sas_buffer_impl.h"
using namespace boost::interprocess;
namespace gr {
  namespace sas {

    sas_buffer::sptr
    sas_buffer::make(int N)
    {
      return gnuradio::get_initial_sptr
        (new sas_buffer_impl(N));
    }

    pmt::pmt_t pack_frequency(float cfreq)
    {
      pmt::pmt_t msg = pmt::from_float(cfreq);
      return msg;
    }

    pmt::pmt_t pack_samp_rate(float samp_rate)
    {
      pmt::pmt_t msg = pmt::from_float(samp_rate);
      return msg;
    }

    pmt::pmt_t pack_number_of_samps(float number_of_samps)
    {
      pmt::pmt_t msg = pmt::from_float(number_of_samps);
      return msg;

    }

    pmt::pmt_t pack_samp_number(float samp_number)
    {
      pmt::pmt_t msg = pmt::from_float(samp_number);
      return msg;
    }

    sas_buffer_impl::sas_buffer_impl(int N)
      : gr::sync_block("sas_buffer",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, N*sizeof(gr_complex))),d_N(N)
    {
      sample = 100000;
      frequency = 12345;
      sample_rate = 98765;
      num_samples= 45678;
      message_port_register_out(pmt::intern("center_freq"));
      message_port_register_out(pmt::intern("samp_rate"));
      message_port_register_out(pmt::intern("samp_number"));
      message_port_register_out(pmt::intern("number_of_samps"));
    }

    /*
     * Our virtual destructor.
     */
    sas_buffer_impl::~sas_buffer_impl()
    {
    }

    int
    sas_buffer_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      gr_complex *out = (gr_complex *) output_items[0];
      try{
          shared_memory_object shm (open_only, "/sas_buffer", read_only);
          mapped_region region(shm, read_only);
          memcpy(&shared_struct, region.get_address(),sizeof(shared_struct));

          std::cout << "Sample Number: " << shared_struct.sample << std::endl;
          std::cout << "Frequency: " << shared_struct.frequency << std::endl;
          std::cout << "Number of Samples: " << shared_struct.num_samples << std::endl;
          std::cout << "Sample Rate: " << shared_struct.sample_rate << std::endl;


          if(shared_struct.frequency != frequency)
          {
          message_port_pub(pmt::intern("center_freq"),pack_frequency(shared_struct.frequency));
          frequency = shared_struct.frequency;
          }
          if(shared_struct.sample != sample)
          {
          message_port_pub(pmt::intern("samp_number"),pack_samp_number(shared_struct.sample));
          sample = shared_struct.sample;
          }
          if(shared_struct.num_samples != num_samples)
          {
          message_port_pub(pmt::intern("number_of_samps"),pack_number_of_samps(shared_struct.num_samples));
          num_samples = shared_struct.num_samples;
          }
          if(shared_struct.sample_rate != sample_rate)
          {
          message_port_pub(pmt::intern("samp_rate"),pack_samp_rate(shared_struct.sample_rate));
          sample_rate = shared_struct.sample_rate;
          }
          memcpy(out,shared_struct.data,shared_struct.num_samples*sizeof(std::complex<float>));
        }
        catch(std::exception const& e)
        {
          memset(out,0,d_N*sizeof(gr_complex));
          std::cout<<"Can't read from buffer\n";
        }

      return noutput_items;
    }

  } /* namespace sas */
} /* namespace gr */
