#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unistd.h>

using namespace std;

#define THRESHOLD 0.000001
#define EULER 2.71828

class FrameGenerator {
	public:
		FrameGenerator();
		FrameGenerator(double,int);
		~FrameGenerator();
		void start();
	private:						
		double lambda; // average number of frames per second
		int seconds;
		std::vector<int> process_points;
		double pmf(int);
		int factorial(int);
};

FrameGenerator::FrameGenerator() {
	lambda = 0;
	seconds = 0;
	process_points.reserve(10);
}

FrameGenerator::FrameGenerator(double l, int s) {
	lambda = l;
	seconds = s;
	process_points.reserve(s*lambda -5);
}

FrameGenerator::~FrameGenerator() {}

void FrameGenerator::start() {
	std::vector<int> interval_descriptors;
	std::vector<double> interval_arrival_times;
	interval_descriptors.reserve(10);
	int max_intervals = 0;
	int total_intervals_with_k_frames, index, frames_in_interval;

	for (int k = 0; pmf(k) > THRESHOLD; k++) {

		// std::cout << "pmf(" << k << ") = " << pmf(k) << std::endl;

		total_intervals_with_k_frames = round(pmf(k) * seconds);


		if (total_intervals_with_k_frames >= max_intervals)
			max_intervals = total_intervals_with_k_frames;
		else if (total_intervals_with_k_frames < max_intervals && total_intervals_with_k_frames == 0) 
			break;

		std::cout << "total intervals with " << k << " frames = " << total_intervals_with_k_frames << std::endl;
		for (int i = 0; i < total_intervals_with_k_frames; i++) {
			interval_descriptors.push_back(k);
		}
	}

	for (int i = 0; i < interval_descriptors.size(); i++)
		cout << interval_descriptors.at(i) << endl;

	size_t size = interval_descriptors.size();

	for (int i = 0; i < size; i++) {
		index = rand() % size;
		while (interval_descriptors[index] == -1) {
			index = rand() % size;
		}

		frames_in_interval = interval_descriptors[index];
		interval_descriptors.at(index) = -1;
		interval_arrival_times.resize(frames_in_interval);

		for (int j = 0; j < interval_arrival_times.size(); j++) {
			interval_arrival_times.at(j) = rand() % 1000;
		}

		std::sort(interval_arrival_times.begin(), interval_arrival_times.end());

		for (int j = 0; j < interval_arrival_times.size(); j++) {
			double point_time = i*1000 + interval_arrival_times.at(j);		
			std::cout << point_time << std::endl;
			process_points.push_back(point_time);
		}

	}

	for (int i = 0; i < process_points.size(); i++) {
		if (i == 0) {
			usleep(process_points.at(i)*1000);
		}
		else {
			usleep((process_points.at(i)-process_points.at(i-1))*1000);
		}

		std::cout << "FRAME ARRIVAL" << std::endl;
	}


}

double FrameGenerator::pmf(int k) {
	return (double)(pow(lambda,k)*pow(EULER,-1*lambda)/factorial(k));
}

int FrameGenerator::factorial(int x) {
	if (x < 0)
		std::cout << "Can't peform negative factorial" << std::endl;
	else if ((x == 0) || (x == 1))
		return 1;
	else
		return x*factorial(x-1);
}

int main(void) {
	// generate random number from a RV with poisson distribution
	// determine proper values for standard internet traffic
	// look up size distribution of frames
	// generate frame arrival time and size

	FrameGenerator my_frames(4, 5);
	srand (time(NULL));
	my_frames.start();

	return 0;
}