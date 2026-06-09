#ifndef DENSITY_TRICKLE_TIMER_H
#define DENSITY_TRICKLE_TIMER_H
#include "ns3/core-module.h"
using namespace ns3;

class DensityTrickleTimer{
    public:
        DensityTrickleTimer();
        void configure(Time imin, Time imax, uint32_t k);
        void start();
        void reset();
        void consistent();
        void inconsistent();
        void setCallBack(Callback<void> cb);
        void setNeighbourCnt(uint32_t cnt);
    private:
        void delayedStart();
        void startInterval();
        void transmit();
        Time m_imin;
        Time m_imax;
        Time m_current;
        uint32_t m_k;
        uint32_t m_c;
        uint32_t m_neighbourCnt;
        EventId m_txEvent;
        EventId m_intervalEvent;
        Callback<void> m_callback;
};
#endif