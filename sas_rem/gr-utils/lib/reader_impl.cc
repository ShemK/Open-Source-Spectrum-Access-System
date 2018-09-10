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
#include "reader_impl.h"

namespace gr {
  namespace utils {

    reader::sptr
    reader::make()
    {
      return gnuradio::get_initial_sptr
        (new reader_impl());
    }
    /*
     std::map<std::string, std::string> mappify1(std::string const& s)
    {
    std::map<std::string, std::string> m;

    std::string key, val;
    std::istringstream iss(s);

    while(std::getline(std::getline(iss, key, ':') >> std::ws, val))
        m[key] = val;

    return m;
    }
    */
    void reader_impl::print(pmt::pmt_t msg)
    {
      std::cout << "******* MESSAGE DEBUG PRINT ********\n";

	  std::cout<<"\nDictionary Keys: "<<pmt::dict_keys(msg)<<"\n";
    std::cout<<"\nDictionary Values: "<<pmt::dict_values(msg)<<"\n";
    //auto m = mappify1(pmt::symbol_to_string(msg));
    //for(auto const& p: m)
    //    std::cout << '{' << p.first << " => " << p.second << '}' << '\n';
	  pmt::print(msg);
      std::cout << "************************************\n";
    }

   

    /*
     * The private constructor
     */
    reader_impl::reader_impl()
      : gr::sync_block("reader",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0))
    {
      message_port_register_in(pmt::mp("print"));
    set_msg_handler(pmt::mp("print"), boost::bind(&reader_impl::print, this, _1));
    }

    /*
     * Our virtual destructor.
     */
    reader_impl::~reader_impl()
    {
    }

    int
    reader_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      std::cout<<"\n"<<nitems_read(0);
      // Do <+signal processing+>

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace utils */
} /* namespace gr */

