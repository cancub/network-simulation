#ifndef MISC_H
#define MISC_H

#include <unistd.h>
#include "pdu.h"

using namespace std;

void corrupt_frame(MPDU* clean_frame);

void ns_rand_sleep() {usleep((rand() % 1000) + 100);}

#endif