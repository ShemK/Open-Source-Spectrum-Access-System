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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "psql_insert_impl.h"

using namespace boost::posix_time;
using namespace std::chrono;
using namespace boost::interprocess;
using namespace std;
namespace gr {
  namespace sas {

    psql_insert::sptr
    psql_insert::make(int N, int num_channels)
    {
      return gnuradio::get_initial_sptr
        (new psql_insert_impl(N, num_channels));
    }

    void
    psql_insert_impl::msg_samp_rate(pmt::pmt_t msg)
    {
    bandwidth = pmt::to_float(msg);
    bwstr.str("");
    bwstr.clear();
    bwstr<<bandwidth;
    }

    void
    psql_insert_impl::gps(pmt::pmt_t msg)
    {
    latitude=pmt::to_float(pmt::vector_ref(msg,0));
		longitude = pmt::to_float(pmt::vector_ref(msg,1));
		latstr.str("");
		latstr.clear();
		longstr.str("");
		longstr.clear();
		latstr<<latitude;
		longstr<<longitude;
    }

    void
    psql_insert_impl::center_freq(pmt::pmt_t msg)
    {
      center_frequency = pmt::to_float(msg);
      cent_freq.str("");
      cent_freq.clear();
      cent_freq<<center_frequency;
    }

    void
    psql_insert_impl::decision(pmt::pmt_t msg)
    {
      for(int i=0;i<num_channels;i++)
          occ[i]=pmt::to_float(pmt::vector_ref(msg,i));
      occstr.str("");
      occstr.clear();
      occstr<<occ[0];
      //TODO add case for split psd
    }
    /*
     * The private constructor
     */
    psql_insert_impl::psql_insert_impl(int N, int num_channels)
      : gr::sync_block("psql_insert",
              gr::io_signature::make(1, 1, N*sizeof(float)),
              gr::io_signature::make(0, 0, 0))
    {
      this->N = N;
      this->num_channels = num_channels;
      c = new pqxx::connection(dbstring);
      if (c->is_open()) {
         cout << "Opened database successfully: " << c->dbname() << std::endl;
      } else {
         cout << "Can't open database" << endl;
      }

      latitude=0;
      longitude=0;
      latstr<<latitude;
      longstr<<longitude;
      bwstr<<0.0;
      occ = new float[num_channels];
      occstr<<0.0;
      //message
      message_port_register_in(pmt::mp("latlong"));
      message_port_register_in(pmt::mp("center_freq"));
      message_port_register_in(pmt::mp("decision"));
      message_port_register_in(pmt::mp("samp_rate"));
      set_msg_handler(pmt::mp("latlong"), boost::bind(&psql_insert_impl::gps, this, _1));
      set_msg_handler(pmt::mp("center_freq"), boost::bind(&psql_insert_impl::center_freq, this, _1));
      set_msg_handler(pmt::mp("decision"), boost::bind(&psql_insert_impl::decision, this, _1));
      set_msg_handler(pmt::mp("samp_rate"), boost::bind(&psql_insert_impl::msg_samp_rate, this, _1));
    }

    /*
     * Our virtual destructor.
     */
    psql_insert_impl::~psql_insert_impl()
    {
    }

    int
    psql_insert_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      pqxx::work w(*(c));
      std::vector<float> buf;
      float *   in = (float *) input_items[0];
      std::string s= date::format("%F %T\n", std::chrono::system_clock::now());

      buf.assign( in, in+N);

      std::stringstream result;
      std::copy(buf.begin(), buf.end(), std::ostream_iterator<float>(result,","));
      std::string r = result.str();
      r = r.substr(0, r.length()-1);  // get rid of the trailing space

      //std::cout << "'" << buf.size() << "'\n";

      std::string sql = "INSERT INTO SpectrumInfo (timetag, latitude, longitude, occ, center_freq, bandwidth, psd) VALUES ('"+s+"','"+latstr.str()+"','"+longstr.str()+"','"+occstr.str()+"','"+cent_freq.str()+"','"+bwstr.str()+"','{"+r+"}');";
      //std::cout<<sql;
      w.exec( sql);


      float decision = decision_maker.getDecision(occ[0],std::stod (cent_freq.str(),0));
      if(decision != -1) {
        std::cout << "Actual Channel " <<  cent_freq.str() << std::endl;
        double frequency = decision_maker.get_previous_center_frequency() - 1e6;
        sql = "UPDATE ChannelInfo SET occ = "+ std::to_string(decision)
                  + " WHERE startfreq = " + std::to_string(frequency) + ";";
        std::cout << sql << std::endl;
        w.exec( sql);
      //  w.commit();
        //printf("Center Frequency%f\n", decision_maker.get_previous_center_frequency());
        //printf("decision: %f\n", decision);

      }
      w.commit();
      // Tell runtime system how many output items we produced.
      return noutput_items;
    }
  } /* namespace sas */
} /* namespace gr */
