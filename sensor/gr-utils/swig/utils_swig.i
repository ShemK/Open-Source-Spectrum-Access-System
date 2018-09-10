/* -*- c++ -*- */

#define UTILS_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "utils_swig_doc.i"

%{
#include "utils/log10_vfvf.h"
#include "utils/psd_cvf.h"
#include "utils/pipe_sink.h"
#include "utils/shmem_write.h"
#include "utils/shmem_read.h"
#include "utils/reader.h"
#include "utils/gps_reader.h"
#include "utils/fbpsd_cvf.h"
#include "utils/cstates_read.h"
#include "utils/liquid_buffer.h"
#include "utils/multichan_ed.h"
%}


%include "utils/log10_vfvf.h"
GR_SWIG_BLOCK_MAGIC2(utils, log10_vfvf);
%include "utils/psd_cvf.h"
GR_SWIG_BLOCK_MAGIC2(utils, psd_cvf);

%include "utils/pipe_sink.h"
GR_SWIG_BLOCK_MAGIC2(utils, pipe_sink);
%include "utils/shmem_write.h"
GR_SWIG_BLOCK_MAGIC2(utils, shmem_write);
%include "utils/shmem_read.h"
GR_SWIG_BLOCK_MAGIC2(utils, shmem_read);
%include "utils/reader.h"
GR_SWIG_BLOCK_MAGIC2(utils, reader);
%include "utils/gps_reader.h"
GR_SWIG_BLOCK_MAGIC2(utils, gps_reader);

%include "utils/fbpsd_cvf.h"
GR_SWIG_BLOCK_MAGIC2(utils, fbpsd_cvf);
%include "utils/cstates_read.h"
GR_SWIG_BLOCK_MAGIC2(utils, cstates_read);

%include "utils/liquid_buffer.h"
GR_SWIG_BLOCK_MAGIC2(utils, liquid_buffer);
%include "utils/multichan_ed.h"
GR_SWIG_BLOCK_MAGIC2(utils, multichan_ed);
