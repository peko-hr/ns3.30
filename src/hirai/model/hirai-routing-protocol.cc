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
 * 
 * 
 * 
 * 
 * チェック項目　ノード数　シミュレーション時間　ブロードキャスト開始時間　ファイル読み取りのファイル名
 *
 */
#define NS_LOG_APPEND_CONTEXT                                                \
  if (m_ipv4)                                                                \
    {                                                                        \
      std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; \
    }

#include "hirai-routing-protocol.h"
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
#include <sys/stat.h>

#include "ns3/mobility-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HiraiRoutingProtocol");

namespace hirai {
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for HIRAI control traffic
const uint32_t RoutingProtocol::HIRAI_PORT = 654;
int numVehicle = 0; //グローバル変数
int broadcastCount = 0;
int recvDistinationCount = 0;
int Grobal_StartTime = 20;
int Grobal_SourceNodeNum = 10;
int Grobal_Seed = 10007;

RoutingProtocol::RoutingProtocol ()
{
}

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::hirai::RoutingProtocol")
          .SetParent<Ipv4RoutingProtocol> ()
          .SetGroupName ("Hirai")
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
                        << ", HIRAI Routing table" << std::endl;

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

  //std::cout<<"Route Ouput Node: "<<m_ipv4->GetObject<Node> ()->GetId ()<<"\n";
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
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvHirai, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetLocal (), HIRAI_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketAddresses.insert (std::make_pair (socket, iface));

  // create also a subnet broadcast socket
  socket = Socket::CreateSocket (GetObject<Node> (), UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvHirai, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetBroadcast (), HIRAI_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketSubnetBroadcastAddresses.insert (std::make_pair (socket, iface));

  if (m_mainAddress == Ipv4Address ())
    {
      m_mainAddress = iface.GetLocal ();
    }

  NS_ASSERT (m_mainAddress != Ipv4Address ());
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
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  //int32_t time = Simulator::Now ().GetMicroSeconds ();
  m_trans[id] = 1;

  numVehicle++;
  if (id == 1)
    {
      Simulator::Schedule (Seconds (Grobal_StartTime - 2), &RoutingProtocol::SourceAndDestination, this);
      //std::cout << "a + " << "\n";
      //Simulator::Schedule (Seconds (20.0), &RoutingProtocol::SendHiraiBroadcast, this, id, 9, 100, 600, 0);
      //SendHiraiBroadcast(id, 9,100,600, 0);
      //sourcecount[id]=1;
      //source_time[id]=time;
    }


  //**結果出力******************************************//
  for (int i = 1; i < 301; i++)
    {
      if (id == 0){
        Simulator::Schedule (Seconds (i), &RoutingProtocol::SimulationResult,
                             this); //結果出力関数
                             }
      Simulator::Schedule (Seconds (i), &RoutingProtocol::SetMyPos,this);
    
    }

  for (int i = 0; i < Grobal_SourceNodeNum; i++)
  {
    //std::cout << "a + " << i << "\n";
    Simulator::Schedule (Seconds (Grobal_StartTime + i), &RoutingProtocol::PreparationForSend, this);
  }
  
}

void
RoutingProtocol::SourceAndDestination ()
{
  std::cout << "\nsource function\n";
  for (int i = 0; i < numVehicle; i++) ///node数　設定する
    {
      // if (m_my_posx[i] >= SourceLowX && m_my_posx[i] <= SourceHighX && m_my_posy[i] >= SourceLowY &&
      //     m_my_posy[i] <= SourceHighY)
      //   {
          source_list.push_back (i);
          des_list.push_back (i);
          //std::cout<<"source list id" << i << " position x"<<m_my_posx[i]<<" y"<<m_my_posy[i]<<"\n";
        // }
    }

  std::mt19937 get_rand_mt (Grobal_Seed);


  std::shuffle (des_list.begin (), des_list.end (), get_rand_mt);
  std::shuffle (source_list.begin (), source_list.end (), get_rand_mt);

  
  for (int i = 0; i < 10; i++)
    {
      std::cout << "shuffle source id" << source_list[i] << "\n";
    }
  std::cout << "\nshuffle destination id" << des_list[0] << "\n\n";


}// source list random 10kotoru

void
RoutingProtocol::PreparationForSend ()
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  int time = Simulator::Now ().GetSeconds ();

  if (time >= Grobal_StartTime)
    {
      //std::cout<<"time" << time - Grobal_StartTime << "\n";
      //std::cout<<"id" << id << "\n";
      //std::cout<<"source_list" << source_list[time - Grobal_StartTime] << "\n";
      if (id == source_list[time - Grobal_StartTime])
        {
          int index_time =
              time -
              Grobal_StartTime; //example time16 simstarttime15のときm_source_id = 1 すなわち２つめのsourceid

          Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
          Vector mypos = mobility->GetPosition ();
          //int MicroSeconds = Simulator::Now ().GetMicroSeconds ();
          //m_start_time[des_list[index_time]] = MicroSeconds + 300000; //秒数をずらし多分足す
          //std::cout << "m_start_time" << m_start_time[des_list[index_time]] << "\n";
          double shift_time = 0.3; //送信時間を0.3秒ずらす

          // SendLsgoBroadcast (0, des_list[index_time], m_my_posx[des_list[index_time]], m_my_posy[des_list[index_time]], 1);
          Simulator::Schedule (Seconds (shift_time), &RoutingProtocol::SendHiraiBroadcast, this, source_list[index_time],
                               des_list[0], m_my_posx[des_list[0]], m_my_posy[des_list[0]], 1);
          sourcecount[id] = 1;
          std::cout<< "\n\nid" << source_list[index_time] << " broadcast" << "\n";
          //std::cout<< "\n\n\nid" << source_list[1] << " broadcast" << "\n";
          //std::cout<< "\n\n\nid" << source_list[2] << " broadcast" << "\n";
          //std::cout<< "\n\n\nid" << source_list[3] << " broadcast" << "\n";
          std::cout << "source node point x=" << mypos.x << " y=" << mypos.y
                  //  << "des node point x=" << m_my_posx[des_list[index_time]]
                  //  << "y=" << m_my_posy[des_list[index_time]] 
                  << "\n";
        }
    }
}


void
RoutingProtocol::SendToHirai (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination,
                                  int32_t hopcount, int32_t des_id)
{
      broadcastCount++;
      socket->SendTo (packet, 0, InetSocketAddress (destination, HIRAI_PORT));
      //int time = Simulator::Now ().GetSeconds ();

      //int index_time = time - Grobal_StartTime; 
      //example time16 simstarttime15のときm_source_id = 1 すなわち２つめのsourceid
}

void
RoutingProtocol::SendHiraiBroadcast (int32_t pri_value, int32_t des_id, int32_t des_x,
                                         int32_t des_y, int32_t hopcount) //pri_value 自分のid
{

  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
       j != m_socketAddresses.end (); ++j)
    {
      //int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
      Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
      Vector mypos = mobility->GetPosition ();


      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();

      if (hopcount != 0) //source node じゃなかったら
        {
          hopcount++;
        }
      // SendHeader sendHeader (des_id, des_x, des_y, hopcount, pri_id[1], pri_id[2], pri_id[3],
      //                        pri_id[4], pri_id[5]);

      SendHeader sendHeader (des_id, des_x, des_y, hopcount, mypos.x, mypos.y, pri_value,
                            0, 0);

      packet->AddHeader (sendHeader);

      TypeHeader tHeader (HIRAITYPE_SEND);
      packet->AddHeader (tHeader);

     
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }

        Time Jitter = Time (MicroSeconds (m_uniformRandomVariable->GetInteger (0, 50000)));
        Simulator::Schedule (Jitter, &RoutingProtocol::SendToHirai, this,
                                 socket, packet, destination, hopcount, des_id);
        
    }
} // namespace hirai

void
RoutingProtocol::RecvHirai (Ptr<Socket> socket)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  int time = Simulator::Now ().GetMicroSeconds ();
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition (); //受信したやつの位置情報
  double sourceDistance;
  double myDistance;

  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  TypeHeader tHeader (HIRAITYPE_HELLO);
  packet->RemoveHeader (tHeader);

  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("Senko protocol message " << packet->GetUid ()
                                              << " with unknown type received: " << tHeader.Get ()
                                              << ". Drop");
      return; // drop
    }
  switch (tHeader.Get ())
    {
      case HIRAITYPE_HELLO: { //hello message を受け取った場合

        // if (m_trans[id] == 0)
        //   break;
        HelloHeader helloheader;
        packet->RemoveHeader (helloheader); //近隣ノードからのhello packet
        int32_t recv_hello_id = helloheader.GetNodeId (); //NOde ID
        int32_t recv_hello_posx = helloheader.GetPosX (); //Node xposition
        int32_t recv_hello_posy = helloheader.GetPosY (); //Node yposition
        //int32_t recv_hello_time = Simulator::Now ().GetMicroSeconds (); //

        // // ////*********recv hello packet log*****************////////////////
        std::cout << "Node ID " << id << "が受信したHello packetは"
                  << "id:" << recv_hello_id << "xposition" << recv_hello_posx << "yposition"
                  << recv_hello_posy << "\n";
        // // ////*********************************************////////////////
        SaveXpoint (recv_hello_id, recv_hello_posx);
        SaveYpoint (recv_hello_id, recv_hello_posy);
        //SaveRecvTime (recv_hello_id, recv_hello_time);
        // SaveRelation (recv_hello_id, recv_hello_posx, recv_hello_posy);

        break; //breakがないとエラー起きる
      }
      case HIRAITYPE_SEND: {

        SendHeader sendheader;
        packet->RemoveHeader (sendheader);

        int32_t des_id = sendheader.GetDesId ();
        int32_t des_x = sendheader.GetPosX ();
        int32_t des_y = sendheader.GetPosY ();
        int32_t hopcount = sendheader.GetHopcount ();
        int32_t packet_x = sendheader.GetId1 (); //送信元のx座標
        int32_t packet_y = sendheader.GetId2 (); //送信元のy座標
        int32_t source_id = sendheader.GetId3 (); 


        if (des_id == id) //宛先が自分だったら
          {
            if(destinationcount[source_id]!=1){//後で直す
            destinationcount[source_id]=1;
            std::cout << "time" << Simulator::Now ().GetMicroSeconds () << "  id" << id
                      << " 受信しました " << "source_id" << source_id << "　source_x"<<packet_x<<"　source_y"<<packet_y<<" 受信成功しました-------------\n\n";
            des_time[source_id] = time;//あとで直す
            }

          // p_recv_x.push_back (mypos.x);
          // p_recv_y.push_back (mypos.y);
          // p_recv_time.push_back (Simulator::Now ().GetMicroSeconds ());
          // p_hopcount.push_back (hopcount);
          // p_recv_id.push_back (id);
          // //p_source_id.push_back (send_id);
          // p_destination_id.push_back (des_id);
          // p_destination_x.push_back (des_x);
          // p_destination_y.push_back (des_y);
          // p_pri_1.push_back (pri_id[0]);
          // p_pri_2.push_back (pri_id[1]);
          // p_pri_3.push_back (pri_id[2]);
          break;
          }
          else{
            sourceDistance = getDistance (packet_x,  packet_y,  des_x, des_y); //送信車両と宛先ノードとの距離
            myDistance = getDistance (mypos.x,  mypos.y,  des_x,  des_y); //自分と宛先ノードとの距離

            if(sourceDistance > myDistance)
            {
              // SendHiraiBroadcast (int32_t pri_value, int32_t des_id, int32_t des_x,
              //                            int32_t des_y, int32_t hopcount) 
              //std::cout<<"id"<<id<<"は自分のほうが近いので再ブロードキャストを行います\n";
              //std::cout<<"destination id"<<des_id<<" des_x"<<des_x<<" des_y"<<des_y<<"\n";
              SendHiraiBroadcast(source_id, des_id, des_x, des_y, hopcount);
              std::cout << "sourceDistance " << sourceDistance << "\n" << "myDistance" << myDistance << "\n\n";
            }
          }
        break;
      }
    }
}

void
RoutingProtocol::SaveXpoint (int32_t map_id, int32_t map_xpoint)
{
  m_xpoint[map_id] = map_xpoint;
}

void
RoutingProtocol::SaveYpoint (int32_t map_id, int32_t map_ypoint)
{
  m_ypoint[map_id] = map_ypoint;
}

int
RoutingProtocol::getDistance (double x, double y, double x2, double y2)
{
  double distance = std::sqrt ((x2 - x) * (x2 - x) + (y2 - y) * (y2 - y));

  return (int) distance;
}

double
RoutingProtocol::getAngle (double a_x, double a_y, double b_x, double b_y, double c_x, double c_y)
{
  double BA_x = a_x - b_x; //ベクトルAのｘ座標
  double BA_y = a_y - b_y; //ベクトルAのy座標
  double BC_x = c_x - b_x; //ベクトルCのｘ座標
  double BC_y = c_y - b_y; //ベクトルCのy座標

  double BABC = BA_x * BC_x + BA_y * BC_y;
  double BA_2 = (BA_x * BA_x) + (BA_y * BA_y);
  double BC_2 = (BC_x * BC_x) + (BC_y * BC_y);

  //double radian = acos (cos);
  //double angle = radian * 180 / 3.14159265;

  double radian = acos (BABC / (std::sqrt (BA_2 * BC_2)));
  double angle = radian * 180 / 3.14159265; //ラジアンから角度に変換
  return angle;
}

void
RoutingProtocol::SetMyPos (void)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition ();
  m_my_posx[id] = mypos.x;
  m_my_posy[id] = mypos.y;
}


void
RoutingProtocol::Trans (int node_id)
{
  m_trans[node_id] = 1;
}

void
RoutingProtocol::NoTrans (int node_id)
{
  //m_trans[node_id] = 0;
  //std::cout << "time" << Simulator::Now ().GetSeconds () << "node id" << node_id
  //<< "が通信不可能になりました\n";
}


// シミュレーション結果の出力関数
void
RoutingProtocol::SimulationResult (void) //
{
  int desCount = 0;

  std::cout<<"time" << Simulator::Now().GetSeconds ()<< "\n";
  int totaldelay;

  if(Simulator::Now().GetSeconds () == 300)
  {
    for (auto itr = sourcecount.begin (); itr != sourcecount.end (); itr++)
            {
              desCount++;
              std::cout <<"source id keytest" << itr->first << "\n";
            }
     for (auto itr = des_time.begin (); itr != des_time.end (); itr++)
            {
              std::cout <<"destime keytest" << itr->first << "\n";
              totaldelay =  des_time[itr->first] - source_time[itr->first];
            }      
  
  std::cout<<"recv count" << destinationcount.size()  << "\n";
  std::cout<<"PDR" << double(destinationcount.size()) / double(sourcecount.size())  << "\n";
  std::cout<<"Delay" << double(totaldelay) / double (des_time.size()) << "\n";
  std::cout<< "broadcast count" << broadcastCount << "\n";
  std::cout<<"Overhead"<< double(broadcastCount) / double(destinationcount.size()) <<"\n";
  std::cout << "broadcast数は" << broadcastCount << "\n";

  std::string filename;
  //std::string send_filename;

  std::string grid_dir = "data/grid_side";
  std::cout<<"grid_side packet csv \n";
    filename = grid_dir + "/grid_side_" + std::to_string (Grobal_Seed) + "nodenum_" +
                       std::to_string (numVehicle) + ".csv";
    //send_filename = shadow_dir + "/send_lsgo/lsgo-seed_" + std::to_string (Grobal_Seed) + "nodenum_" +
    //                        std::to_string (numVehicle) + ".csv";
        
    const char *dir = grid_dir.c_str();
    struct stat statBuf;

    if (stat(dir, &statBuf) != 0) //directoryがなかったら
    {
      std::cout<<"ディレクトリが存在しないので作成します\n";
      mkdir(dir, S_IRWXU);
    }
    
      std::ofstream result (filename);
      result << "PDR"
             << ","
             << "delay"
             << ","
             << "Overhead" << std::endl;

      result << double(destinationcount.size()) / double(sourcecount.size())
             << ","
             << double(totaldelay) / double (des_time.size()) 
             << ","
             << double(broadcastCount) / double(destinationcount.size())  << std::endl;
      
      // for (int i = 0; i < packetCount; i++)
      //   {
      //     packetTrajectory << p_recv_x[i] << ", "<< p_recv_y[i] << ", " << p_recv_time[i]
      //                      << ", " << p_hopcount[i] << ", " << p_recv_id[i] << ", "
      //                      << p_destination_id[i] << ", "
      //                      << p_destination_x[i] << ", " << p_destination_y[i] << ", " << p_pri_1[i]
      //                      << ", " << p_pri_2[i] << ", " << p_pri_3[i] << std::endl;
      //   }
  }
}
  

std::map<int, int> RoutingProtocol::broadcount; //key 0 value broudcast数
std::map<int, int> RoutingProtocol::m_start_time; //key destination_id value　送信時間
std::map<int, int> RoutingProtocol::m_finish_time; //key destination_id value 受信時間
std::map<int, double> RoutingProtocol::m_my_posx; // key node id value position x
std::map<int, double> RoutingProtocol::m_my_posy; // key node id value position y
std::map<int, int> RoutingProtocol::m_trans; //key node id value　通信可能かどうか1or0
std::map<int, int> RoutingProtocol::m_stop_count; //key node id value 止まっている時間カウント
std::map<int, int> RoutingProtocol::m_node_start_time; //key node id value 止まっている時間カウント
std::map<int, int> RoutingProtocol::m_node_finish_time; //key node id value 止まっている時間カウント
std::map<int, int> RoutingProtocol::m_source_id;
std::map<int, int> RoutingProtocol::m_des_id;
std::map<int, int> RoutingProtocol::destinationcount;
std::map<int, int> RoutingProtocol::sourcecount;
std::map<int, int> RoutingProtocol::source_time;
std::map<int, int> RoutingProtocol::des_time;
std::vector<int> RoutingProtocol::source_list;
std::vector<int> RoutingProtocol::des_list;

} // namespace hirai
} // namespace ns3
