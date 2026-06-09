#include "density_trickle_timer.h"
#include<cstdlib>
#include<algorithm>
using namespace ns3;

DensityTrickleTimer::DensityTrickleTimer(){
    m_neighbourCnt = 0;
    m_c = 0;
}

void DensityTrickleTimer::configure(Time imin, Time imax, uint32_t k){
    m_imin = imin;
    m_imax = imax;
    m_k = k;
    m_current = imin;
}

void DensityTrickleTimer::setCallBack(Callback<void> cb){
    m_callback = cb;
}

void DensityTrickleTimer::setNeighbourCnt(uint32_t cnt){
    m_neighbourCnt = cnt;
}

void DensityTrickleTimer::start(){
    // if(m_txEvent.isRunning()){
    //     m_txEvent.Cancel();
    // }
    // if(m_intervalEVent.isRunning()){
    //     m_intervalEVent.Cancel();
    // }

    delayedStart();
}

void DensityTrickleTimer::reset(){
    // if(m_txEvent.isRunning()){
    //     m_txEvent.Cancel();
    // }
    // if(m_intervalEVent.isRunning()){
    //     m_intervalEVent.Cancel();
    // }

    m_current = m_imin;
    delayedStart();
}

void DensityTrickleTimer::consistent(){
    m_c++;
}

void DensityTrickleTimer::inconsistent(){
    reset();
}

void DensityTrickleTimer::delayedStart(){
    //node with >=20 neighbours -> density factor = 1
    double densityFactor = std::min(1.0, m_neighbourCnt/20.0);
    //delay chosen between 0 to I/2 depending on the density factor
    double startDelay = densityFactor * m_current.GetSeconds()/2.0;
    //wait for startDelay seconds and the startInterval
    Simulator::Schedule(Seconds(startDelay), &DensityTrickleTimer::startInterval, this);
}

void DensityTrickleTimer::startInterval(){
    //standard trickle transmission time selection
    m_c = 0;

    std::cout<<Simulator::Now().GetSeconds()<<"\t"<<m_neighbourCnt<<std::endl;

    double Iby2 = m_current.GetSeconds()/2;
    double t = Iby2 + ((double)rand()/RAND_MAX) * Iby2;
    m_txEvent = Simulator::Schedule(Seconds(t), &DensityTrickleTimer::transmit, this); //transmission
    m_intervalEvent = Simulator::Schedule(m_current, &DensityTrickleTimer::startInterval, this); //next interval
    if(m_current + m_current <= m_imax){
        m_current += m_current;
    }
}

void DensityTrickleTimer::transmit(){

    std::cout << "Transmission\t" << Simulator::Now().GetSeconds()<<std::endl;

    if(m_c < m_k && !m_callback.IsNull()){
        m_callback();
    }
}