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

#ifndef INCLUDED_UTILS_MULTICHAN_ED_IMPL_H
#define INCLUDED_UTILS_MULTICHAN_ED_IMPL_H

#include <utils/multichan_ed.h>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/rolling_mean.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/utility.hpp>
#include <boost/mpl/clear.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"

#include <boost/interprocess/shared_memory_object.hpp>
//#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <ctime>
#include <chrono>
#include <string.h>
#include <sstream>
#include <volk/volk.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>

namespace gr {
  namespace utils {

    class multichan_ed_impl : public multichan_ed
    {
     private:
      int fft_len, num_channels, * cstates, * state;
      float threshold, sensitivity,* last_mean, * last_variance;
      std::vector<float> *oldbuf;
      float *noise_floor,*cur_noise_floor,rx_rate,wbnoise_floor,*fcstates;
      float *thresholds, *means, *variances;

     public:
      multichan_ed_impl(int fft_len, float threshold, int num_channels);
      ~multichan_ed_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);

      pmt::pmt_t pack_decision(float * fcstates, int num_channels);
      pmt::pmt_t pack_noise_floor(float * noise_floor, int num_channels);

    };

  } // namespace utils
} // namespace gr

#endif /* INCLUDED_UTILS_MULTICHAN_ED_IMPL_H */

