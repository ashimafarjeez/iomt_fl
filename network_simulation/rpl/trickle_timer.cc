#include "trickle_timer.h"
#include<cstdlib>

MyTrickleTimer::MyTrickleTimer(){} //constructor-initialization is in configure function

void MyTrickleTimer::configure(Time imin, Time imax, uint32_t k){
    //copy configuration to internal variables 
    m_imin = imin;
    m_imax = imax;
    m_k = k;
    m_current = imin;
}

void MyTrickleTimer::setCallBack(std::function<void()>cb){
    m_callback = cb;
}

void MyTrickleTimer::start(){
    //chk if some intervals are already running and if so, cancel them
    if(m_txEvent.IsPending()){
        m_txEvent.Cancel();
    }
    if(m_intervalEvent.IsPending()){
        m_intervalEvent.Cancel();
    }

    startInterval();
}

void MyTrickleTimer::reset(){
    //cancel all the old running startInterval calls
    if(m_txEvent.IsPending()){
        m_txEvent.Cancel();
    }
    if(m_intervalEvent.IsPending()){
        m_intervalEvent.Cancel();
    }

    m_current = m_imin;
    startInterval();
}

void MyTrickleTimer::consistent(){
    m_c++;
}

void MyTrickleTimer::inconsistent(){
    reset();
}

void MyTrickleTimer::startInterval(){
    m_c = 0;
    double half = m_current.GetSeconds()/2.0;
    double t = half + ((double)rand() / RAND_MAX) * half; //rand()/RAND_MAX produces a num btwn 0 and 1, *half scales it to 0 to I/2 and then +half to shift it to I/2 to I
    m_txEvent = Simulator::Schedule(Seconds(t), &MyTrickleTimer::transmit, this); //call transmit after t seconds, save it so that it can later be rescheduled, cancelled, etc
    m_intervalEvent = Simulator::Schedule(m_current, &MyTrickleTimer::startInterval, this); //start next interval after current interval is done
    if(m_current + m_current <= m_imax){
        m_current+=m_current;
    }
}

void MyTrickleTimer::transmit(){
    if(m_c < m_k){ //suppression rule check
        m_callback();
    }
}