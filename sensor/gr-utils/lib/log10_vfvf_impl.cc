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
#include "log10_vfvf_impl.h"

namespace gr {
  namespace utils {

    log10_vfvf::sptr
    log10_vfvf::make(int n)
    {
      return gnuradio::get_initial_sptr
        (new log10_vfvf_impl(n));
    }

    /*
     * The private constructor
     */
    log10_vfvf_impl::log10_vfvf_impl(int n)
      : gr::sync_block("log10_vfvf",
              gr::io_signature::make(1, 1, n*sizeof(float)),
              gr::io_signature::make(1, 1, n*sizeof(float)))
    {
      d_pxx_in = (float*)volk_malloc(sizeof(float)*n,
                                      volk_get_alignment());
      d_pxx_out = (float*)volk_malloc(sizeof(float)*n,
                                      volk_get_alignment());
      d_tmp_buf = (float*)volk_malloc(sizeof(float)*n,
                                      volk_get_alignment());    
      dbw2m = (float*)volk_malloc(sizeof(float)*n, volk_get_alignment());

      for(int ii=0;ii<n;ii++)
        dbw2m[ii]=-30;

      this->n=n;


    }

    /*
     * Our virtual destructor.
     */
    log10_vfvf_impl::~log10_vfvf_impl()
    {
      volk_free(d_pxx_in);
      volk_free(d_pxx_out);
    }

    int
    log10_vfvf_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];
      memcpy(d_pxx_in, in, sizeof(float)*n);
      float scale=log(10.0)/log(2.0);
      int N=n/2;
      volk_32f_log2_32f(d_tmp_buf, d_pxx_in, n);
      volk_32f_s32f_multiply_32f(d_pxx_out, d_tmp_buf, scale, n);
      volk_32f_x2_add_32f(d_pxx_out, d_pxx_out, dbw2m, n);
      memcpy(&out[N],&d_pxx_out[0],sizeof(float)*N);
      memcpy(&out[0],&d_pxx_out[N],sizeof(float)*N);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace utils */
} /* namespace gr */

