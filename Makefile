CXX = g++
CXXFLAGS = -I. -std=c++11 -pthread

test: test.o host.o frame_generators.o frames.o
	$(CXX) $(CXXFLAGS) -o test test.o host.o frame_generators.o frames.o -I.

host: host.o frame_generators.o frames.o 
	$(CXX) $(CXXFLAGS) -o host host.o frame_generators.o frames.o -I.

clean:
	rm *.o 