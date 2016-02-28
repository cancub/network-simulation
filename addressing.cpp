#include <string>
#include <sstream>

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

	
	int character_number;
	std::string result = "";

	for(int i = 0; i < 12; i++) {
		if (i != 0 && i %2 == 0)  {
			result += ":";
		}

		character_number = rand() % 16;
		// cout << character_number << endl;
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
	int character_number;
	std::string result = "";

	for(int i = 0; i < 4; i++) {
		if (i > 0)
			result += ".";
		result += patch::to_string(rand() % 256);
	}

	return result;
}



// int main(void) {
// 	cout << random_mac() << endl;
// 	cout << random_ip() << endl;
// } 