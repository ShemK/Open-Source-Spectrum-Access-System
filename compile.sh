#!/bin/bash

g++ -o radio_test radio_test.cpp ofdm_phy.cpp timer.cc loop.cpp  config_reader.cpp Engine.cpp CognitiveEngine.cpp -lboost_system -luhd -lliquid -lm -lc -lpthread -lrt -lz -lconfig -std=c++11
