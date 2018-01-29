To run server_mac: g++ -o server server.cpp MAC.cpp tun.cpp config_reader.cpp -lrt -lpthread -lz -lconfig -std=c++11

To run client_mac: g++ -o client client.cpp MAC.cpp tun.cpp config_reader.cpp -lrt -lpthread -lz -lconfig -std=c++11 -Wall

To run phy: g++ -o radio_test radio_test.cpp ofdm_phy.cpp timer.cc -luhd -lboost_system -lliquid -lm -lc -lpthread -lrt -std=c++11


Creating Libraries

g++ -Wall -fPIC -c MAC.cpp tun.cpp -lrt -lpthread -lz -std=c++11

g++ -shared -Wl,-soname,libmactest.so.1 -o libmactest.so.1.0 *.o

sudo mv libmactest.so.1.0 /usr/bin/

sudo ln -sf /usr/lib/libmactest.so.1.0 /usr/lib/libmactest.so.1

sudo ln -sf /usr/lib/libmactest.so.1 /usr/lib/libmactest.so

sudo ldconfig

g++ -o client client.cpp -lmactest -std=c++11 -lz -lrt
