//deployment of nodes at random positions and creation of an iomt network

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"

// #include "../rpl/trickle_timer.h"
// #include "../rpl/rpl_routing.h"
// #include "../rpl/trickle_timer.cc"
// #include "../rpl/rpl_routing.cc"

#include "../q-routing/q_routing.h"
#include "../q-routing/q_routing.cc"

using namespace ns3;

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

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

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

    QRouting q(nodes);
    q.initialize();
    for(int i=1; i<20; i++){ //node i generates a packet at time = i
        Simulator::Schedule(Seconds(i), &QRouting::genPacket, &q, i);
    }

    Simulator::Stop(Seconds(120));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}