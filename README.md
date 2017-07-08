# Open-Source-Spectrum-Access-System
Open Source Spectrum Access System

REM Connector

This is meant to connect the central REM to the information from other distributed sensors.

The distributed sensors will be writted in either python or c++.

To compile: g++ -o test test.cpp CentralRemConnector.cpp -lpqxx -lgnuradio-pmt -std=c++11

To test if it works, run the python script to send information
