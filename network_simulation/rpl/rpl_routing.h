#ifndef RPL_ROUTING_H
#define RPL_ROUTING_H

#include<vector>
#include<map>
#include<set>
#include<string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "trickle_timer.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
using namespace ns3;

struct RplNodeState{
    int nodeId, rank, prefParent;
    double etx, residualEnergy; //etx is from node to root (path etx and not link etx)
    bool joined; //whether the node joined rpl network
    std::set<int>neighbours; //set to prevent duplicates (not used yet!!)
    std::map<int,int>neighbourRanks; //unused
    std::map<int,int>neighbourEtx; //unused
    uint32_t dioSent, dioReceived; //num of dio packets transmitted and received
    Time lastDioTime; //timestamp of last received dio
};

struct DioMessage{
    int sender, rank, version, dodagId; //sender's rank, version for when topology resets/changes
    double etx; //sender's
};

class DioHeader: public Header{
    public:
        DioHeader();
        void setSender(uint32_t sender);
        void setRank(uint32_t rank);
        uint32_t getSender() const; //const -> the function does not modify the object
        uint32_t getRank() const;
        //static -> belongs to class and not object; works without creating an object
        //even if called through an obj, doesn't actually use the object
        static TypeId GetTypeId(); //no const cuz it's static (has no obj to modify)
        virtual TypeId GetInstanceTypeId() const; //virtual -> overwrite the function of parent class (Header) ig
        virtual uint32_t GetSerializedSize() const;
        virtual void Serialize(Buffer::Iterator start) const; //iterator points to an address inside packet memory
        virtual uint32_t Deserialize(Buffer::Iterator start);
        virtual void Print(std::ostream &os) const;
    private:
        uint32_t m_sender, m_rank;
};

class DataHeader: public Header{
    public:
        DataHeader();
        void setSrc(uint32_t src);
        void setDest(uint32_t dest);
        void setSeq(uint32_t seq);
        void setHopCnt(uint32_t hopCnt);
        void setCreationTime(uint64_t t);
        uint32_t getSrc() const;
        uint32_t getDest() const;
        uint32_t getSeq() const;
        uint32_t getHopCnt() const;
        uint64_t getCreationTime() const;
        static TypeId GetTypeId();
        virtual TypeId GetInstanceTypeId() const;
        virtual uint32_t GetSerializedSize() const;
        virtual void Serialize(Buffer::Iterator start) const;
        virtual uint32_t Deserialize(Buffer::Iterator start);
        virtual void Print(std::ostream &os) const;
    private:
        uint32_t m_src, m_dest, m_seq, m_hopCnt;
        //seq->packet num
        uint64_t m_creationTime;
};

enum RplControlType{
    DIS = 0, //dodag info solicitation - when new node joins/node lost connectivity
    DIO = 1, //dodag info obj - has rank, routing info to build the tree
    DAO = 2, //destination advertisement obj - for downward routing
    DAO_ACK = 3
};

class RplRouting{
    public:
        RplRouting(NodeContainer &nodes, Ipv4InterfaceContainer interfaces); //receives nodes and stores them (constructor)
        void initialize(); //initial routing state for every node
        void startRoot(); //makes node 0 dodag root
        void sendDIO(int nodeId); //broadcast dio
        void receiveDIO(int receiver, DioMessage dio);
        void sendDAO(int nodeId);
        void processDIS(int nodeId);

        void genPacket(int nodeId);
        void printMetrics();

        void setupDioSockets();
        void receiveDioPacket(Ptr<Socket> socket);

        void setupDataSockets();
        void receiveDataPacket(Ptr<Socket> socket);

        bool isReachable(int a, int b);
        double getDistance(int a, int b);
        double computeEtx(int a, int b);

        void monitorSnifferRx(Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu, SignalNoiseDbm signalNoise, uint16_t staId);
    private:
        NodeContainer m_nodes;
        std::vector<RplNodeState> m_state;
        std::vector<MyTrickleTimer> m_trickle; //every node has it's own independent trickle timer
        double m_range; //communication range
        //packet metrics
        uint32_t m_totGenerated;
        uint32_t m_totDelivered;
        uint32_t m_totDropped;
        double m_totDelay;
        int m_nextSeq;
        std::vector<Ptr<Socket>> m_dioSockets;
        std::vector<Ptr<Socket>> m_dataSockets;
        Ipv4InterfaceContainer m_interfaces;

        double m_totRssi;
        uint32_t m_rssiSamples;

        std::map<std::pair<int,int>, uint32_t> m_dioSentCnt;
        std::map<std::pair<int,int>, uint32_t> m_dioRecvCnt;

        int m_cnt;
        int m_totHopCnt;
};

#endif