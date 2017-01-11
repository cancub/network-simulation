#include <stdlib.h>
#include <cstdint>
#include <vector>
#include <pdu.h>

void corrupt_frame(MPDU* clean_frame) {
	// take the SDU of this frame, select a random byte, and flip a bit in that byte
	byte_to_corrupt = rand() % clean_frame->SDU.size();


}
