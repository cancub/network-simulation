#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unistd.h>
// #include <string>

using namespace std;

#define THRESHOLD 0.0001

class FrameGenerator {
	public:
		FrameGenerator();
		FrameGenerator(float,int);
		~FrameGenerator();
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
		// float factorial(int);
		// float scaled_power(double,int);
};

FrameGenerator::FrameGenerator() {
	lambda = 0;
	seconds = 0;
	process_points.reserve(10);
}

FrameGenerator::FrameGenerator(float l, int s) {
	lambda = l;
	seconds = s;
	process_points.reserve(s*lambda -5);
}

FrameGenerator::~FrameGenerator() {}

void FrameGenerator::set_seconds(int s) {seconds = s;}

void FrameGenerator::set_lambda(double l) {lambda = l;}

void FrameGenerator::start() {
	/* 
	This function will, using the poisson distribution, determine the probability mass
	function for the specified lambda. Knowing the probability that k arrivals occur each
	second, we can determine the number of second intervals that should have this
	many arrivals. 
	*/

	std::vector<int> interval_cdf;
	float min;
	int max = 0;
	int value, test_number;
	int frames_in_interval;
	
	// get the pmf and cdf for lambda
	obtain_statistics();

	interval_cdf.reserve(cdf.size());

	// get minimum value in the cdf
	min = cdf.at(0);

	/*
	use that minimum value to divide every value in the cdf.
	The point of this exercise is to essential make a transfer function
	that takes as its input a uniform random variable and outputs
	a poisson random variable. Dividing every element of the cdf by the
	smallest value gives a vector of positive values, most of which will likely round
	to different integer values. Note that because every number is being scaled
	by the same amount, the relationship between each value remains the same
	*/
	for (std::vector<float>::iterator it = cdf.begin(); it != cdf.end(); ++it) {
		value = round(*it/min);

		// we need to ignore 0 values at the beginning of the vector, 
		// this will be clearer in a bit
		if (value == 0)
			value = -1;

		// find the maximum value of the vector
		if (value > max)
			max = value;

		interval_cdf.push_back(value);
		// cout << interval_cdf.at(interval_cdf.size() - 1) << endl;
	}

	/*
	Here's where this setup comes together. There will be <seconds> 1 second
	intervals for which a number of frames following the poisson process pmf
	will arrive. To determine the number of frames that arrive each interval,
	a random number is generated from 0 to the maximum the cdf we modified
	above. With the index of interval_cdf still representing the number
	of frames to arrive in an interval, we search for the first value in the new
	cdf that is less than our (uniformly generated) random number. Because it is
	a cdf, it is clear that 

	arg max pdf[i] = arg max interval_cdf[i] - interval_cdf[i-1]

	so the most likely range that the random number will fall in is the range represented
	by i

	(does that make sense?)
	*/
	for (int i = 0; i < seconds; i++) {

		// find the test_number for the modified cdf
		test_number = rand() % max;

		// locate the first index at which the value of the augmented cdf is
		// larger than the random number
		for (int j = 0; j < interval_cdf.size(); j++) {
			if (test_number < interval_cdf.at(j)) {
				frames_in_interval = j;
				break;
			}
		}

		// for this second interval, generate that many frames with timing
		// between 0 s and 1 s. In the future, it would be better to have 
		// some recursion here to comply with the true poisson random process,
		// since each sub interval of a poisson random process is itself
		// a poisson random process
		for (int j = 0; j < frames_in_interval; j++) {
			value = 1000000 * i + rand() % 1000000;
			process_points.push_back(value);
		}
	}

	std::sort(process_points.begin(), process_points.end());

	for (int i = 0; i < process_points.size(); i++) {
		if (i == 0) 
			value = process_points.at(0);
		else
			value = process_points.at(i) - process_points.at(i-1);

		usleep(value);

		cout << "Frame #" << i + 1 << " sent at: " << process_points.at(i)/1000000.0 << " s" << endl;
	}
}

void FrameGenerator::obtain_statistics() {
	pmf.reserve(5);
	cdf.reserve(5);

	float p_at_k = -1;
	float max = 0;
	float min = 1;
	int k = 0;

	while (1) {
		// loop until we've found a value for pmf(k) that is to the right of the pmf peak
		// and essentially 0

		p_at_k = get_pmf_value(k++);

		if (p_at_k > max) {
			// this is how the code knows if it is testing the pmf to the right of the
			// peak
			max = p_at_k;
		}
		else if (p_at_k == 0 && p_at_k < max) {
			// leave the loop, as the code is now at a zero value for pmf(k) and 
			// it is to the right of the peak
			break;	
		}

		pmf.push_back(p_at_k);

		if (cdf.size() == 0)
			cdf.push_back(p_at_k);
		else
			cdf.push_back(p_at_k + cdf.at(cdf.size()-1));

		// cout << p.size() << endl;
		// cout << "cdf[" << cdf.size()-1 << "] = " << cdf.at(cdf.size()-1) << endl;
		// usleep(50000);
	}

	// for (int i = 0; i < pmf.size(); i++) {
	// 	cout << pmf.at(i) << "\t\t" << cdf.at(i) << endl;
	// }
}

float FrameGenerator::get_pmf_value(int k) {
	/* 
	Since the number of terms involving k in the denominator and numerator
	are the same, it is simple to find pdf(k) by turning it into a 
	series of multiplications
	*/

	float result = exp(-lambda); // the e^(-lambda) term in the numerator

	for (int i = 1; i <= k; i++){
		result *= lambda/i;
	}

	if (result < THRESHOLD)
		result = 0;

	return result;
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