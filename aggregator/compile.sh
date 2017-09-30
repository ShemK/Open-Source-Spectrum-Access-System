#!/bin/bash

g++ -o test test.cpp CentralRemConnector.cpp decision_maker.cc InformationParser.cpp -lpqxx -lgnuradio-pmt -std=c++11
