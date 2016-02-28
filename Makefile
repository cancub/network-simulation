CXX = g++
CXXFLAGS = -I. -std=c++11 -pthread
DEPS = host.o frames.o frame_generators.o addressing.o networking_devices.o

test: test.o $(DEPS)
	$(CXX) $(CXXFLAGS) -o test test.o $(DEPS) -I.

host: host.o frame_generators.o frames.o 
	$(CXX) $(CXXFLAGS) -o host host.o frame_generators.o frames.o -I.

frame_generators: frame_generators.o
	$(CXX) $(CXXFLAGS) -o frame_generators frame_generators.o 

clean:
	rm *.o host test *.h.gch