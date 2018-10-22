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
#include "cstates_read_impl.h"
 using namespace boost::interprocess;
namespace gr {
  namespace utils {

    cstates_read::sptr
    cstates_read::make(int num_channels)
    {
      return gnuradio::get_initial_sptr
        (new cstates_read_impl(num_channels));
    }

    /*
     * The private constructor
     */
    cstates_read_impl::cstates_read_impl(int num_channels)
      : gr::sync_block("cstates_read",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, num_channels*sizeof(float))),d_num_channels(num_channels)
    {}

    /*
     * Our virtual destructor.
     */
    cstates_read_impl::~cstates_read_impl()
    {
    }

    int
    cstates_read_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      float *out = (float *) output_items[0];
      try{
      shared_memory_object shm (open_only, "cstates", read_only);
      //windows_shared_memory shm(open_only, "cstates", read_only);
      mapped_region region(shm, read_only);
      memcpy(out,region.get_address(),d_num_channels*sizeof(float));
      
      }
      catch(std::exception const& e)
      {
        memset(out,0,d_num_channels*sizeof(float));

      }
      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace utils */
} /* namespace gr */

