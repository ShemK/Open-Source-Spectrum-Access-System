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

#ifndef INCLUDED_UTILS_PSD_CVF_IMPL_H
#define INCLUDED_UTILS_PSD_CVF_IMPL_H

#include <utils/psd_cvf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/fft/fft.h>
#include <gnuradio/filter/single_pole_iir.h>

namespace gr {
  namespace utils {

    class psd_cvf_impl : public psd_cvf
    {
     private:
      int d_fft_len;
      unsigned int d_tmpbuflen;
      float d_alpha;
      float * d_pxx, * d_pxx_out, *d_tmpbuf, *d_tmp_pxx;
      double d_samp_rate;

      std::vector<filter::single_pole_iir<float,float,double> > d_avg_filter;
      filter::firdes::win_type d_window_type;
      std::vector<float> d_window;
      fft::fft_complex *d_fft;
      std::vector<float> d_freq;

     public:
      psd_cvf_impl(double samp_rate, int fft_len, int window_type, float alpha);
      ~psd_cvf_impl();

      void build_window();

      std::vector<float> build_freq();

      void periodogram(float *pxx, const gr_complex *signal);

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);

      void msg_samp_rate(pmt::pmt_t msg);   
      void set_samp_rate(double d_samp_rate)
      {
        psd_cvf_impl::d_samp_rate  = d_samp_rate;
      }

      void set_fft_len(int fft_len);
      void set_window_type(int d_window);

      void set_average(float d_alpha) {
        psd_cvf_impl::d_alpha = d_alpha;
        for(unsigned int i = 0; i < d_fft_len; i++) {
          d_avg_filter[i].set_taps(d_alpha);
        }
      }
    };

  } // namespace utils
} // namespace gr

#endif /* INCLUDED_UTILS_PSD_CVF_IMPL_H */

