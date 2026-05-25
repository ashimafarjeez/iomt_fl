#include<climits>
#include<algorithm>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "trickle_timer.h"
#include "rpl_routing.h"
using namespace ns3;

RplRouting::RplRouting(NodeContainer &nodes):m_nodes(nodes){
    m_range = 100.0;
    m_state.resize(nodes.GetN()); //resize vectors acc to num of nodes
    m_trickle.resize(nodes.GetN());
}

void RplRouting::initialize(){
    for(uint32_t i=0; i<m_nodes.GetN(); i++){ //GetN() returns uint32_t
        m_state[i].nodeId = i;
        m_state[i].rank = INT_MAX;
        m_state[i].prefParent = -1;
        m_state[i].joined = false;
        m_state[i].etx = 0;
        m_trickle[i].configure(MilliSeconds(128), Seconds(16), 1);
        //bind sendDIO to the node's trickle timer so that when it fires, call sendDIO
        m_trickle[i].setCallBack(MakeBoundCallback(&RplRouting::sendDIO, this, i));
    }
}

void RplRouting::startRoot(){
    m_state[0].rank = 0;
    m_state[0].joined = true;
    m_trickle[0].start();
}

double RplRouting::getDistance(int a, int b){
    Ptr<MobilityModel> ma = m_nodes.Get(a)->GetObject<MobilityModel>();
    Ptr<MobilityModel> mb = m_nodes.Get(b)->GetObject<MobilityModel>();
    return CalculateDistance(ma->GetPosition(), mb->GetPosition());
}

bool RplRouting::isReachable(int a, int b){
    return getDistance(a,b)<=m_range;
}

double RplRouting::computeEtx(int a, int b){ //higher etx -> poor
    double d = getDistance(a,b);
    //packet reception ratio - packets received/sent - approximated using 1-(dist/range)
    double prr = std::max(0.1, 1.0-(d/m_range)); //prevent div by 0 by taking max
    return 1.0/prr;
}

/*dio - control plane info and not application data
control plane - handles routing, topology, network organizaiton
data plane - actual medical data (application packets)
*/
void RplRouting::sendDIO(int sender){
    DioMessage dio;
    dio.sender = sender;
    dio.rank = m_state[sender].rank;
    dio.etx = m_state[sender].etx;

    for(uint32_t i=0; i<m_nodes.GetN(); i++){
        if(i==sender) continue;
        if(isReachable(sender, i)){
            Simulator::Schedule(MilliSeconds(5), &RplRouting::receiveDIO, this, i, dio);
        }
    }
    m_state[sender].dioSent++;
}

void RplRouting::receiveDIO(int receiver, DioMessage dio){
    m_state[receiver].dioReceived++;
    double linkCost = computeEtx(receiver, dio.sender);
    int candidateRank = dio.rank + (int)(linkCost*10); //rank if the sender was chosen as parent
    if(candidateRank < m_state[receiver].rank){
        m_state[receiver].rank = candidateRank;
        m_state[receiver].etx = dio.etx + linkCost;
        m_state[receiver].prefParent = dio.sender;
        m_state[receiver].joined = true;
        m_trickle[receiver].inconsistent(); //change in topology -> change in network state
    }else{
        m_trickle[receiver].consistent();
    }
}

void RplRouting::sendDAO(int nodeId){ //no actual dao yet
    int parent = m_state[nodeId].prefParent;
    if(parent==-1) return;
    std::cout<<"DAO "<<nodeId<<" -> "<<parent<<std::endl;
}