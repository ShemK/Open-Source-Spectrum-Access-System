/* -*- c++ -*- */

#define SAS_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "sas_swig_doc.i"

%{
#include "sas/psql_insert.h"
#include "sas/sas_buffer.h"
#include "sas/ed_threshold.h"
%}


%include "sas/psql_insert.h"
GR_SWIG_BLOCK_MAGIC2(sas, psql_insert);
%include "sas/sas_buffer.h"
GR_SWIG_BLOCK_MAGIC2(sas, sas_buffer);

%include "sas/ed_threshold.h"
GR_SWIG_BLOCK_MAGIC2(sas, ed_threshold);

