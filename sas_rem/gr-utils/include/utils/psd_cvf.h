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


#ifndef INCLUDED_UTILS_PSD_CVF_H
#define INCLUDED_UTILS_PSD_CVF_H

#include <utils/api.h>
#include <gnuradio/sync_decimator.h>

namespace gr {
  namespace utils {

    /*!
     * \brief <+description of block+>
     * \ingroup utils
     *
     */
    class UTILS_API psd_cvf : virtual public gr::sync_decimator
    {
     public:
      typedef boost::shared_ptr<psd_cvf> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of utils::psd_cvf.
       *
       * To avoid accidental use of raw pointers, utils::psd_cvf's
       * constructor is in a private implementation
       * class. utils::psd_cvf::make is the public interface for
       * creating new instances.
       */
      static sptr make(double samp_rate, int fft_len, int window_type, float alpha);

      virtual void set_samp_rate(double d_samp_rate) = 0;
      virtual void set_fft_len(int fft_len) = 0;

      virtual void set_window_type(int d_window) = 0;
      virtual void set_average(float d_average) = 0;
    };

  } // namespace utils
} // namespace gr

#endif /* INCLUDED_UTILS_PSD_CVF_H */

