To run server_mac: g++ -o server server.cpp MAC.cpp tun.cpp -lrt -lpthread -lz -std=c++11

To run client_mac: g++ -o client client.cpp MAC.cpp tun.cpp -lrt -lpthread -lz -std=c++11

To run phy: g++ -o radio_test radio_test.cpp ofdm_phy.cpp timer.cc -luhd -lliquid -lm -lc -lpthread -lrt -std=c++11
