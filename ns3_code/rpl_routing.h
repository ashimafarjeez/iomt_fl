#ifndef RPL_ROUTING_H
#define RPL_ROUTING_H

#include<vector>
#include<map>
#include<set>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "trickle_timer.h"
using namespace ns3;

struct RplNodeState{
    int nodeId, rank, prefParent;
    double etx, residualEnergy; //etx is from node to root (path etx and not link etx)
    bool joined; //whether the node joined rpl network
    std::set<int>neighbours; //set to prevent duplicates
    std::map<int,int>neighbourRanks;
    std::map<int,int>neighbourEtx;
    uint32_t dioSent, dioReceived; //num of dio packets transmitted and received
    Time lastDioTime; //timestamp of last received dio
};

struct DioMessage{
    int sender, rank, version, dodagId; //sender's rank, version for when topology resets/changes
    double etx; //sender's
};

enum RplControlType{
    DIS = 0, //dodag info solicitation - when new node joins/node lost connectivity
    DIO = 1, //dodag info obj - has rank, routing info to build the tree
    DAO = 2, //destination advertisement obj - for downward routing
    DAO_ACK = 3
};

class RplRouting{
    public:
        RplRouting(NodeContainer &nodes); //receives nodes and stores them (constructor)
        void initialize(); //initial routing state for every node
        void startRoot(); //makes node 0 dodag root
        void sendDIO(int nodeId); //broadcast dio
        void receiveDIO(int receiver, DioMessage dio);
        void sendDAO(int nodeId);
        void processDIS(int nodeId);
        bool isReachable(int a, int b);
        double getDistance(int a, int b);
        double computeEtx(int a, int b);
    private:
        NodeContainer m_nodes;
        std::vector<RplNodeState> m_state;
        std::vector<TrickleTimer> m_trickle; //every node has it's own independent trickle timer
        double m_range; //communication range
};

#endif