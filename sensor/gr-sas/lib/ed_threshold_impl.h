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

#ifndef INCLUDED_SAS_ED_THRESHOLD_IMPL_H
#define INCLUDED_SAS_ED_THRESHOLD_IMPL_H

#include <sas/ed_threshold.h>

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
  namespace sas {

    class ed_threshold_impl : public ed_threshold
    {
     private:
      int fft_len, num_channels, * cstates, * state;
      float threshold, sensitivity,* last_mean, * last_variance;
      std::vector<float> *oldbuf;
      float *noise_floor,*cur_noise_floor,rx_rate,wbnoise_floor,*fcstates;
      float *thresholds, *means, *variances;
  
     public:
      ed_threshold_impl(int fft_len, int num_channels, float threshold);
      ~ed_threshold_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
         
      pmt::pmt_t pack_decision(float * fcstates, int num_channels);
      pmt::pmt_t pack_noise_floor(float * noise_floor, int num_channels);
      
    };

  } // namespace sas
} // namespace gr

#endif /* INCLUDED_SAS_ED_THRESHOLD_IMPL_H */

