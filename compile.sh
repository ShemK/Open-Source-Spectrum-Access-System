#!/bin/bash

g++ -o radio_test radio_test.cpp ofdm_phy.cpp timer.cc loop.cpp  -lboost_system -luhd -lliquid -lm -lc -lpthread -lrt -lz -std=c++11
