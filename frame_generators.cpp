#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unistd.h>
#include <time.h>
#include "frame_generators.h"
#include <math.h>

using namespace std;

#define THRESHOLD 0.00001

Poisson::Poisson() {
    lambda = 0;
    seconds = 0;
    process_points.reserve(10);
}

Poisson::Poisson(float l) {
    lambda = l;
    seconds = 0;
    process_points.reserve(10);
    // build_cdf();
}

Poisson::Poisson(float l, int s) {
    lambda = l;
    seconds = s;
    process_points.reserve(s*lambda);
    // build_cdf();
}

Poisson::~Poisson() {}

void Poisson::set_seconds(int s) {seconds = s;}

void Poisson::set_lambda(float l) {
    lambda = l;
    // build_cdf();
}

void Poisson::run() {
    /* 
    This function collects all the possion proccess points up
    until the total time for the points is greater than the
    seconds parameter
    */

    float interarrival_t;  // the interarrival time, t = tau_i - tau_i-1
    float total_t = 0.0;   // the total time for the process thus far, summing the interarrival times
    
    // get the cdf for the given lambda
    // build_cdf();    

    while (1) {
        // get the next interarrival time, based on the cdf we obtained
        interarrival_t = interarrival_time();
        // add that to the total time thus far
        total_t += interarrival_t;
        // check to see if this next point will fall outside the specified time range
        // and store it if it does not
        if (total_t < seconds) {
            process_points.push_back(interarrival_t);
        }
        else
            break;
    }

    // print to screen the timings for the frames
    run_capture();

    // print to screen the observed pmf (histogram) for the process
    print_capture_pmf();
}

// void Poisson::build_cdf() {

//     double increment = 0.0001;
//     double t = 0.0;

//     /*
//     CDF is a 2-D vector, which looks like this
//     cdf.at(i) = [time, cdf at time]
//     */

//     // this euqation describes the CDF for an interarrival process
//     double cdf_at_t = 1.0 - exp(-lambda*t);

//     // check to see if the CDF calues is within a predefined limit
//     // of being at it's maximum, 1. This is to prevent the code from
//     // continuing to compute values for the CDF that are exceptionally
//     // unlikely to be selected

    
//     editor's note: in the future, it would be better to have the final product
//     of this array stored somewhere so that it can just be pulled if the lambda
//     has been used before. If it's a new lambda that has never been used before,
//     store the cdf after it has been created.
    
//     while (1.0 - cdf_at_t > THRESHOLD) {
//         // each row of the CDF consists of a time and a CDF value at that time
//         std::vector<double> row;
//         row.push_back(t);
//         row.push_back(cdf_at_t);
//         cdf.push_back(row);

//         // find the next round's values
//         t += increment;
//         cdf_at_t = 1.0 - exp(-lambda*t);
//     }

//     // finally, ensure that the cdf does indeed go to 1
//     cdf.at(cdf.size()-1)[1] = 1.0;
// }

float Poisson::interarrival_time() {
    // find the test_number for the modified cdf
    // double result;
    
    return -1.0f * log(1.0f - ((rand() % 10000)/10000.0f))/lambda;
}

int Poisson::interarrival_time_us() {
    return (int)(interarrival_time()*1000000);
}

void Poisson::run_capture() {

    float sleep_time;
    float total_time = 0.0;
    int count = 1;

    // loop through all the values of the <seconds> second process
    for (std::vector<float>::iterator it = process_points.begin(); it != process_points.end(); ++it) {
        // find the time that should be printed on the screen
        total_time += *it;
        // find the number of frames that will have been sent after this one
        count++;
        cout << "Frame #" << count << " sent at: " << total_time << " s" << endl;
    }
    
}

void Poisson::print_capture_pmf() {
    int frames_in_interval = 0; // the frames seen in a given second interval
    int interval = 1;   // the second interval under observation
    double total_time = 0.0;    // the time from start at a given observation
    std::vector<float> capture_pmf(1); // the observed pmf (histogram) of this recent running of a process

    // loop through each of the arrival times in the point process
    for (std::vector<float>::iterator it = process_points.begin(); it != process_points.end(); ++it) {
        // add the interarrival time to the total time to know which
        // second interval we should be in
        total_time += *it;

        // if this most recent arrival occured in a new interval
        // we need to increment the element of the pmf corresponding
        // to how many frames we saw in the last interval
        while (total_time > float(interval)) {
            // there may be a need to resize the pmf if the 
            // value of the number of frames in the last interval
            // is greater than the current number of elements in the
            // pmf
            if (frames_in_interval > capture_pmf.size() - 1) {
                capture_pmf.resize(frames_in_interval + 1);
            }

            // increment the pmf at the element corresponding
            // to the number of frames in the previous interval
            capture_pmf.at(frames_in_interval) += 1.0;

            // reset the number of frames in the interval
            frames_in_interval = 0;

            // move to the next interval
            interval++;
        }

        // remember that we are still looking at the arrival time specified by
        // *it and we have yet to increase the number of frames seen in its interval
        // so we do so now
        frames_in_interval++;
    }


    // the above method cuts off the last interval, so we make the addition here
    if (frames_in_interval > capture_pmf.size() - 1) {
        capture_pmf.resize(frames_in_interval + 1);
    }
    capture_pmf.at(frames_in_interval) += 1.0;

    // print the normalized histogram (PMF) for this run
    for (int i = 0; i < capture_pmf.size(); i++) {
        cout << i << ": " << capture_pmf.at(i) << '\t' << capture_pmf.at(i) / float(seconds) << endl;
    }

}

// int main(void){

//     srand( time(NULL));
//     Poisson p(3,100);
//     p.run();
// }
