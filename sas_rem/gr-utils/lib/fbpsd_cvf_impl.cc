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
#include "fbpsd_cvf_impl.h"
#include <volk/volk.h>
#include <cmath>

namespace gr {
  namespace utils {

    fbpsd_cvf::sptr
    fbpsd_cvf::make(int fft_len, double samp_rate, int window_type, float alpha, int num_channels)
    {
      return gnuradio::get_initial_sptr
        (new fbpsd_cvf_impl(fft_len, samp_rate, window_type, alpha, num_channels));
    }

    /*
     * The private constructor
     */
    fbpsd_cvf_impl::fbpsd_cvf_impl(int fft_len, double samp_rate, int window_type, float alpha, int num_channels)
      : gr::sync_decimator("fbpsd_cvf",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, num_channels, (fft_len/num_channels)*sizeof(float)), (fft_len/num_channels))
    {
      d_samp_rate = samp_rate;
      d_fft_len = fft_len;
      d_window_type = (filter::firdes::win_type)window_type;
      d_alpha = alpha;
      d_num_channels = num_channels;

      d_tmpbuf = static_cast<float *>(volk_malloc(
              sizeof(float) * d_fft_len, volk_get_alignment()));
      d_tmp_pxx = static_cast<float *>(volk_malloc(
              sizeof(float) * d_fft_len, volk_get_alignment()));
      d_pxx = static_cast<float *>(volk_malloc(
              sizeof(float) * d_fft_len, volk_get_alignment()));
      d_pxx_out = (float*)volk_malloc(sizeof(float)*d_fft_len,
                                      volk_get_alignment());
      d_fft = new fft::fft_complex(fft_len, true);

      d_avg_filter.resize(d_fft_len);
      build_window();
      for(unsigned int i = 0; i < d_fft_len; i++) {
        d_avg_filter[i].set_taps(d_alpha);
        //std::cout<<type(d_avg_filter[i])<<"\n";
      }
      set_decimation(fft_len);

    }

    /*
     * Our virtual destructor.
     */
    fbpsd_cvf_impl::~fbpsd_cvf_impl()
    {
      delete d_fft;
      volk_free(d_tmpbuf);
      volk_free(d_tmp_pxx);
      volk_free(d_pxx);
      volk_free(d_pxx_out);
    }


    void
    fbpsd_cvf_impl::set_fft_len(int fft_len)  {
      fbpsd_cvf_impl::d_fft_len = fft_len;
      delete d_fft;
      volk_free(d_tmpbuf);
      volk_free(d_tmp_pxx);
      volk_free(d_pxx);
      volk_free(d_pxx_out);
      d_fft = new fft::fft_complex(fft_len, true);
      d_tmpbuf = static_cast<float *>(volk_malloc(
              sizeof(float) * d_fft_len, volk_get_alignment()));
      d_tmp_pxx = static_cast<float *>(volk_malloc(
              sizeof(float) * d_fft_len, volk_get_alignment()));
      d_pxx = static_cast<float *>(volk_malloc(
              sizeof(float) * d_fft_len, volk_get_alignment()));
      d_pxx_out = (float*)volk_malloc(sizeof(float)*d_fft_len,
                                      volk_get_alignment());
      d_avg_filter.resize(d_fft_len);
      build_window();
      for(unsigned int i = 0; i < d_fft_len; i++) {
        d_avg_filter[i].set_taps(d_alpha);
      }
      set_decimation(fft_len);
    }

     void
    fbpsd_cvf_impl::set_window_type(int window)  {
      fbpsd_cvf_impl::d_window_type = static_cast<filter::firdes::win_type>(window);
      build_window();
    }

     void fbpsd_cvf_impl::periodogram(float *pxx, const gr_complex *signal)
    {
      if (d_window.size()) {
        // window signal
        volk_32fc_32f_multiply_32fc(d_fft->get_inbuf(), signal,
                                    &d_window.front(), d_fft_len);
      }
      else {
        // don't window signal
        memcpy(d_fft->get_inbuf(), signal,
               sizeof(gr_complex) * d_fft_len);
      }

      d_fft->execute(); // fft

      // calc fft to periodogram
      volk_32fc_s32f_x2_power_spectral_density_32f(pxx, d_fft->get_outbuf(),
      d_fft_len, 1.0, d_fft_len);


      // do fftshift
      d_tmpbuflen = static_cast<unsigned int>(floor(d_fft_len / 2.0));
      memcpy(d_tmpbuf, &pxx[0], sizeof(float) * (d_tmpbuflen + 1));
      memcpy(&pxx[0], &pxx[d_fft_len - d_tmpbuflen],
             sizeof(float) * (d_tmpbuflen));
      memcpy(&pxx[d_tmpbuflen], d_tmpbuf,
             sizeof(float) * (d_tmpbuflen + 1));
    }

    //build freq vector for when samp_rate != fft_len
    std::vector<float> fbpsd_cvf_impl::build_freq()
    {
      std::vector<float> freq(d_fft_len);
      double point = -d_samp_rate / 2;
      for (unsigned int i = 0; i < d_fft_len; i++) {
        freq[i] = point;
        point += d_samp_rate / d_fft_len;
      }
      return freq;
    }
    //build window for fft
    void
    fbpsd_cvf_impl::build_window() {
      d_window.clear();
      if (d_window_type != filter::firdes::WIN_NONE) {
        d_window = filter::firdes::window(d_window_type, d_fft_len,
                                          6.76);
      }
    }



    int
    fbpsd_cvf_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
     const gr_complex *in = (const gr_complex *) input_items[0];
     float *out[d_num_channels];
     
     for(int i=0;i<d_num_channels;i++)
            out[i] = (float *) output_items[i];
            
            
      d_freq = build_freq();
      periodogram(d_pxx, in);

      // smoothing of periodogram = psd
      for(int i = 0; i < d_fft_len; i++) {
        d_pxx_out[i] = d_avg_filter[i].filter(d_pxx[i]);
      }

      for(int i=0;i<d_num_channels;i++)
      memcpy(out[i], &d_pxx_out[i*(d_fft_len/d_num_channels)], (d_fft_len/d_num_channels) * sizeof(float));


      return noutput_items;
    }

  } /* namespace utils */
} /* namespace gr */

