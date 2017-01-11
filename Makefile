CXX = g++
CXXFLAGS = -I. -std=c++11 -pthread
DEPS = host.o md5.o pdu.o frame_generators.o addressing.o data_links.o networking_devices.o l4_protocols.o l3_protocols.o sim_tcpdump.o wqueue.h

test: test.o $(DEPS)
	$(CXX) $(CXXFLAGS) -o test test.o $(DEPS) -I.

host: host.o frame_generators.o frames.o 
	$(CXX) $(CXXFLAGS) -o host host.o frame_generators.o frames.o -I.

frame_generators: frame_generators.o
	$(CXX) $(CXXFLAGS) -o frame_generators frame_generators.o 

clean:
	rm -f *.o *.h.gch test host frame_generators