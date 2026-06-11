#include<climits>
#include<algorithm>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "density_trickle_timer.h"
#include "density_rpl.h"
using namespace ns3;

RplRouting::RplRouting(NodeContainer &nodes, Ipv4InterfaceContainer interfaces):m_nodes(nodes), m_interfaces(interfaces){
    m_range = 100.0;
    m_state.resize(nodes.GetN()); //resize vectors acc to num of nodes
    m_trickle.resize(nodes.GetN());
    m_totDelay=0.0;
    m_totDelivered=0;
    m_totDropped=0;
    m_totGenerated=0;
    m_nextSeq=0;
    m_totRssi = 0;
    m_rssiSamples = 0;
    m_cnt = 0;
    m_totHopCnt = 0;
}

void RplRouting::initialize(){
    setupDioSockets();
    setupDataSockets();
    for(uint32_t i=0; i<m_nodes.GetN(); i++){ //GetN() returns uint32_t
        m_state[i].nodeId = i;
        m_state[i].rank = INT_MAX;
        m_state[i].prefParent = -1;
        m_state[i].joined = false;
        m_state[i].etx = 0;
        m_state[i].dioSent = 0;
        m_state[i].dioReceived = 0;
        m_state[i].residualEnergy = 100.0;
        m_trickle[i].configure(MilliSeconds(128), Seconds(16), 1);
        int neighbourCnt = 0;
        for(uint32_t j=0; j<m_nodes.GetN(); j++){
            if(i==j) continue;
            if(isReachable(i,j)) neighbourCnt++;
        }
        m_trickle[i].setNeighbourCnt(neighbourCnt);
        //std::cout<<"Node "<<i<<" neighbours = "<<neighbourCnt<<std::endl;
        //bind sendDIO to the node's trickle timer so that when it fires, call sendDIO
        m_trickle[i].setCallBack(
            [this, i](){
                sendDIO(i);
            } //call sendDIO(i) on this object when callback fires
        );
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
    auto key = std::make_pair(a, b);
    uint32_t sent = m_dioSentCnt[key];
    uint32_t recv = m_dioRecvCnt[key];
    if(sent<10) return 10.0;
    double prr = (double)recv/sent;
    if(prr<0.01) return 100.0;
    return 1.0/prr;

    // double d = getDistance(a,b);
    // //packet reception ratio - packets received/sent - approximated using 1-(dist/range)
    // double prr = std::max(0.1, 1.0-(d/m_range)); //prevent div by 0 by taking max
    // return 1.0/prr;
}

/*dio - control plane info and not application data
control plane - handles routing, topology, network organizaiton
data plane - actual medical data (application packets)
*/
void RplRouting::sendDIO(int sender){
    //currently testing for one node
    Ptr<Packet> p = Create<Packet>(32);
    DioHeader hdr;
    m_state[sender].dioSent++;
    hdr.setSender(sender);
    hdr.setRank(m_state[sender].rank);
    p->AddHeader(hdr);
    for(uint32_t i=0; i<m_nodes.GetN(); i++){
        if((int)i==sender) continue;
        if(isReachable(sender, i)){
            m_dioSentCnt[{sender,i}]++;
            m_dioSockets[sender]->SendTo(p->Copy(), 0, InetSocketAddress(m_interfaces.GetAddress(i), 9999));
        }
    }
    //std::cout<<sender<< " sent \n";


    // DioMessage dio;
    // dio.sender = sender;
    // dio.rank = m_state[sender].rank;
    // dio.etx = m_state[sender].etx;

    // for(uint32_t i=0; i<m_nodes.GetN(); i++){
    //     if((int)i==sender) continue;
    //     if(isReachable(sender, i)){
    //         Simulator::Schedule(MilliSeconds(5), &RplRouting::receiveDIO, this, i, dio);
    //     }
    // }
    // m_state[sender].dioSent++;
}

//does routing layer work
void RplRouting::receiveDIO(int receiver, DioMessage dio){
    if(receiver==dio.sender) return;
    m_state[receiver].lastDioTime = Simulator::Now();
    m_state[receiver].dioReceived++;
    double linkCost = computeEtx(dio.sender, receiver);
    int candidateRank = dio.rank + (int) (linkCost*10); //rank if the sender was chosen as parent

    if(candidateRank < m_state[receiver].rank){
        bool firstJoin = !m_state[receiver].joined;
        m_state[receiver].rank = candidateRank;
        m_state[receiver].etx = dio.etx + linkCost;

        m_trickle[receiver].setEtx(m_state[receiver].etx);

        m_state[receiver].prefParent = dio.sender;
        m_state[receiver].joined = true;
        m_trickle[receiver].inconsistent(); //change in topology -> change in network state
        if(firstJoin){
            m_cnt++;
            //std::cout<<receiver<<" joined via "<<dio.sender<<"\trank: "<<candidateRank<<std::endl;
        }
    }else{
        m_trickle[receiver].consistent();
    }
}

void RplRouting::sendDAO(int nodeId){ //no actual dao yet
    int parent = m_state[nodeId].prefParent;
    if(parent==-1) return;
    //std::cout<<"DAO "<<nodeId<<" -> "<<parent<<std::endl;
}

//generate data packet
void RplRouting::genPacket(int nodeId){
    if(!m_state[nodeId].joined) return;
    m_totGenerated++;
    
    int parent = m_state[nodeId].prefParent;
    if(parent==-1){
        m_totDropped++;
        return;
    }
    //create packet, add header and send it
    Ptr<Packet> p = Create<Packet>(64);
    DataHeader hdr;
    hdr.setSrc(nodeId);
    hdr.setDest(0);
    hdr.setSeq(m_nextSeq++);
    hdr.setHopCnt(0);
    hdr.setCreationTime(Simulator::Now().GetMicroSeconds());
    p->AddHeader(hdr);
    //SendTo(packet, flags, dest_addr) from this socket
    //dest_addr -> interface addr on port 10000
    m_dataSockets[nodeId]->SendTo(p, 0, InetSocketAddress(m_interfaces.GetAddress(parent), 10000));
}

void RplRouting::printMetrics(){
    std::cout<<"Generated: "<<m_totGenerated<<std::endl;
    std::cout<<"Delivered: "<<m_totDelivered<<std::endl;
    std::cout<<"Dropped: "<<m_totGenerated-m_totDelivered<<std::endl;
    double pdr = 0;
    if(m_totGenerated){ //to prevent div by 0
       pdr = 100.0 * m_totDelivered/m_totGenerated;
    }
    double avgDelay = 0;
    if(m_totDelivered){
        avgDelay = m_totDelay/m_totDelivered;
    }
    double loss = 0;
    if(m_totGenerated){
        loss = 100.0 * (m_totGenerated-m_totDelivered)/m_totGenerated;
    }
    uint64_t totDio = 0;
    for(auto &s:m_state){
        totDio+=s.dioSent;
    }
        int cnt =  0;
    double totEtx = 0;
    for(auto &s:m_state){
        if(s.joined && s.nodeId!=0){
            totEtx += s.etx;
            cnt++;
        }
    }
    if(cnt){
        std::cout<<"Average ETX: "<<totEtx/cnt<<std::endl;
    }
    std::cout<<"PDR: "<<pdr<<std::endl;
    std::cout<<"Average delay: "<<avgDelay<<std::endl;
    std::cout<<"Packet loss rate: "<<loss<<std::endl;
    std::cout<<"RSSI: "<<m_totRssi/m_rssiSamples<<std::endl;
    std::cout<<"Total DIO: "<<totDio<<std::endl;
    std::cout<<"Nodes joined: "<<m_cnt<<std::endl;
    std::cout<<"Hop count: "<<m_totHopCnt<<std::endl;
}


void RplRouting::setupDioSockets(){
    m_dioSockets.resize(m_nodes.GetN());
    for(uint32_t i=0; i<m_nodes.GetN(); i++){
        Ptr<Socket> sock = Socket::CreateSocket(m_nodes.Get(i), UdpSocketFactory::GetTypeId());
        sock->Bind(InetSocketAddress(m_interfaces.GetAddress(i), 9999));
        sock->SetRecvCallback(MakeCallback(&RplRouting::receiveDioPacket, this));
        m_dioSockets[i] = sock;
    }
}

//does network layer work
void RplRouting::receiveDioPacket(Ptr<Socket>socket){
    Address from;
    Ptr<Packet> p = socket->RecvFrom(from);
    DioHeader hdr;
    p->RemoveHeader(hdr);
    int receiver = -1;
    for(uint32_t i=0; i<m_dioSockets.size(); i++){
        if(m_dioSockets[i]==socket){
            receiver = i;
            break;
        }
    }
    DioMessage dio;
    dio.sender = hdr.getSender();
    m_dioRecvCnt[{dio.sender, receiver}]++;
    dio.rank = hdr.getRank();
    dio.etx = 0;
    dio.version = 0;
    dio.dodagId = 0;
    receiveDIO(receiver, dio);
}

void RplRouting::setupDataSockets(){
    m_dataSockets.resize(m_nodes.GetN());
    for(uint32_t i=0; i<m_nodes.GetN(); i++){
        Ptr<Socket> sock = Socket::CreateSocket(m_nodes.Get(i), UdpSocketFactory::GetTypeId());
        sock->Bind(InetSocketAddress(m_interfaces.GetAddress(i), 10000)); //port 10000
        sock->SetRecvCallback(MakeCallback(&RplRouting::receiveDataPacket, this));
        m_dataSockets[i]=sock;
    }
}

void RplRouting::receiveDataPacket(Ptr<Socket>socket){
    Address from;
    Ptr<Packet> p = socket->RecvFrom(from);
    DataHeader hdr;
    p->RemoveHeader(hdr);
    int receiver=-1;
    for(uint32_t i=0; i<m_dataSockets.size(); i++){
        if(m_dataSockets[i]==socket){
            receiver=i;
            break;
        }
    }
    if(receiver==0){
        m_totDelivered++;
        m_totHopCnt+=hdr.getHopCnt();
        double delay = (Simulator::Now().GetMicroSeconds() - hdr.getCreationTime())/1000000.0;
        m_totDelay+=delay;
        //std::cout<<"Delivered packet "<<hdr.getSeq()<< " from "<<hdr.getSrc()<<" hops = "<<hdr.getHopCnt()<<std::endl;
        return;
    }
    //gotta send the received packet to parent
    int parent = m_state[receiver].prefParent;
    if(parent==-1){
        m_totDropped++;
        return;
    }
    hdr.setHopCnt(hdr.getHopCnt()+1);
    //reattach packet with hdr
    p->AddHeader(hdr);
    m_dataSockets[receiver]->SendTo(p, 0, InetSocketAddress(m_interfaces.GetAddress(parent), 10000));
}

void RplRouting::monitorSnifferRx(Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu, SignalNoiseDbm signalNoise, uint16_t staId){
    double rssi = signalNoise.signal;
    m_totRssi+=rssi;
    m_rssiSamples++;
}


//dioheader

DioHeader::DioHeader(){
    m_sender = 0;
    m_rank = 0;
}

void DioHeader::setSender(uint32_t sender){
    m_sender=sender;
}

void DioHeader::setRank(uint32_t rank){
    m_rank = rank;
}

uint32_t DioHeader::getSender() const{
    return m_sender;
}

uint32_t DioHeader::getRank() const{
    return m_rank;
}

TypeId DioHeader::GetTypeId(){
    static TypeId tid = TypeId("DioHeader").SetParent<Header>();
    return tid;
}

//useless func?
//used to jst overwrite the header function ig
TypeId DioHeader::GetInstanceTypeId() const{
    return GetTypeId();
}

uint32_t DioHeader::GetSerializedSize() const{
    return 8; //sender 4 bytes + rank 4 bytes
}

//to convert data into bytes and write them into packets
void DioHeader::Serialize(Buffer::Iterator start) const{
    //Hton -> host to network
    start.WriteHtonU32(m_sender); //write m_sender into packet bytes in the given iterator ig
    start.WriteHtonU32(m_rank);
}

//reconstruct data from packets
uint32_t DioHeader::Deserialize(Buffer::Iterator start){
    m_sender = start.ReadNtohU32();
    m_rank = start.ReadNtohU32();
    return 8;
}

void DioHeader::Print(std::ostream &os) const{
    //write into the given output stream os
    os<<"Sender = "<<m_sender<<" Rank = "<<m_rank;
}


//dataheader

DataHeader::DataHeader(){
    m_src = 0;
    m_dest = 0;
    m_seq=0;
    m_hopCnt=0;
    m_creationTime=0;
}

void DataHeader::setSrc(uint32_t src){
    m_src=src;
}

void DataHeader::setDest(uint32_t dest){
    m_dest=dest;
}

void DataHeader::setSeq(uint32_t seq){
    m_seq=seq;
}

void DataHeader::setHopCnt(uint32_t hopCnt){
    m_hopCnt=hopCnt;
}

void DataHeader::setCreationTime(uint64_t t){
    m_creationTime=t;
}

uint32_t DataHeader::getSrc() const{
    return m_src;
}

uint32_t DataHeader::getDest() const{
    return m_dest;
}

uint32_t DataHeader::getSeq() const{
    return m_seq;
}

uint32_t DataHeader::getHopCnt() const{
    return m_hopCnt;
}

uint64_t DataHeader::getCreationTime() const{
    return m_creationTime;
}

TypeId DataHeader::GetTypeId(){
    static TypeId tid = TypeId("DataHeader").SetParent<Header>();
    return tid;
}

//useless func?
//used to jst overwrite the header function ig
TypeId DataHeader::GetInstanceTypeId() const{
    return GetTypeId();
}

uint32_t DataHeader::GetSerializedSize() const{
    return 24;
}

//to convert data into bytes and write them into packets
void DataHeader::Serialize(Buffer::Iterator start) const{
    //Hton -> host to network
    start.WriteHtonU32(m_src); //write m_sender into packet bytes in the given iterator ig
    start.WriteHtonU32(m_dest);
    start.WriteHtonU32(m_seq);
    start.WriteHtonU32(m_hopCnt);
    start.WriteHtonU64(m_creationTime);
}

//reconstruct data from packets
uint32_t DataHeader::Deserialize(Buffer::Iterator start){
    m_src = start.ReadNtohU32();
    m_dest = start.ReadNtohU32();
    m_seq = start.ReadNtohU32();
    m_hopCnt = start.ReadNtohU32();
    m_creationTime = start.ReadNtohU64();
    return 24;
}

void DataHeader::Print(std::ostream &os) const{
    //write into the given output stream os
    os<<"Src = "<<m_src<<" Dest = "<<m_dest<<" Seq = "<<m_seq<<" HopCnt = "<<m_hopCnt<<" Creation time = "<<m_creationTime;
}


