#ifndef IOMT_Q_ROUTING_H
#define IOMT_Q_ROUTING_H
#include<vector>
#include<map>
#include<set>
#include<climits>
#include<algorithm>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
using namespace ns3;

struct QNodeState{
    int nodeId;
    bool active; //chk if a node is usable
    std::set<int> neighbours;
    std::map<int,double> qValues; //neighbour, routing cost
    uint32_t packetsSent;
    uint32_t packetsReceived;
    double residualEnergy;
};

class QRouting{
    public:
        QRouting(NodeContainer &nodes);
        void initialize();
        void discoverNeighbours();
        void initializeQValues();
        bool isReachable(int a, int b);
        double getDist(int a, int b);
        double computeEtx(int a, int b);
        int selectNextHop(int nodeId);
        void genPacket(int src);
        void frwdPacket(int currNode);
        void updateQVal(int node, int neighbour, double reward, double futureCost);
        double getMinFutureCost(int nodeId);
    private:
        NodeContainer m_nodes;
        std::vector<QNodeState> m_state;
        double m_range;
        double m_alpha; //learning rate
        int m_sink; //destination
};

#endif