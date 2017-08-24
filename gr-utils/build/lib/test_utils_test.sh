#!/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir=/home/wireless/workspace/darpaSC2/rem/gr-utils/lib
export PATH=/home/wireless/workspace/darpaSC2/rem/gr-utils/build/lib:$PATH
export LD_LIBRARY_PATH=/home/wireless/workspace/darpaSC2/rem/gr-utils/build/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$PYTHONPATH
test-utils 
