# -*- MakeFile -*-
# Tutorial on creating makefiles:
# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

GPP=g++
CFLAGS=-std=c++11 -g
LIBS=-lboost_system -luhd -lliquid -lm -lc -lpthread -lrt -lz -lconfig
OBJ_DIR=obj
LIB_DIR=lib
PHY_DEPS=$(OBJ_DIR)/radio_test.o $(OBJ_DIR)/ofdm_phy.o $(OBJ_DIR)/loop.o $(OBJ_DIR)/timer.o $(OBJ_DIR)/config_reader.o $(OBJ_DIR)/Engine.o $(OBJ_DIR)/CognitiveEngine.o
MAC_DEPS=$(OBJ_DIR)/tun.o $(OBJ_DIR)/MAC.o $(OBJ_DIR)/config_reader.o


all: directories client server radio_test

server: directories server.cpp
	$(GPP) -o server server.cpp $(MAC_DEPS) $(LIBS) $(CFLAGS)

client: directories client.cpp $(MAC_DEPS)
	$(GPP) -o client client.cpp $(MAC_DEPS) $(LIBS) $(CFLAGS)

radio_test: directories $(PHY_DEPS)
	$(GPP) -o radio_test $(PHY_DEPS) BufferQ.h $(LIBS) $(CFLAGS)

lib: directories $(MAC_DEPS)
	$(GPP) -fPIC -shared -Wl,-soname,libmactest.so.1 -o $(OBJ_DIR)/libmactest.so.1.0 $(MAC_DEPS) $(LIBS) $(CFLAGS)

$(OBJ_DIR)/tun.o:  tun.cpp tun.hpp
	$(GPP) -c -o $@ tun.cpp $(CFLAGS)

$(OBJ_DIR)/MAC.o: MAC.cpp MAC.hpp
	$(GPP) -c -o $@ MAC.cpp $(CFLAGS)

$(OBJ_DIR)/radio_test.o: radio_test.cpp
	$(GPP) -c -o $@ radio_test.cpp $(CFLAGS)

$(OBJ_DIR)/ofdm_phy.o:  ofdm_phy.cpp ofdm_phy.hpp
	$(GPP) -c -o $@ ofdm_phy.cpp $(CFLAGS)

$(OBJ_DIR)/loop.o: loop.cpp loop.hpp
	$(GPP) -c -o $@ loop.cpp $(CFLAGS)

$(OBJ_DIR)/timer.o:  timer.cc timer.h
	$(GPP) -c -o $@ timer.cc $(CFLAGS)

$(OBJ_DIR)/config_reader.o:  config_reader.cpp config_reader.hpp
	$(GPP) -c -o $@ config_reader.cpp $(CFLAGS)

$(OBJ_DIR)/Engine.o: Engine.cpp Engine.hpp
	$(GPP) -c -o $@ Engine.cpp $(CFLAGS)

$(OBJ_DIR)/CognitiveEngine.o: CognitiveEngine.cpp CognitiveEngine.hpp
	$(GPP) -c -o $@ CognitiveEngine.cpp $(CFLAGS)

clean:
	rm -f $(OBJ_DIR)/*.o radio_test client server

directories:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(LIB_DIR)
