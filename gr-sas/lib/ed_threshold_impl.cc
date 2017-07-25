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
#include "ed_threshold_impl.h"

using namespace boost::accumulators;
using namespace std;

namespace gr {
  namespace sas {

    template <typename InputIterator, typename OutputIterator>
    void normalizeProbability(InputIterator begin, InputIterator end, OutputIterator out) {
        typedef typename std::iterator_traits<InputIterator>::reference ref_t;
        double norm = std::accumulate(begin, end, 0.0);
        std::transform(begin, end, out, [norm](const ref_t t){ return t/norm; });
    }


    pmt::pmt_t ed_threshold_impl::pack_decision(float * fcstates, int num_channels)
    {
      pmt::pmt_t msg = pmt::make_vector(num_channels,pmt::PMT_NIL);
      for(int i=0;i<num_channels;i++)
      pmt::vector_set(msg,i,pmt::from_float(fcstates[i]));
      
      return msg;
    }
    ed_threshold::sptr
    ed_threshold::make(int fft_len, int num_channels, float threshold)
    {
      return gnuradio::get_initial_sptr
        (new ed_threshold_impl(fft_len, num_channels, threshold));
    }

    /*
     * The private constructor
     */
    ed_threshold_impl::ed_threshold_impl(int fft_len, int num_channels, float threshold)
      : gr::sync_block("ed_threshold",
              gr::io_signature::make(1, 1, fft_len*sizeof(float)),
              gr::io_signature::make(1, 1, fft_len*sizeof(float)))
    {
      this->fft_len = fft_len;
      this->num_channels = num_channels;
      this->threshold = threshold;

      cstates =  new int[num_channels]; 
      fcstates =  new float[num_channels]; 
      state =  new int[num_channels];
      last_mean = new float[num_channels];
      last_variance = new float[num_channels];
      thresholds = new float[num_channels];
      means = new float[num_channels];
      variances = new float[num_channels];
      oldbuf =  new std::vector<float>[num_channels];
      noise_floor = new float[num_channels];
      cur_noise_floor = new float[num_channels];
      for(int i=0;i<num_channels;i++)
      {
          oldbuf[i].resize(fft_len);
          noise_floor[i] = 100;
          cur_noise_floor[i] = 100;
      }
      wbnoise_floor=100;
      message_port_register_out(pmt::mp("decision"));
      message_port_register_out(pmt::mp("noise_floor"));
    }

    /*
     * Our virtual destructor.
     */
    ed_threshold_impl::~ed_threshold_impl()
    {
    }

    int
    ed_threshold_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];

      std::vector<float> buf[num_channels];
      float noise_floor[num_channels];
      for (int i=0; i<num_channels; i++)
      {	
        
        accumulator_set<float, stats<tag::count, tag::max, tag::mean, tag::variance> > acc;
        
        buf[i].assign(in, in + (i+1)*fft_len/num_channels);

        volk_32f_s32f_calc_spectral_noise_floor_32f(&cur_noise_floor[i], &buf[i][0],15, fft_len);
        
        wbnoise_floor = (cur_noise_floor[i] < wbnoise_floor || wbnoise_floor < -150) ? cur_noise_floor[i] :  wbnoise_floor;
        noise_floor[i] = (cur_noise_floor[i] - wbnoise_floor > 15 || cur_noise_floor[i]< -200) ? wbnoise_floor :  cur_noise_floor[i];
        //noise_floor[i] = (cur_noise_floor[i] - cur_noise_floor[i-1] < 10 || cur_noise_floor[i] - cur_noise_floor[i+1] < 10) ? cur_noise_floor[i] :  std::min(cur_noise_floor[i-1],cur_noise_floor[i+1]);
        
        acc=for_each(buf[i].begin(), buf[i].end(),acc);
        means[i]=mean(acc);
        variances[i]=variance(acc);

        if(means[i]-noise_floor[i]> threshold && means[i]>-150.0)
          {
            fcstates[i]=1;
            continue;
          }
        
        fcstates[i] = ((means[i]-noise_floor[i])/std::fabs(noise_floor[i]) > 0.0) ? (means[i]-noise_floor[i])/std::fabs(noise_floor[i]) : 0.0;
        //channelstate ranking as quality
        
      }

      message_port_pub(pmt::intern("decision"), pack_decision(fcstates, num_channels));
      message_port_pub(pmt::intern("noise_floor"), pmt::from_float(noise_floor[num_channels/2]));
      //TODO: EXTEND TO SUBCHANNEL SENSING
      memcpy(out,in, fft_len*sizeof(float) );
      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace sas */
} /* namespace gr */

