#include "q_routing.h"
using namespace ns3;

QRouting::QRouting(NodeContainer &nodes):m_nodes(nodes){
    m_range = 100.0;
    m_alpha = 0.5;
    m_sink = 0;
    m_state.resize(nodes.GetN());
}

void QRouting::initialize(){
    for(uint32_t i=0; i<m_nodes.GetN(); i++){
        m_state[i].nodeId = i;
        m_state[i].active = true;
        m_state[i].packetsSent = 0;
        m_state[i].packetsReceived = 0;
        m_state[i].residualEnergy = 100.0;
    }
    discoverNeighbours();
    initializeQValues();
}

bool QRouting::isReachable(int a, int b){
    return getDist(a, b) <= m_range;
}

double QRouting::getDist(int a, int b){
    Ptr<MobilityModel> ma = m_nodes.Get(a)->GetObject<MobilityModel>();
    Ptr<MobilityModel> mb = m_nodes.Get(b)->GetObject<MobilityModel>();
    return CalculateDistance(ma->GetPosition(), mb->GetPosition());
}

double QRouting::computeEtx(int a, int b){
    double d = getDist(a, b);
    double prr = std::max(0.1, 1.0 - (d/m_range));
    return 1.0/prr;
}

void QRouting::discoverNeighbours(){
    for(uint32_t i=0; i<m_nodes.GetN(); i++){
        for(uint32_t j=0; j<m_nodes.GetN(); j++){
            if(i==j) continue;
            if(isReachable(i, j)){
                m_state[i].neighbours.insert(j);
            }
        }
    }
}

//initial routing based on etx
void QRouting::initializeQValues(){
    for(uint32_t i=0; i<m_nodes.GetN(); i++){
        for(int n: m_state[i].neighbours){
            double etx = computeEtx(i, n);
            m_state[i].qValues[n] = etx;
        }
    }
}

int QRouting::selectNextHop(int nodeId){
    double bestCost = INT_MAX;
    int bestNeighbour = -1;
    for(auto &e: m_state[nodeId].qValues){
        double q = e.second;
        if(q<bestCost){
            bestCost = q;
            bestNeighbour = e.first;
        }
    }
    return bestNeighbour;
}

void QRouting::genPacket(int src){
    std::cout<<src<<" generated packet\n";
    frwdPacket(src);
}

void QRouting::frwdPacket(int currNode){
    if(currNode == m_sink){
        std::cout<<"Packet reached sink\n";
        return;
    }
    int nxtHop = selectNextHop(currNode);
    if(nxtHop==-1){
        std::cout<<"No route available\n"; //drop packet
        return;
    }
    std::cout<<currNode<<" to "<<nxtHop<<std::endl;
    m_state[currNode].packetsSent++;
    m_state[nxtHop].packetsReceived++;
    double linkDelay = computeEtx(currNode, nxtHop);
    double futureCost = getMinFutureCost(nxtHop);
    updateQVal(currNode, nxtHop, linkDelay, futureCost);
    Simulator::Schedule(MilliSeconds(5), &QRouting::frwdPacket, this, nxtHop);
}

void QRouting::updateQVal(int node, int neighbour, double reward, double futureCost){
    double oldQ = m_state[node].qValues[neighbour];
    double newQ = oldQ + m_alpha*(reward+futureCost-oldQ);
    m_state[node].qValues[neighbour] = newQ;
}

double QRouting::getMinFutureCost(int nodeId){
    if(nodeId==m_sink) return 0;
    double best = INT_MAX;
    for(auto &e:m_state[nodeId].qValues){
        best = std::min(best, e.second);
    }
    return best;
}