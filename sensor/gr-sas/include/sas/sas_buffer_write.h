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


#ifndef INCLUDED_SAS_SAS_BUFFER_WRITE_H
#define INCLUDED_SAS_SAS_BUFFER_WRITE_H

#include <sas/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace sas {

    /*!
     * \brief <+description of block+>
     * \ingroup sas
     *
     */
    class SAS_API sas_buffer_write : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<sas_buffer_write> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of sas::sas_buffer_write.
       *
       * To avoid accidental use of raw pointers, sas::sas_buffer_write's
       * constructor is in a private implementation
       * class. sas::sas_buffer_write::make is the public interface for
       * creating new instances.
       */
      static sptr make(int N);
    };

  } // namespace sas
} // namespace gr

#endif /* INCLUDED_SAS_SAS_BUFFER_WRITE_H */

