#ifndef FRAMES_H
#define FRAMES_H

#include <stdlib.h>
#include <string>

class Frame {
	public:
		Frame();
		Frame(std::string);
		Frame(std::string, size_t);
		Frame(std::string, std::string, size_t);
		~Frame();
		std::string get_src_mac();
		std::string get_dst_mac();
		std::string get_src_ip();
		std::string get_dst_ip();
		size_t get_frame_size();
		void set_src_mac(std::string);
		void set_dst_mac(std::string);
		void set_src_ip(std::string);
		void set_dst_ip(std::string);
		void set_frame_size(size_t);
		void erase();
	private:
		std::string source_mac;
		std::string destination_mac;
		std::string source_ip;
		std::string destination_ip;
		size_t frame_size;
};

#endif