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
		std::vector<float> cdf, pmf;
		std::vector<int> process_points; // the actual times of arrival
		void obtain_statistics();
		float get_pmf_value(int);
		int poisson_arrivals();
		void run_capture();
};


#endif