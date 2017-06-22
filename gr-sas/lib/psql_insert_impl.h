/* -*- c++ -*- */
/* 
 * Copyright 2017 A. S. Jauhar, ahmadsj@vt.edu
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

#ifndef INCLUDED_SAS_PSQL_INSERT_IMPL_H
#define INCLUDED_SAS_PSQL_INSERT_IMPL_H


#include <sas/psql_insert.h>
#include <sas/date.h>
#include <pqxx/pqxx>

#include <algorithm>
#include <sstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <string.h>
#include <chrono>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/utility.hpp>
#include <boost/mpl/clear.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"

namespace gr {
  namespace sas {

    class psql_insert_impl : public psql_insert
    {
     private:
      std::vector<tag_t> d_tags;
      std::vector<tag_t>::iterator d_tags_itr;
      pmt::pmt_t d_filter;
      int N, num_channels, * cstates, * state;
      float threshold, sensitivity,* last_mean, * last_variance, *occ;
      std::vector<float> *oldbuf;
      float *noise_floor,*cur_noise_floor,rx_rate,wbnoise_floor,*fcstates;
	    float *thresholds, *means, *variances;
      std::string dbstring = "dbname = rem user = wireless password = wireless hostaddr = 127.0.0.1 port = 5432";

      pqxx::connection *c;
      
      float latitude, longitude, center_frequency;
      std::ostringstream latstr,longstr, cent_freq, occstr;

     public:
      psql_insert_impl(int N, int num_channels);
      ~psql_insert_impl();

      void gps(pmt::pmt_t msg);

      void center_freq(pmt::pmt_t msg);

      void decision(pmt::pmt_t msg);
      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace sas
} // namespace gr

#endif /* INCLUDED_SAS_PSQL_INSERT_IMPL_H */

