#ifndef MY_TRICKLE_TIMER_H //include guards - to include it only if not already included
#define MY_TRICKLE_TIMER_H

#include<functional>
#include "ns3/core-module.h"
using namespace ns3;

class MyTrickleTimer{
    public:
        MyTrickleTimer(); //constructor
        void configure(Time imin, Time imax, uint32_t k); //min, max interval, redundanncy constant k
        void start();
        void reset(); //when inconsistency deducted, interval back to min
        void consistent(); //when received info matches current topology, increase suppression
        void inconsistent(); //when topology changes, rapid dissemiantion
        void setCallBack(std::function<void()> cb); //function to execute in order to transmit (trickle only controls timing and does not know what to transmit)
    private:
        //m_ -> naming convention for member variable
        void startInterval();
        void transmit(); //handles actual packet transmission logic
        Time m_imin;
        Time m_imax; //interval size
        Time m_current; //current intvl length
        uint32_t m_k; //redundancy constant
        uint32_t m_c; //consistency counter
        EventId m_txEvent; //scheduled transmission event
        EventId m_intervalEvent; //scheduled start of next interval
        std::function<void()> m_callback;
};

#endif //end of the guard block