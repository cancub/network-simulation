/*
   wqueue.h

   Worker thread queue based on the Standard C++ library list
   template class.

   ------------------------------------------

   Copyright @ 2013 [Vic Hargrave - http://vichargrave.com]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef __wqueue_h__
#define __wqueue_h__

#include <mutex>
#include <condition_variable> // std::condition_variable
#include <list>
#include "frames.h"

using namespace std;

#define MAX_QUEUE_LENGTH 20

template <typename T> class wqueue
{
    list<T>             m_queue;
    std::mutex*              m_mutex;
    condition_variable*      m_condv; 
    int max_entries;

    public:
        wqueue() {
            m_mutex = new std::mutex;
            m_condv = new condition_variable;
            max_entries = MAX_QUEUE_LENGTH;
        }
        ~wqueue() {
            delete m_mutex;
            // delete m_condv;
        }
        void add(T item) {
            std::unique_lock<std::mutex> lck(*m_mutex);

            if (m_queue.size() >= max_entries) {
                // simulate a dropped frame due to heavy traffic load
                lck.unlock();
                return;
            }
            m_queue.push_back(item);
            m_condv->notify_one();
        }
        T remove() {
            std::unique_lock<std::mutex> lck(*m_mutex);
            if (m_queue.size() == 0) {
                m_condv->wait(lck);
            }

            T item = m_queue.front();
            m_queue.pop_front();
            return item;
        }
        int size() {
            m_mutex->lock();
            int size = m_queue.size();
            m_mutex->unlock();
            return size;
        }
        void set_max_size(int max_value) {
            max_entries = max_value;
        }
};

#endif
