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

#ifndef INCLUDED_UTILS_SHMEM_READ_IMPL_H
#define INCLUDED_UTILS_SHMEM_READ_IMPL_H

#include <utils/shmem_read.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstring>
#include <cstdlib>
#include <string>
#define MAX_ESC_BUFFER 4000

namespace gr {
  namespace utils {

    class shmem_read_impl : public shmem_read
    {
    private:
      int d_N, d_num_channels;
      struct ESC_Struct{
        int sample;
        double frequency;
        double sample_rate;
        int num_samples;
        std::complex<float> data[MAX_ESC_BUFFER];
      };
      ESC_Struct shared_struct;
    public:
      shmem_read_impl(int N, int num_channels);
      ~shmem_read_impl();

      // Where all the action really happens
      int work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items);
      };

    } // namespace utils
  } // namespace gr

  #endif /* INCLUDED_UTILS_SHMEM_READ_IMPL_H */
