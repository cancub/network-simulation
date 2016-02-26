#include <stdlib.h>
#include <string>
#include "frames.h"


Frame::Frame() {
	source_address = "";
	destination_address = "";
	size = 0;
}

Frame::Frame(string src, string dest, size_t length) {
	source_address = src;
	destination_address = dst;
	size = length;
}

string Frame::src() { return source_address; }

string Frame::dst() { return destination_address; }
		
size_t Frame::size() {return size;}