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
#include "pipe_sink_impl.h"

namespace gr {
  namespace utils {

    pipe_sink::sptr
    pipe_sink::make(int N, const char *filename)
    {
      return gnuradio::get_initial_sptr
        (new pipe_sink_impl(N, filename));
    }

    /*
     * The private constructor
     */
    pipe_sink_impl::pipe_sink_impl(int N, const char *filename)
      : gr::sync_block("pipe_sink",
              gr::io_signature::make(1, 1, N * sizeof(float)),
              gr::io_signature::make(0, 0, 0))
    {
        this->N = N;
        mkfifo(filename, 1500);
        fd = open(filename, O_WRONLY);
    }

    /*
     * Our virtual destructor.
     */
    pipe_sink_impl::~pipe_sink_impl()
    {
    }

    int
    pipe_sink_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      int x =  write(fd, in, N*sizeof(float));
      std::cout<<"\n"<<x<<"\n";
      return noutput_items;
    }

  } /* namespace utils */
} /* namespace gr */

