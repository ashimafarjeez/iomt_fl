//deployment of nodes at random positions and creation of an iomt network

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

void sendDIO(int sender, NodeContainer &nodes, std::vector<int> &rank, std::vector<int> &parent, double range){
    Ptr<MobilityModel> senderMob = nodes.Get(sender)->GetObject<MobilityModel>();
    Vector senderPos = senderMob->GetPosition();

    for(int i=0; i<200; i++){
        if(i==sender) continue;

        Ptr<MobilityModel> receiverMob = nodes.Get(i)->GetObject<MobilityModel>();
        Vector receiverPos = receiverMob->GetPosition();

        double dist = CalculateDistance(senderPos, receiverPos);

        if(dist<=range){
            int nRank = rank[sender]+1;
            //check if better route is found
            if(rank[i]==-1 || nRank < rank[i]){
                rank[i]=nRank;
                parent[i]=sender;

                //Rebroadcast DIO
                sendDIO(i, nodes, rank, parent, range);
            }
        }
    }
}

int main(){
    NodeContainer nodes;
    nodes.Create(200);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper phy;

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();

    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices;
    devices = wifi.Install(phy, mac, nodes);

    MobilityHelper mobility;
    //assign random coordinates to nodes within a 500m*500m deployment area
    mobility.SetPositionAllocator(
        "ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=500.0]")
    );
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    std::vector<int> rank(200, -1);
    std::vector<int> parent(200, -1);
    rank[0] = 0;
    double range = 100.0;
    sendDIO(0, nodes, rank, parent, range);

    for(int i=0; i<200; i++){
        std::cout<<i<<"\t"<<parent[i] <<"\t"<<rank[i] <<std::endl;
    }

    return 0;
}