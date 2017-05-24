To run mac: g++ -o main mac_main.cpp MAC.cpp -lrt -lpthread -std=c++11

To run phy: g++ -o radio_test radio_test.cpp ofdm_phy.cpp timer.cc -luhd -lliquid -lm -lc -lpthread -lrt -std=c++11
