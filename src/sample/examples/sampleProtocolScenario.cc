/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Alberto Gallegos
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alberto Gallegos <ramonet@fc.ritsumei.ac.jp>
 *         Ritsumeikan University, Shiga, Japan
 */


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/sample-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"


#include <iostream>
#include <fstream>
#include <vector>
#include <string>




NS_LOG_COMPONENT_DEFINE ("SampleProtocolMinimum");

using namespace ns3;


//uint16_t port = 625;
Ptr<ConstantVelocityMobilityModel> cvmm;




int
main (int argc, char *argv[])
{

   int numNodes = 120;
   double heightField = 200;
   double widthField = 400;


   int pktSize = 1024;  //packet Size in bytes


   RngSeedManager::SetSeed(15);
   RngSeedManager::SetRun (7);

   ////////////////////// CREATE NODES ////////////////////////////////////
   NodeContainer staticNodes;
   staticNodes.Create(numNodes);
   std::cout<<"Nodes created\n";

   ///////////////////// CREATE DEVICES ////////////////////////////////////

   UintegerValue ctsThr = (true ? UintegerValue (100) : UintegerValue (2200));
   Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThr);

   // A high number avoid drops in packet due to arp traffic.
   Config::SetDefault ("ns3::ArpCache::PendingQueueSize", UintegerValue (400));

   WifiHelper wifi;
   wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
   //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));


   //PHYSICAL LAYER configuration
   YansWifiPhyHelper wifiPhyHelper =  YansWifiPhyHelper::Default ();
   YansWifiChannelHelper wifiChannelHelper;
   wifiChannelHelper.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
   wifiChannelHelper.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(52));
   Ptr<YansWifiChannel> pchan = wifiChannelHelper.Create ();
   wifiPhyHelper.SetChannel (pchan);
   
   //MAC LAYER configuration
   WifiMacHelper wifiMacHelper;
   wifiMacHelper.SetType ("ns3::AdhocWifiMac");

 
   NetDeviceContainer nodeDevices;
   nodeDevices = wifi.Install (wifiPhyHelper, wifiMacHelper, staticNodes);



   std::cout<<"Devices installed\n";

   ////////////////////////   MOBILITY  ///////////////////////////////////////////

   /////// 1- Random in a rectangle Topology

   Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();
   x->SetAttribute ("Min", DoubleValue (0));
   x->SetAttribute ("Max", DoubleValue (widthField));


   Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable>();
   y->SetAttribute ("Min", DoubleValue (0));
   y->SetAttribute ("Max", DoubleValue (heightField));



   Ptr<RandomRectanglePositionAllocator> alloc = CreateObject<RandomRectanglePositionAllocator>();
   alloc->SetX (x);
   alloc->SetY (y);
   alloc->AssignStreams(3);


  ///////// 2- Grid Topology
  /*
   Ptr<GridPositionAllocator> alloc =CreateObject<GridPositionAllocator>();
   alloc->SetMinX((m_widthField/12)/2); //initial position X for the grid
   alloc->SetMinY(0); // Initial position Y for the grid
   alloc->SetDeltaX(m_widthField/12); //12 nodes per row (space between nodes in the row)
   alloc->SetDeltaY(m_heightField/10); //10 nodes per column (space between nodes in column)
   alloc->SetLayoutType( GridPositionAllocator::COLUMN_FIRST);
   alloc->SetN(10); //1 column  10 nodes
  */

  MobilityHelper mobilityFixedNodes;
  mobilityFixedNodes.SetPositionAllocator (alloc);
  mobilityFixedNodes.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityFixedNodes.Install (staticNodes);


  std::cout<<"mobility set \n";


  ////////////////////////   INTERNET STACK /////////////////////////

   SampleHelper sampleProtocol;

   Ipv4ListRoutingHelper listrouting;
   listrouting.Add(sampleProtocol, 10);

   InternetStackHelper internet;
   internet.SetRoutingHelper(listrouting);
   internet.Install (staticNodes);
  
   Ipv4AddressHelper ipv4;
   NS_LOG_INFO ("Assign IP Addresses.");
   ipv4.SetBase ("10.1.1.0", "255.255.255.0");
   Ipv4InterfaceContainer interfaces;
   interfaces = ipv4.Assign (nodeDevices);
   std::cout<<"Internet Stack installed\n";

   
	//////////////////// APPLICATIONS ////////////////////////////////
/*	uint16_t sinkPort = 9;
	PacketSinkHelper sink ("ns3::UdpSocketFactory",
	InetSocketAddress (Ipv4Address::GetAny (),sinkPort));
	ApplicationContainer sinkApps = sink.Install (mobileNode);
	sinkApps.Start (Seconds (158));
	sinkApps.Stop (Seconds (485));


	Ipv4Address sinkAddress = mobileNode->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
	uint16_t onoffPort = 9;
	OnOffHelper onoff1 ("ns3::UdpSocketFactory", InetSocketAddress (sinkAddress, onoffPort));
	onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
	onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
	onoff1.SetAttribute ("PacketSize", UintegerValue (pktSize));
	onoff1.SetAttribute ("DataRate", StringValue (std::to_string(appDataRate)));



	double deviation = 0;
	for(int i=0; i<=clientNodes.GetN()-1;  i++)
	  {
	     ApplicationContainer apps1;
		 apps1.Add(onoff1.Install (NodeList::GetNode (i)));
		 apps1.Start (Seconds (161)+Seconds(deviation));
		 apps1.Stop (Seconds (162)+Seconds(deviation));
		 deviation = deviation + 0.1;
	  }
    std::cout<<"applications installed\n";
/////////////////////////////////////////////////////////////////////////

*/


	Simulator::Stop(Seconds (10));


	Simulator::Run ();

    // clean variables
    staticNodes = NodeContainer();
    interfaces = Ipv4InterfaceContainer ();
    nodeDevices = NetDeviceContainer ();
  


    Simulator::Destroy ();
    std::cout<<"end of simulation\n";
}
