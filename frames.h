#ifndef FRAMES_H
#define FRAMES_H

#include <stdlib.h>
#include <string>

class Frame {
	public:
		Frame();
		Frame(string, string, size_t);
		string src();
		string dst();
		size_t size();
	private:
		string source_address;
		string destination_address;
		size_t size;
};

#endif