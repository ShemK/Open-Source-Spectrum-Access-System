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

#ifndef INCLUDED_UTILS_CSTATES_READ_IMPL_H
#define INCLUDED_UTILS_CSTATES_READ_IMPL_H

#include <utils/cstates_read.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
//#include <boost/interprocess/windows_shared_memory.hpp>
#include <cstring>
#include <cstdlib>
#include <string>

namespace gr {
  namespace utils {

    class cstates_read_impl : public cstates_read
    {
     private:
      int d_num_channels;

     public:
      cstates_read_impl(int num_channels);
      ~cstates_read_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace utils
} // namespace gr

#endif /* INCLUDED_UTILS_CSTATES_READ_IMPL_H */

