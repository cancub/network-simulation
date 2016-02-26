#ifndef FRAMES_H
#define FRAMES_H

#include <stdlib.h>
#include <string>

class Frame {
	public:
		Frame();
		Frame(string, string, size_t);
		string dst();
		string src();
		size_t size();
	private:
		string destination_address;
		string source_address;
		size_t size;
};

#endif