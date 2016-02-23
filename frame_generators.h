#ifndef FRAME_GENERATORS_H
#define FRAME_GENERATORS_H

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unistd.h>

class Poisson {
	public:
		Poisson();
		Poisson(float,int);
		~Poisson();
		void set_lambda(double);
		void set_seconds(int);
		void start();
	private:						
		float lambda; // average number of frames per second
		int seconds;
		std::vector< std::vector<float> > cdf;
		std::vector<float> process_points; // the actual times of arrival
		void obtain_cdf();
		float poisson_arrival();
		void run_capture();
		void print_capture_pmf();
};


#endif