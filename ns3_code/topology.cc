//deployment of nodes at random positions and creation of an iomt network

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "rpl_routing.h"
#include "../rpl/rpl_routing.cc"
#include "../rpl/rpl_routing.h"
#include "../rpl/trickle_timer.cc"
#include "../rpl/trickle_timer.h"
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

    RplRouting rpl(nodes);
    rpl.initialize();
    rpl.startRoot();
    Simulator::Stop(Seconds(120));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}