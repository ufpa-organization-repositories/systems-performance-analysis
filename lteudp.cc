#include "ns3/nstime.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/random-variable-stream.h"
#include <ns3/building.h>
#include <ns3/mobility-building-info.h>

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/tcp-socket.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include <iostream>
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include <ns3/buildings-module.h>
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/log.h"
#include <sys/timeb.h>
#include <ns3/internet-trace-helper.h>
#include <ns3/spectrum-module.h>
#include <ns3/log.h>
#include <ns3/string.h>
#include <fstream>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LTEUdpEchoExample");

int 
main (int argc, char *argv[])
{
  for (uint32_t x = 1 ; x < 4; ++x){
  NS_LOG_INFO ("Create nodes.");
  uint16_t numberOfNodes = 10;
  std::string Scheduler = "VANET";
  double simTime = 25.0;
  uint16_t seed = x;
  CommandLine cmd;
  cmd.AddValue("seed", "Seed of simulation", seed);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.Parse(argc, argv);
  ns3::SeedManager::SetSeed(seed);
  // remote host node
  NodeContainer remoteNodeContainer;
  remoteNodeContainer.Create (1);



  // create lte helper
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  // create epc helper
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  
  // impl path loss model
  lteHelper->SetAttribute("PathlossModel", StringValue("ns3::LogDistancePropagationLossModel"));

  // Get P-GW from EPC Helper
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  
  InternetStackHelper internet;
  internet.Install (remoteNodeContainer);

  NS_LOG_INFO ("Create channels.");

  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (30000)); // jumbo frames here as well
  // p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));  

  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteNodeContainer.Get(0));  // [P-GW] ------- [internet] ------ [remote host node]
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  
  Ipv4Address remoteHostAddr;

  remoteHostAddr = internetIpIfaces.GetAddress(1); // remote host ip adr

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteNodeContainer.Get(0)->GetObject<Ipv4> ());

  // hardcoded UE addresses for now
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.255.255.0"), 1);

  // create enb

  NodeContainer enbs;
  enbs.Create (1);

  //Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  //enbPositionAlloc->Add (Vector (0.0, 0.0, 0.0));
  //enbPositionAlloc->Add (Vector (100, 0.0, 0.0));

  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  //enbMobility.SetPositionAllocator(enbPositionAlloc);
  enbMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
  "MinX", DoubleValue (0.0),
  "MinY", DoubleValue (0.0),
  "DeltaX", DoubleValue (10.0),
  "DeltaY", DoubleValue (10.0),
  "GridWidth", UintegerValue (5),
  "LayoutType", StringValue ("RowFirst"));

  enbMobility.Install (enbs);
  NetDeviceContainer enbLteDev = lteHelper->InstallEnbDevice (enbs);

  // ues client
  NodeContainer ues;
  ues.Create(numberOfNodes);

  MobilityHelper ueMobility;

  ueMobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                    "X", DoubleValue (0.0),
                                    "Y", DoubleValue (0.0),
                                    "rho", DoubleValue (350.0));
  //ueMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  ueMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                               "Mode", StringValue ("Time"),
                                "Time", StringValue ("1s"),
                                "Speed", StringValue ("ns3::UniformRandomVariable[Min=5.0|Max=10.0]"),
                                "Bounds", RectangleValue (Rectangle (-3000.0, 3000.0, -3000.0, 3000.0)));
  ueMobility.Install (ues);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ues);
  
  InternetStackHelper internet2;
  internet2.Install (ues);
  
  Ptr<Node> ue = ues.Get(0);          
  //Ptr<NetDevice> ueLteDevice = ueLteDevs.Get(0);
  Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  
  //Ipv4InterfaceContainer uE_Interface1;
  //uE_Interface1 = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs1));

  // set the default gateway for the UE
  //Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());          
  //ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress(), 1);

  for (uint32_t u = 0; u < numberOfNodes; ++u)
   {
   Ptr<Node> ueNode_sg = ues.Get (u);
   Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode_sg->GetObject<Ipv4> ());
   ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
   }

  // we can now attach the UE, which will also activate the default EPS bearer
  lteHelper->Attach (ueLteDevs);

  //////// network setting clear;

  // server setting

  uint16_t port = 9;  // well-known echo port number
  UdpEchoServerHelper server (port);
  ApplicationContainer apps = server.Install (remoteNodeContainer.Get (0));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (simTime));

  Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (0), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = packetSinkHelper.Install (ues);
  sinkApps.Start (Seconds (1.0));
  sinkApps.Stop (Seconds (simTime));

  //
  // Create a UdpEchoClient application to send UDP datagrams from node zero to
  // node one.
  //
  
  //uint32_t packetSize = 4096;
  uint32_t packetSize = 8.192;
  uint32_t maxPacketCount = 2000;
  Time interPacketInterval = Seconds (0.1);
  UdpEchoClientHelper client (remoteHostAddr, port);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));

  apps = client.Install (ues);
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (simTime));

  // client.SetFill (apps.Get(0), 0xff, 1024 * 10);
  
  // AsciiTraceHelper ascii;
  // p2ph.EnableAsciiAll (ascii.CreateFileStream ("lte-udp-echo.tr"));
  // p2ph.EnablePcapAll ("lte-udp-echo", false);

  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;
  flowmon = flowmonHelper.InstallAll ();



  NS_LOG_INFO ("Run Simulation.");

  Simulator::Stop(Seconds(simTime));
  AnimationInterface anim ("lte-test.xml");
  anim.SetMaxPktsPerTraceFile(999999999999);
  Simulator::Run ();
  flowmon->CheckForLostPackets ();



    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon -> GetFlowStats();
    std::stringstream ss;
    ss << numberOfNodes;
  std::string numnode = ss.str();


     std::string STATS = Scheduler + "-" + numnode + "-STATS.txt";
     std::ofstream logger (STATS.c_str());

   std::string PDlT = Scheduler+"-"+ numnode + "-DlTHROUGHPUT.txt";
   std::string PUlT = Scheduler+"-"+numnode+ "-UlTHROUGHPUT.txt";
   std::string PDlD = Scheduler+"-"+numnode+ "-DlDELAY.txt";
   std::string PUlD = Scheduler+"-"+numnode+ "-UlDELAY.txt";
   std::string PDlJ = Scheduler+"-"+numnode+ "-DlJITTER.txt";
   std::string PUlJ = Scheduler+"-"+numnode+ "-UlJITTER.txt";

   std::ofstream PthroughputDL (PDlT.c_str());
   std::ofstream PdelayDL (PDlD.c_str());
   std::ofstream PjitterDL (PDlJ.c_str());
   std::ofstream PthroughputUL (PUlT.c_str());
   std::ofstream PdelayUL (PUlD.c_str());
   std::ofstream PjitterUL (PUlJ.c_str());

   PjitterDL << "%  Jitter" << std::endl;
   PjitterUL << "%  Jitter" << std::endl;


   PdelayDL << "% Delay" << std::endl;
   PdelayUL << "% Delay" << std::endl;

   PthroughputDL << "%  Throughput" << std::endl;
   PthroughputUL << "%  Throughput" << std::endl;
   

    for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
   {
                Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);


                std::cout << "  Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
                std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
                std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
                std::cout << "  Throughput: " << i->second.rxBytes * 8.0 /simTime/ 1024 / 1024  << " Mbps\n";
                std::cout << "  Tx Packets = " << i->second.txPackets << "\n";
                std::cout << "  Rx Packets = " << i->second.rxPackets << "\n";

        if ((i->second.rxPackets)!=1 && (i->second.rxPackets)!=0){
      logger << "Delay = " << (i->second.delaySum / i->second.rxPackets)/1000000 << " ms" << std::endl;
      logger << "Jitter = " << (i->second.jitterSum / (i->second.rxPackets - 1))/1000000 << " ms" << std::endl;
    


      if (t.sourceAddress == remoteHostAddr) {    
      
      PthroughputDL << i->second.rxBytes * 8.0 / simTime / 1024 / 1024<< std::endl;
      PdelayDL << (i->second.delaySum / i->second.rxPackets)/1000000 << std::endl; 
      PjitterDL << (i->second.jitterSum / (i->second.rxPackets - 1))/1000000 << std::endl;
      }
      

      if (t.destinationAddress == remoteHostAddr) {
      PthroughputUL << i->second.rxBytes * 8.0 / simTime / 1024 / 1024<< std::endl;
      PdelayUL << (i->second.delaySum / i->second.rxPackets)/1000000 << std::endl; 
      PjitterUL << (i->second.jitterSum / (i->second.rxPackets - 1))/1000000 << std::endl;
                                    }

   
   
      }else {
      logger << "Delaysum = " << i->second.delaySum << std::endl;
      logger << "Jittersum = " << i->second.jitterSum << std::endl;

    }
  }        

            



PthroughputDL.close();
PdelayDL.close();
PjitterDL.close();
PthroughputUL.close();
PdelayUL.close();
PjitterUL.close();

logger.close();
  


//flowmon->SerializeToXmlFile ("arq.flowmon", false, false);



if (x==1) {
flowmon->SerializeToXmlFile ("lteudp1.flowmon", false, false);}
if (x==2){
flowmon->SerializeToXmlFile ("lteudp2.flowmon", false, false);}
if (x==3) {
flowmon->SerializeToXmlFile ("lteudp3.flowmon", false, false);}
/*
if (x==4) {
flowmon->SerializeToXmlFile ("lteudp4.flowmon", false, false);}
if (x==5 ){
flowmon->SerializeToXmlFile ("lteudp5.flowmon", false, false);}
if (x==6 ){
flowmon->SerializeToXmlFile ("lteudp6.flowmon", false, false);}
if (x==7 ){
flowmon->SerializeToXmlFile ("lteudp7.flowmon", false, false);}
if (x==8 ){
flowmon->SerializeToXmlFile ("lteudp8.flowmon", false, false);}
if (x==9 ){
flowmon->SerializeToXmlFile ("lteudp9.flowmon", false, false);}
if (x==10 ){
flowmon->SerializeToXmlFile ("lteudp10.flowmon", false, false);}
if (x==11 ){
flowmon->SerializeToXmlFile ("lteudp11.flowmon", false, false);}
if (x==12){
flowmon->SerializeToXmlFile ("lteudp12.flowmon", false, false);}
if (x==13 ){
flowmon->SerializeToXmlFile ("lteudp13.flowmon", false, false);}
if (x==14 ){
flowmon->SerializeToXmlFile ("lteudp14.flowmon", false, false);}
if (x==15) {
flowmon->SerializeToXmlFile ("lteudp15.flowmon", false, false);}
if (x==16) {
flowmon->SerializeToXmlFile ("lteudp16.flowmon", false, false);}
if (x==17) {
flowmon->SerializeToXmlFile ("lteudp17.flowmon", false, false);}
if (x==18) {
flowmon->SerializeToXmlFile ("lteudp18.flowmon", false, false);}
if (x==19) {
flowmon->SerializeToXmlFile ("lteudp19.flowmon", false, false);}
if (x==20) {
flowmon->SerializeToXmlFile ("lteudp20.flowmon", false, false);}
if (x==21){
flowmon->SerializeToXmlFile ("lteudp21.flowmon", false, false);}
if (x==22){
flowmon->SerializeToXmlFile ("lteudp22.flowmon", false, false);}
if (x==23) {
flowmon->SerializeToXmlFile ("lteudp23.flowmon", false, false);}
if (x==24) {
flowmon->SerializeToXmlFile ("lteudp24.flowmon", false, false);}
if (x==25) {
flowmon->SerializeToXmlFile ("lteudp25.flowmon", false, false);}
if (x==26) {
flowmon->SerializeToXmlFile ("lteudp26.flowmon", false, false);}
if (x==27) {
flowmon->SerializeToXmlFile ("lteudp27.flowmon", false, false);}
if (x==28) {
flowmon->SerializeToXmlFile ("lteudp28.flowmon", false, false);}
if (x==29) {
flowmon->SerializeToXmlFile ("lteudp29.flowmon", false, false);}
if (x==30) {
flowmon->SerializeToXmlFile ("lteudp30.flowmon", false, false);}
if (x==31) {
flowmon->SerializeToXmlFile ("lteudp31.flowmon", false, false);}
if (x==32) {
flowmon->SerializeToXmlFile ("lteudp32.flowmon", false, false);}
*/

 

  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
}
