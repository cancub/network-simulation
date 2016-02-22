#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unistd.h>
// #include <string>

using namespace std;

#define THRESHOLD 0.00000001
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
		std::vector<int> process_points; // the actual times of arrival
		double pmf(int);
		float factorial(int);
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
	/* 
	This function will, using the poisson distribution, determine the probability mass
	function for the specified lambda. Knowing the probability that k arrivals occur each
	second, we can determine the number of second intervals that should have this
	many arrivals. 
	*/

	std::vector<int> interval_descriptors;
	std::vector<double> interval_arrival_times;
	interval_descriptors.reserve(10);
	int max_intervals = 0;
	int total_intervals_with_k_frames, index, frames_in_interval;
	int i = 0;
	double p_at_k;

	for (int k = 0; k < 2*lambda; k++) {
		// make sure that we don't keep tesing after the probability
		// for k frames has become infinitesimal

		p_at_k = pmf(k);

		// find the integer number of second intervals that should have k arrivals
		total_intervals_with_k_frames = round(p_at_k * seconds);

		cout << "pmf(" << k << ") = " << p_at_k << endl;
		cout << "total intervals over " << seconds << " = " << total_intervals_with_k_frames << endl;
		cout << "max_intervals = " << max_intervals << ", total_intervals_with_k_frames = " << total_intervals_with_k_frames << endl;
		usleep(500000);

		// must find a time to stop testing. There may be 0 intervals with 0
		// arrivals, so if the test was just for 0 intervals, it may stop right away.
		// Instead, test the loop is on the right tail of the pmf and that 
		// the number of intervals with this many frames is 0, since anything
		// further to the right will have an even lower probability

		if (total_intervals_with_k_frames >= max_intervals)
			max_intervals = total_intervals_with_k_frames;
		else if (total_intervals_with_k_frames < max_intervals && total_intervals_with_k_frames == 0) 
			break;

		std::cout << "total intervals with " << k << " frames = " << total_intervals_with_k_frames << std::endl;
		
		// interval descriptors will now contain a group of descriptors which
		// represent how many times the output of the generator at an interval should be k frames 
		for (int i = 0; i < total_intervals_with_k_frames; i++) {
			interval_descriptors.push_back(k);
		}
	}

	for (int i = 0; i < interval_descriptors.size(); i++)
		cout << interval_descriptors.at(i) << endl;


	// size_t size = interval_descriptors.size();
	std::random_shuffle (interval_descriptors.begin(), interval_descriptors.end() );

	for (std::vector<int>::iterator it = interval_descriptors.begin(); it != interval_descriptors.end(); it++) {
		// select an item from interval descriptors at random
		// to see how many frames will arrive during this second interval.
		// make sure it was an item that was not selected before
		// index = rand() % size;
		// while (interval_descriptors[index] == -1) {
		// 	index = rand() % size;
		// }


		// frames_in_interval = interval_descriptors[index];
		// interval_descriptors.at(index) = -1;
		interval_arrival_times.resize(*it);

		for (int j = 0; j < *it; j++) {
			interval_arrival_times.at(j) = rand() % 1000;
		}

		std::sort(interval_arrival_times.begin(), interval_arrival_times.end());

		for (int j = 0; j < *it; j++) {
			double point_time = i*1000 + interval_arrival_times.at(j);		
			std::cout << point_time << std::endl;
			process_points.push_back(point_time);
		}

		i++;

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
	float top = pow(lambda,k)*pow(EULER,-1*lambda);
	// cout << "getting factorial, k = " << k << endl;
	float bottom = factorial(k);
	// cout << bottom << endl;
	// cout << top << "/" << bottom << endl;
	if (isinf(top) || isinf(bottom))
		return 0;
	else
		return top/bottom;
}

float FrameGenerator::factorial(int x) {
	float result;
	// usleep(1000000);
	// cout << "x = " << x << endl;

	if (x < 0) {
		std::cout << "Can't peform negative factorial" << std::endl;
	}
	else if (x == 0) {
		// cout << "0! = 1" << endl;
		return 1;
	}
	else if (x == 1) {
		// cout << "1! = 1" << endl;
		return 1;
	}
	else {
		result = x*factorial(x-1);

		// cout << x << " * " << x-1 << "!" << " = " << result << endl;
		return result;
	}
}

int main(int argc, char *argv[]) {
	// generate random number from a RV with poisson distribution
	// determine proper values for standard internet traffic
	// look up size distribution of frames
	// generate frame arrival time and size

	if (argc == 3){
		FrameGenerator my_frames(atoi(argv[1]),atoi(argv[2]));
		srand (time(NULL));
		my_frames.start();
	}
	else {
		std::cout << "Not enough arguments. Need (lambda, seconds)" << std::endl;
	}


	return 0;
}