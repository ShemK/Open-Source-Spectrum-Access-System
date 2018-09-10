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


#ifndef INCLUDED_UTILS_LOG10_VFVF_H
#define INCLUDED_UTILS_LOG10_VFVF_H

#include <utils/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace utils {

    /*!
     * \brief <+description of block+>
     * \ingroup utils
     *
     */
    class UTILS_API log10_vfvf : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<log10_vfvf> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of utils::log10_vfvf.
       *
       * To avoid accidental use of raw pointers, utils::log10_vfvf's
       * constructor is in a private implementation
       * class. utils::log10_vfvf::make is the public interface for
       * creating new instances.
       */
      static sptr make(int n);
    };

  } // namespace utils
} // namespace gr

#endif /* INCLUDED_UTILS_LOG10_VFVF_H */

