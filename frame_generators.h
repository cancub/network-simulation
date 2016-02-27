#ifndef FRAME_GENERATORS_H
#define FRAME_GENERATORS_H

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unistd.h>
// #include <chrono>

class Poisson {
	public:
		Poisson();
		Poisson(int);
		Poisson(double,int);
		~Poisson();
		void set_lambda(double);
		void set_seconds(int);
		void start();
		double interarrival_time();
	private:						
		double lambda; // average number of frames per second
		int seconds;
		std::vector< std::vector<double> > cdf;
		std::vector<double> process_points; // the actual times of arrival
		void build_cdf();
		void run_capture();
		void print_capture_pmf();
};


#endif