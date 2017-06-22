#!/bin/sh
export VOLK_GENERIC=1
export GR_DONT_LOAD_PREFS=1
export srcdir=/home/wireless/git/Open-Source-Spectrum-Access-System/gr-sas/lib
export PATH=/home/wireless/git/Open-Source-Spectrum-Access-System/gr-sas/build/lib:$PATH
export LD_LIBRARY_PATH=/home/wireless/git/Open-Source-Spectrum-Access-System/gr-sas/build/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$PYTHONPATH
test-sas 
