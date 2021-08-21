/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 */
#define NS_LOG_APPEND_CONTEXT                                                \
  if (m_ipv4)                                                                \
    {                                                                        \
      std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; \
    }

#include "sample-routing-protocol.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include <algorithm>
#include <limits>
#include <math.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <random>

#include "ns3/mobility-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SampleRoutingProtocol");

namespace sample {
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for SAMPLE control traffic
const uint32_t RoutingProtocol::SAMPLE_PORT = 654;
int posx = 0; //送信者の位置情報
int posy = 0;
int maxLenge = 0; // 受信者との距離マックス

RoutingProtocol::RoutingProtocol ()
{
}

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::sample::RoutingProtocol")
          .SetParent<Ipv4RoutingProtocol> ()
          .SetGroupName ("Sample")
          .AddConstructor<RoutingProtocol> ()
          .AddAttribute ("UniformRv", "Access to the underlying UniformRandomVariable",
                         StringValue ("ns3::UniformRandomVariable"),
                         MakePointerAccessor (&RoutingProtocol::m_uniformRandomVariable),
                         MakePointerChecker<UniformRandomVariable> ());
  return tid;
}

RoutingProtocol::~RoutingProtocol ()
{
}

void
RoutingProtocol::DoDispose ()
{
}

void
RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
  *stream->GetStream () << "Node: " << m_ipv4->GetObject<Node> ()->GetId ()
                        << "; Time: " << Now ().As (unit)
                        << ", Local time: " << GetObject<Node> ()->GetLocalTime ().As (unit)
                        << ", SAMPLE Routing table" << std::endl;

  //Print routing table here.
  *stream->GetStream () << std::endl;
}

int64_t
RoutingProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uniformRandomVariable->SetStream (stream);
  return 1;
}

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif,
                              Socket::SocketErrno &sockerr)
{

  //std::cout << "Route Ouput Node: " << m_ipv4->GetObject<Node> ()->GetId () << "\n";
  Ptr<Ipv4Route> route;

  if (!p)
    {
      std::cout << "loopback occured! in routeoutput";
      return route; // LoopbackRoute (header,oif);
    }

  if (m_socketAddresses.empty ())
    {
      sockerr = Socket::ERROR_NOROUTETOHOST;
      NS_LOG_LOGIC ("No zeal interfaces");
      std::cout << "RouteOutput No zeal interfaces!!, packet drop\n";

      Ptr<Ipv4Route> route;
      return route;
    }

  return route;
}

bool
RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header,
                             Ptr<const NetDevice> idev, UnicastForwardCallback ucb,
                             MulticastForwardCallback mcb, LocalDeliverCallback lcb,
                             ErrorCallback ecb)
{

  std::cout << "Route Input Node: " << m_ipv4->GetObject<Node> ()->GetId () << "\n";
  return true;
}

void
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ipv4 == 0);
  m_ipv4 = ipv4;
}

void
RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (i, 0).GetLocal () << " interface is up");
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
    {
      return;
    }
  // Create a socket to listen only on this interface
  Ptr<Socket> socket;

  socket = Socket::CreateSocket (GetObject<Node> (), UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvSample, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetLocal (), SAMPLE_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketAddresses.insert (std::make_pair (socket, iface));

  // create also a subnet broadcast socket
  socket = Socket::CreateSocket (GetObject<Node> (), UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvSample, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetBroadcast (), SAMPLE_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketSubnetBroadcastAddresses.insert (std::make_pair (socket, iface));

  if (m_mainAddress == Ipv4Address ())
    {
      m_mainAddress = iface.GetLocal ();
    }

  NS_ASSERT (m_mainAddress != Ipv4Address ());

  /*  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
        {

          // Use primary address, if multiple
          Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
        //  std::cout<<"############### "<<addr<<" |ninterface "<<m_ipv4->GetNInterfaces ()<<"\n";
       

              TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
              Ptr<Node> theNode = GetObject<Node> ();
              Ptr<Socket> socket = Socket::CreateSocket (theNode,tid);
              InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), SAMPLE_PORT);
              if (socket->Bind (inetAddr))
                {
                  NS_FATAL_ERROR ("Failed to bind() ZEAL socket");
                }
              socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
              socket->SetAllowBroadcast (true);
              socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvSample, this));
              //socket->SetAttribute ("IpTtl",UintegerValue (1));
              socket->SetRecvPktInfo (true);

              m_socketAddresses[socket] = m_ipv4->GetAddress (i, 0);

              //  NS_LOG_DEBUG ("Socket Binding on ip " << m_mainAddress << " interface " << i);

              break;
           
        }
*/
}

void
RoutingProtocol::NotifyInterfaceDown (uint32_t i)
{
}

void
RoutingProtocol::NotifyAddAddress (uint32_t i, Ipv4InterfaceAddress address)
{
}

void
RoutingProtocol::NotifyRemoveAddress (uint32_t i, Ipv4InterfaceAddress address)
{
}

void
RoutingProtocol::DoInitialize (void)
{
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector pos = mobility->GetPosition ();


  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  std::cout<<"--topology--  id " << id << "x_pos " << pos.x << "y_pos" << pos.y << "\n";
  

  //std::cout << "broadcast will be send\n";
  //SendXBroadcast();
  for (int i = 0; i < 100; i++)
    {
      if (id == 0)
        {
          // Simulator::Schedule (Seconds (i), &RoutingProtocol::SendXBroadcast, this);
        }
    }
  for (int i = 1; i < 100; i++)
    {
      if (id == 0)
        {
          Simulator::Schedule (Seconds (i), &RoutingProtocol::SimulationResult,
                               this); //結果出力関数
          // std::cout << "time " << Simulator::Now ().GetMicroSeconds () << "\n";
        }
    }
}

void
RoutingProtocol::RecvSample (Ptr<Socket> socket)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  std::cout << "In recv Sample(Node " << m_ipv4->GetObject<Node> ()->GetId () << ")\n";
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector recvpos = mobility->GetPosition ();

  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  TypeHeader tHeader (SAMPLETYPE_HELLO);
  packet->RemoveHeader (tHeader);

  HelloHeader helloheader;
  packet->RemoveHeader (helloheader); //近隣ノードからのhello packet
  //int32_t recv_hello_id = helloheader.GetNodeId (); //NOde ID
  int32_t recv_hello_posx = helloheader.GetPosX (); //Node xposition
  int32_t recv_hello_posy = helloheader.GetPosY (); //Node yposition
  int send_road = distinctionRoad (recv_hello_posx, recv_hello_posy);
  int recv_road = distinctionRoad (recvpos.x, recvpos.y);

  double distance = std::sqrt ((recv_hello_posx - recvpos.x) * (recv_hello_posx - recvpos.x) +
                               (recv_hello_posy - recvpos.y) * (recv_hello_posy - recvpos.y));

  if (send_road != recv_road) //受信ノードと送信ノードの道路IDが違うならば
    {
      // std::cout << "異なる道路\n";
      recvCount[id]++;
      // std::cout << "In recv Sample(Node " << m_ipv4->GetObject<Node> ()->GetId () << ")"
      //           << "x=" << recvpos.x << "y=" << recvpos.y << "distance" << distance << "\n";
    }
  else
    {
    }

  if (distance > maxLenge)
    maxLenge = distance;

  if (distance > 150)
    std::cout << "送信者との距離 " << distance << "\n";
}

void
RoutingProtocol::SendXBroadcast (void)
{
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
       j != m_socketAddresses.end (); ++j)
    {
      int32_t id = m_ipv4->GetObject<Node> ()->GetId ();

      Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
      Vector mypos = mobility->GetPosition ();
      posx = mypos.x;
      posy = mypos.y;
      std::cout << "id" << id << "broadcast"
                << "x=" << posx << "y=" << posy << "time" << Simulator::Now ().GetSeconds ()
                << "\n";
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();

      // RrepHeader rrepHeader (0, 3, Ipv4Address ("10.1.1.15"), 5, Ipv4Address ("10.1.1.13"),
      //                        Seconds (3));
      // packet->AddHeader (rrepHeader);
      HelloHeader helloHeader (id, mypos.x, mypos.y);
      packet->AddHeader (helloHeader);

      TypeHeader tHeader (SAMPLETYPE_HELLO);
      packet->AddHeader (tHeader);

      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }
      socket->SendTo (packet, 0, InetSocketAddress (destination, SAMPLE_PORT));
      std::cout << "broadcast sent\n";
    }
}

int
RoutingProtocol::distinctionRoad (int x_point, int y_point)
{
    /// 道路1〜30  31〜61
  int gridRange = 200;
  int x = 0;
  int y = 0;
  int colomnCount = 1;
  int interRange = 10; //交差点の大きさ interRange × interRange の正方形

  for (int roadId = 1; roadId <= 60; roadId++)
    {   
        if (roadId <= 30)
          {
            if (x + interRange < x_point && x_point < x + gridRange - interRange && y - interRange < y_point && y_point < y + interRange) 
            {
              return roadId;
            }
            if(colomnCount == 5)
            {
              colomnCount = 1;
              x = 0;
              y = y + gridRange;
            }
            else {
              x = x+ gridRange;
              colomnCount++;
            }

            if(roadId == 30)
            {
              x = 0;
              y = 0;
            }
          }
        else //31〜
          {
            if (x - interRange < x_point && x_point < x + interRange && y +  interRange < y_point && y_point < y + gridRange -  interRange) 
            {
              return roadId;
            }
            if(colomnCount == 6)
            {
              colomnCount = 1;
              x = 0;
              y = y + gridRange;
            }
            else {
              x = x+ gridRange;
              colomnCount++;
            }
      }
    }
  return 0; // ０を返す = 交差点ノード
}

void
RoutingProtocol::SimulationResult (void) //
{
  if (Simulator::Now ().GetSeconds () == 16)
    {
      for (auto itr = recvCount.begin (); itr != recvCount.end (); itr++)
        {
          std::cout << "recv hello packet id = " << itr->first // キーを表示
                    << "取得数 " << itr->second << "\n"; // 値を表示
        }
      std::cout << "recv max lenge " << maxLenge << "\n";
      std::cout << "m_beta" << 9.0 << "\n";
      std::cout << "m_beta" << 30.2 << "\n";
    }
}

std::map<int, int> RoutingProtocol::recvCount;

} //namespace sample
} //namespace ns3
