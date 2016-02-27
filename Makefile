CXX=g++
CXXFLAGS=-I. -std=c++11

host: host.o frame_generators.o frames.o 
	$(CXX) $(CXXFLAGS) -o host host.o frame_generators.o frames.o -I.