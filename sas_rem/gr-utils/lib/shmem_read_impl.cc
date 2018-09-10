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
#include "shmem_read_impl.h"
using namespace boost::interprocess;
namespace gr {
  namespace utils {

    shmem_read::sptr
    shmem_read::make(int N, int num_channels)
    {
      return gnuradio::get_initial_sptr
        (new shmem_read_impl(N, num_channels));
    }

    /*
     * The private constructor
     */
    shmem_read_impl::shmem_read_impl(int N, int num_channels)
      : gr::sync_block("shmem_read",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, num_channels, N*sizeof(float))),d_N(N), d_num_channels(num_channels)
    {

    }

    /*
     * Our virtual destructor.
     */
    shmem_read_impl::~shmem_read_impl()
    {
    }

    int
    shmem_read_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      float *out[d_num_channels];
      for (int i=0; i<d_num_channels; i++)
  	  {
	    out[i] = (float *) output_items[i];
		  }
      try{
      shared_memory_object shm (open_only, "buffer", read_only);
      mapped_region region(shm, read_only);
      for(int i=0;i<d_num_channels;i++)
      memcpy(out[i],region.get_address()+(d_N*i*sizeof(float)),d_N*sizeof(float));
      }
      catch(std::exception const& e)
      {
        for(int i=0;i<d_num_channels;i++)
          memset(out[i],-200,d_N*sizeof(float));
      }
      // Do <+signal processing+>

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace utils */
} /* namespace gr */

