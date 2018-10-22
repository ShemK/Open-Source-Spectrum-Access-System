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
#include "shmem_write_impl.h"
using namespace boost::interprocess;
namespace gr {
  namespace utils {

    shmem_write::sptr
    shmem_write::make(int N, int num_channels)
    {
      return gnuradio::get_initial_sptr
        (new shmem_write_impl(N, num_channels));
    }

    /*
     * The private constructor
     */
    shmem_write_impl::shmem_write_impl(int N, int num_channels)
      : gr::sync_block("shmem_write",
              gr::io_signature::make(1, num_channels, N*sizeof(float)),
              gr::io_signature::make(0, 0, 0)),d_N(N),d_num_channels(num_channels)
    {
      
      /* struct shm_remove
      {
         shm_remove() { shared_memory_object::remove("buffer"); }
         ~shm_remove(){ shared_memory_object::remove("buffer"); }
      } remover;
      
      shared_memory_object shm (open_or_create, "buffer", read_write);
      shm.truncate(d_N*sizeof(float));
      mapped_region region(shm, read_write);
      */
    }

    /*
     * Our virtual destructor.
     */
    shmem_write_impl::~shmem_write_impl()
    {
      /*
      std::cout<<"destroying everything";
      shared_memory_object shm (open_only, "buffer", read_write);
      mapped_region region(shm, read_write);
      std::memset(region.get_address(), -500.0, region.get_size());
      shared_memory_object::remove("buffer");
      */
    }

    int
    shmem_write_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      float *in[d_num_channels];

      for (int i=0; i<d_num_channels; i++)
  	  {
	    in[i] = (float *) input_items[i];
		  }

      shared_memory_object shm (open_or_create, "buffer", read_write);
      shm.truncate(d_num_channels*d_N*sizeof(float));
      mapped_region region(shm, read_write);
      for(int i=0;i<d_num_channels;i++)
        memcpy(region.get_address()+(d_N*i*sizeof(float)),in[i], d_N*sizeof(float) );

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace utils */
} /* namespace gr */

