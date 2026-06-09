//deployment of nodes at random positions and creation of an iomt network

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"

#include "../rpl/trickle_timer.h"
#include "../rpl/rpl_routing.h"
#include "../rpl/trickle_timer.cc"
#include "../rpl/rpl_routing.cc"

// #include "../density_rpl/density_trickle_timer.h"
// #include "../density_rpl/density_rpl.h"
// #include "../density_rpl/density_trickle_timer.cc"
// #include "../density_rpl/density_rpl.cc"

// #include "../q-routing/q_routing.h"
// #include "../q-routing/q_routing.cc"

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

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    RplRouting rpl(nodes, interfaces);
    rpl.initialize();
    rpl.startRoot();

    //packet scheduling
    //for every node, generate a packet every 5 seconds starting from 20 seconds
    for(int node=1; node<200; node++){
        for(int t=20; t<=120; t+=5){
            Simulator::Schedule(Seconds(t), &RplRouting::genPacket, &rpl, node); //last 2 -> obj on which the func is to be called, parameter to be passed
        }
    }

    // QRouting q(nodes);
    // q.initialize();
    // for(int i=1; i<20; i++){ //node i generates a packet at time = i
    //     Simulator::Schedule(Seconds(i), &QRouting::genPacket, &q, i);
    // }

    Simulator::Stop(Seconds(250));
    Simulator::Run();
    rpl.printMetrics(); 
    Simulator::Destroy();

    return 0;
}