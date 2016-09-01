#ifndef TRANSCEIVERS_H
#define TRANSCEIVERS_H

#include <thread>
#include "wqueue.h"
#include "frames.h"

class Transceivers{
    public:
        
        std::thread spawn_receiver();
        std::thread spawn_transmitter();
    private:

        wqueue<Frame*>* frame_queue;
}


#endif