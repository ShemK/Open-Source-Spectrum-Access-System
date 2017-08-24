#!/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir=/home/wireless/workspace/darpaSC2/rem/gr-utils/python
export PATH=/home/wireless/workspace/darpaSC2/rem/gr-utils/build/python:$PATH
export LD_LIBRARY_PATH=/home/wireless/workspace/darpaSC2/rem/gr-utils/build/lib:$LD_LIBRARY_PATH
export PYTHONPATH=/home/wireless/workspace/darpaSC2/rem/gr-utils/build/swig:$PYTHONPATH
/usr/bin/python2 /home/wireless/workspace/darpaSC2/rem/gr-utils/python/qa_reader.py 
