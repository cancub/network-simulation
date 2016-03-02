#include <string>
#include <sstream>

// a patch for converting int to string
namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}

#include <stdlib.h>
#include <iostream>
#include <vector>
#include "addressing.h"
#include <time.h>

using namespace std;

std::string random_mac() {
	// as one might expect, this is a function
	// that takes no argument and generates
	// a random MAC address 
	
	int character_number;
	std::string result = ""; // we'll keep adding more characters to this result

	for(int i = 0; i < 12; i++) {
		// there are 12 difference characters that must be
		// obtained (not including separators)
		if (i != 0 && i %2 == 0)  {
			// after every 2nd character, add a colon
			result += ":";
		}

		// characters must be hex
		character_number = rand() % 16;

		// find the proper asci represenation for numbers and capital letters
		if (character_number < 10) {
			result += char(character_number+48);
		}
		else {
			result += char(character_number+55);
		}
	}

	return result;
}  

std::string random_ip() {
	// again, as one might expect, this takes no argument
	// and returns a random ip
	int character_number;
	std::string result = ""; // we'll keep adding more characters to this

	for(int i = 0; i < 4; i++) {
		// there are four numbers between 0 and 255 that are separated by periods
		// for IPv4
		if (i > 0)
			result += ".";
		// add the stringified version of this random number to the IP
		result += patch::to_string(rand() % 256);
	}

	return result;
}