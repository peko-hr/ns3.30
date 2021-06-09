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

#include "shutoushu-routing-protocol.h"
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

NS_LOG_COMPONENT_DEFINE ("ShutoushuRoutingProtocol");

namespace shutoushu {
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for SHUTOUSHU control traffic
const uint32_t RoutingProtocol::SHUTOUSHU_PORT = 654;
int numVehicle = 0; //グローバル変数
int broudcastCount = 0;
int recvDistinationCount = 0;

RoutingProtocol::RoutingProtocol ()
{
}

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::shutoushu::RoutingProtocol")
          .SetParent<Ipv4RoutingProtocol> ()
          .SetGroupName ("Shutoushu")
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
                        << ", SHUTOUSHU Routing table" << std::endl;

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
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvShutoushu, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetLocal (), SHUTOUSHU_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_socketAddresses.insert (std::make_pair (socket, iface));

  // create also a subnet broadcast socket
  socket = Socket::CreateSocket (GetObject<Node> (), UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvShutoushu, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetBroadcast (), SHUTOUSHU_PORT));
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
              InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), SHUTOUSHU_PORT);
              if (socket->Bind (inetAddr))
                {
                  NS_FATAL_ERROR ("Failed to bind() ZEAL socket");
                }
              socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
              socket->SetAllowBroadcast (true);
              socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvShutoushu, this));
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
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  //int32_t time = Simulator::Now ().GetMicroSeconds ();
  m_trans[id] = 1;

  numVehicle++;
  if (id == sourceId)
    {
      // Simulator::Schedule (Seconds (1.0), &RoutingProtocol::SendShutoushuBroadcast, this);
      SendShutoushuBroadcast(id, 12 ,10000,10000, 0);
    }

  //**結果出力******************************************//
  // for (int i = 1; i < SimTime; i++)
  //   {
      if (id == 0)
        Simulator::Schedule (Seconds (5), &RoutingProtocol::SimulationResult,
                             this); //結果出力関数
    //}

}
// void
// RoutingProtocol::Send (int des_id)
// {
//   Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
//   Vector mypos = mobility->GetPosition ();
//   int MicroSeconds = Simulator::Now ().GetMicroSeconds ();
//   m_start_time[des_id] = MicroSeconds;
//   SendShutoushuBroadcast (0, des_id, m_my_posx[des_id], m_my_posy[des_id], 1);
//   std::cout << "\n\n\n\n\nsource node point x=" << mypos.x << "y=" << mypos.y
//             << "des node point x=" << m_my_posx[des_id] << "y=" << m_my_posy[des_id] << "\n";
// }

//**window size 以下のhello message の取得回数と　初めて取得した時間を保存する関数**//
// void
// RoutingProtocol::SetCountTimeMap (void)
// {
//   int32_t current_time = Simulator::Now ().GetMicroSeconds ();
//   int count = 0; //取得回数のカウント変数 この関数が呼び出されるたびに０に初期化
//   int check_id = -1; //MAPのIDが変化するタイミングを調べる変数
//   for (auto itr = m_recvtime.begin (); itr != m_recvtime.end (); itr++)
//     {
//       if (check_id == -1) //一番最初にこのループに入ったときの処理
//         {
//           int32_t dif_time = 0;
//           dif_time = current_time - itr->second; //現在の時刻とmapの取得時刻を比較
//           if (dif_time < WindowSize)
//             {
//               check_id = itr->first; //keyを代入
//               m_first_recv_time[itr->first] = itr->second; //ループの最初　＝　最初のhello取得時間
//               count = 1;
//             }
//         }
//       else
//         {
//           if (check_id != itr->first) //ループ中のkeyが変わった時
//             {
//               int32_t dif_time = 0;
//               dif_time = current_time - itr->second; //現在の時刻とmapの取得時刻を比較
//               //std::cout << "current time" << current_time << "\n";
//               //std::cout << "dif_time" << dif_time << "\n";
//               if (dif_time < WindowSize)
//                 {
//                   m_first_recv_time[itr->first] = itr->second;
//                 }
//               count = 1; //カウント変数の初期化
//               check_id = itr->first; //新しいkeyを代入
//               m_recvcount[check_id] = count; //カウント変数を代入
//             }
//           else
//             {
//               int32_t dif_time = 0;
//               dif_time = current_time - itr->second; //現在の時刻とmapの取得時刻を比較
//               //std::cout << "current time" << current_time << "\n";
//               //std::cout << "dif_time" << dif_time << "\n";
//               if (dif_time < WindowSize) //時間の差がウィンドウサイズ以内だったら
//                 {
//                   auto itr2 = m_first_recv_time.find (itr->first); // キーitr->firstを検索
//                   if (itr2 == m_first_recv_time.end ()) //使ったら削除する
//                     { // 見つからなかった場合
//                       m_first_recv_time[itr->first] = itr->second;
//                     }
//                   count++; //keyが変わらずループが回っている時カウント変数を加算していく
//                   m_recvcount[check_id] = count; //カウント変数を代入
//                 }
//             }
//         }

//       // int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
//       // std::cout << "id" << id << "    current time" << current_time;
//       // std::cout << " send node has recvmap  = " << itr->first // キーを表示
//       //           << " time  = " << itr->second << "\n"; // 値を表示
//     }
// }

// void
// RoutingProtocol::SetEtxMap (void) //////ETXをセットする関数
// {
//   int32_t current_time = Simulator::Now ().GetMicroSeconds ();
//   //current_time = current_time / 1000000; //secondになおす

//   for (auto itr = m_first_recv_time.begin (); itr != m_first_recv_time.end (); itr++)
//     {
//       //double micro_second = 1000000;
//       double diftime; //論文のt-t0と同意
//       double first_recv = (double) itr->second;
//       diftime = (current_time - first_recv) / 1000000;
//       //difftime = difftime / 1000000;
//       //std::cout << "id" << itr->first << "dif_time" << diftime << "\n";
//       double rt = (double) m_recvcount[itr->first] / diftime; //論文のrtの計算
//       //std::cout << "id" << itr->first << "のrtは" << rt << "rt*rt" << rt * rt << "\n";
//       if (rt > 1)
//         rt = 1.0;
//       m_rt[itr->first] = rt;
//       double etx = 1.000000 / (rt * rt);
//       if (etx < 1)
//         etx = 1;
//       m_etx[itr->first] = etx;

//       //std::cout << "id " << itr->first << " m_etx" << m_etx[itr->first] << "\n";
//       //std::cout << "\n";
//     }
// }

// void
// RoutingProtocol::SetPriValueMap (int32_t des_x, int32_t des_y)
// {
//   double Dsd; //ソースノードと目的地までの距離(sendSHUTOUSHUbroadcastするノード)
//   double Did; //候補ノードと目的地までの距離
//   int Distination_x = des_x;
//   int Distination_y = des_y;
//   Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
//   Vector mypos = mobility->GetPosition ();
//   int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
//   if (id == testId)
//     std::cout << "id" << id << "が持つ\n";

//   for (auto itr = m_etx.begin (); itr != m_etx.end (); itr++)
//     { ////next                  目的地までの距離とETX値から優先度を示す値をマップに保存する
//       Dsd = getDistance (mypos.x, mypos.y, Distination_x, Distination_y);
//       Did = getDistance (m_xpoint[itr->first], m_ypoint[itr->first], Distination_x, Distination_y);
//       int inter = 0; //初期値0 intersectionにいる場合は1に変わる
//       double dif = Dsd - Did;
//       if (dif < 0)
//         {
//           continue;
//         }
//       //std::cout << "id " << itr->first << " Dsd" << Dsd << " Did" << Did << "\n";

//       ///交差点にいるか　いないかの場合分け
//       // for (int x = 0; x < 2200;)
//       //   {
//       //     for (int y = 0; y < 2200;)
//       //       {
//       //         //std::cout << "x" << x << "y" << y << "\n";
//       //         int DisInter = getDistance (x, y, m_xpoint[itr->first],
//       //                                     m_ypoint[itr->first]); //交差点までの距離
//       //         if (DisInter < InterArea)
//       //           {
//       //             std::cout << "候補ノードid" << itr->first << "は交差点にいます\n";
//       //             inter = 1;
//       //           }
//       //         y = y + 300;
//       //       }
//       //     x = x + 300;
//       //   }

//       if (inter == 1)
//         {
//           m_pri_value[itr->first] = (Dsd - Did) / (m_etx[itr->first] * m_etx[itr->first]);
//           double angle = getAngle (
//               des_x, des_y, mypos.x, mypos.y, m_xpoint[itr->first],
//               m_ypoint[itr->first]); //角度Bを求める 中心となる座標b_x b_y = source node  = id = id
//           std::cout << "source id" << id << "候補ノードid" << itr->first
//                     << "とのdestinationまでの角度は" << angle << "\n";

//           m_pri_value[itr->first] = m_pri_value[itr->first] * InterPoint * angle;

//           //std::cout << "test angle" << getAngle (1.0, 1.0, 0.0, 0.0, 0.0, 1.0) << "\n";
//         }
//       else
//         {
//           m_pri_value[itr->first] = (Dsd - Did) / (m_etx[itr->first] * m_etx[itr->first]);
//         }

//       //std::cout << "id=" << itr->first << "のm_pri_value " << m_pri_value[itr->first] << "\n";
//     }
// }

// void
// RoutingProtocol::SendHelloPacket (void)
// {
//   for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
//        j != m_socketAddresses.end (); ++j)
//     {
//       int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
//       Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
//       Vector mypos = mobility->GetPosition ();

//       Ptr<Socket> socket = j->first;
//       Ipv4InterfaceAddress iface = j->second;
//       Ptr<Packet> packet = Create<Packet> ();

//       HelloHeader helloHeader (id, mypos.x, mypos.y);
//       packet->AddHeader (helloHeader);

//       TypeHeader tHeader (SHUTOUSHUTYPE_HELLO);
//       packet->AddHeader (tHeader);

//       Ipv4Address destination;
//       if (iface.GetMask () == Ipv4Mask::GetOnes ())
//         {
//           destination = Ipv4Address ("255.255.255.255");
//         }
//       else
//         {
//           destination = iface.GetBroadcast ();
//         }

//       Time Jitter = Time (MicroSeconds (m_uniformRandomVariable->GetInteger (0, 50000)));
//       //socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTOUSHU_PORT));
//       if (m_trans[id] == 1) //通信可能ノードのみ
//         {
//           Simulator::Schedule (Jitter, &RoutingProtocol::SendToHello, this, socket, packet,
//                                destination);
//         }
//     }
// }

// void
// RoutingProtocol::SendToHello (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination)
// {
//   int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
//   std::cout << " send id " << id << "  time  " << Simulator::Now ().GetMicroSeconds () << "\n";
//   socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTOUSHU_PORT));
// }

void
RoutingProtocol::SendToShutoushu (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination,
                                  int32_t hopcount, int32_t des_id)
{
      broudcastCount++;
      socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTOUSHU_PORT));
}

void
RoutingProtocol::SendShutoushuBroadcast (int32_t pri_value, int32_t des_id, int32_t des_x,
                                         int32_t des_y, int32_t hopcount) //pri_vaLUE 自分のid
{

  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
       j != m_socketAddresses.end (); ++j)
    {
      int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
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

      SendHeader sendHeader (des_id, des_x, des_y, hopcount, mypos.x, mypos.y, id,
                            0, 0);

      packet->AddHeader (sendHeader);

      TypeHeader tHeader (SHUTOUSHUTYPE_SEND);
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


        Simulator::Schedule (MicroSeconds (0), &RoutingProtocol::SendToShutoushu, this,
                                 socket, packet, destination, hopcount, des_id);
        
    }
} // namespace shutoushu

void
RoutingProtocol::RecvShutoushu (Ptr<Socket> socket)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition (); //受信したやつの位置情報
  double sourceDistance;
  double myDistance;


  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  TypeHeader tHeader (SHUTOUSHUTYPE_HELLO);
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
      case SHUTOUSHUTYPE_HELLO: { //hello message を受け取った場合

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
      case SHUTOUSHUTYPE_SEND: {

        SendHeader sendheader;
        packet->RemoveHeader (sendheader);

        int32_t des_id = sendheader.GetDesId ();
        int32_t des_x = sendheader.GetPosX ();
        int32_t des_y = sendheader.GetPosY ();
        int32_t hopcount = sendheader.GetHopcount ();
        int32_t packet_x = sendheader.GetId1 (); //送信元のx座標
        int32_t packet_y = sendheader.GetId2 (); //送信元のy座標
        //int32_t packet_id = sendheader.GetId3 (); // //id
        // int32_t pri4_id = sendheader.GetId4 ();
        // int32_t pri5_id = sendheader.GetId5 ();

        // SendHeader sendHeader (des_id, des_x, des_y, hopcount, mypos.x, mypos.y, id,
        //                     0, 0);


        if (des_id == id) //宛先が自分だったら
          {
            destinationcount[id] = 1;
            std::cout << "time" << Simulator::Now ().GetMicroSeconds () << "  id" << id
                      << "受信しましたよ　成功しました-------------\n";
            break;
          }
          else{
            sourceDistance = getDistance (packet_x,  packet_y,  des_x, des_y); //送信車両と宛先ノードとの距離
            myDistance = getDistance (mypos.x,  mypos.y,  des_x,  des_y); //自分と宛先ノードとの距離

            if(sourceDistance > myDistance)
            {
              // SendShutoushuBroadcast (int32_t pri_value, int32_t des_id, int32_t des_x,
              //                            int32_t des_y, int32_t hopcount) 
              std::cout<<"id"<<id<<"は自分のほうが近いので再ブロードキャストを行います\n";
              SendShutoushuBroadcast(id, des_id, des_x, des_y, hopcount);
            }
          }


        
        // int32_t pri_id[] = {sendheader.GetId1 (), sendheader.GetId2 (), sendheader.GetId3 (),
        //                     sendheader.GetId4 (), sendheader.GetId5 ()};

        ////*********recv hello packet log*****************////////////////
        // std::cout << " id " << id << "が受信したshutoushu packetは\n";
        // for (int i = 0; i < 5; i++)
        //   {
        //     if (pri_id[i] != 1000000)
        //       std::cout << "pri_id[" << i << "] =" << pri_id[i] << "すなわち優先度は" << i + 1
        //                 << "\n";
        //   }
        // std::cout << "from id" << hopcount << "\n";
        //           << "id:" << des_id << "xposition" << des_x << "yposition" << des_y
        //           << "priority 1 node id" << pri1_id << "priority 2 node id" << pri2_id
        //           << "priority 3 node id" << pri3_id << "priority 4 node id" << pri4_id
        //           << "priority 5 node id" << pri5_id << "\n";
        ////*********************************************////////////////

        

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

//ノード間の関係性を更新するメソッド
// void
// RoutingProtocol::SaveRelation (int32_t map_id, int32_t map_xpoint, int32_t map_ypoint)
// {
//   Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
//   Vector mypos = mobility->GetPosition ();
//   int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
//   int myRoadId = distinctionRoad (mypos.x, mypos.y); //自分の道路ID
//   int NeighborRoadId = distinctionRoad (map_xpoint, map_ypoint); // 近隣ノードの道路ID
//   int currentRelation = 0; //現在ノードとの関係性
//   if (myRoadId == 0 || NeighborRoadId == 0) // どちらかが交差点道路なら
//     {
//       currentRelation = 1; //同一道路なら1
//     }

//   if (myRoadId == NeighborRoadId)
//     {
//       currentRelation = 1; //同一道路なら1
//     }
//   else
//     {
//       currentRelation = 2; //異なる道路なら2
//     }

//   // if (id == testId)   test mobility debug
//   //   {
//   //     std::cout << "自分の道路IDは" << myRoadId << "\n";
//   //     std::cout << "id" << map_id << "の道路IDは" << NeighborRoadId << "\n";
//   //     std::cout << "id" << map_id << "とのcurrentRelation" << currentRelation << "\n";
//   //     std::cout << "id" << map_id << "recv数は" << m_recvtime.size () << "\n";
//   //     for (auto itr = m_recvtime.begin (); itr != m_recvtime.end (); itr++)
//   //       {
//   //         std::cout << "recv hello packet id = " << itr->first // キーを表示
//   //                   << "time" << itr->second << "\n"; // 値を表示
//   //       }
//   //   }

//   if (m_relation[map_id] != 0) //以前にリンクを持っていたら
//     {
//       if (currentRelation != m_relation[map_id]) //以前と関係性が異なるなら
//         {
//           if (NeighborRoadId == 0 || myRoadId == 0) // どちらかが交差点ノードの場合
//             {
//               currentRelation = m_relation[map_id]; //前の関係性を維持する
//             }
//           else
//             {
//               if (id == testId)
//                 {
//                   std::cout << "以前と関係性が違います　破棄する前の取得数 " << m_recvtime.size ()
//                             << "\n";
//                   std::cout << "関係性が変わった IDは" << id << "pare id" << map_id << "\n";
//                   // }
//                   m_recvtime.erase (map_id); // helloパケット取得履歴を破棄
//                   // if (id == testId)
//                   //   {
//                   std::cout << "破棄後の取得数 " << m_recvtime.size () << "\n";
//                 }
//             }
//         }
//     }
//   m_relation[map_id] = currentRelation; // 現在の関係をマップに保存
// }

// void
// RoutingProtocol::SaveRecvTime (int32_t map_id, int32_t map_recvtime)
// {
//   //int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
//   m_recvtime.insert (std::make_pair (map_id, map_recvtime));
// }

// void
// RoutingProtocol::SendXBroadcast (void)
// {
// }

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

///SUMO問題解決のためmobility.tclファイルを読み込み→ノードの発車時刻と到着時刻を知る
/*
void
RoutingProtocol::ReadFile (void)
{
  std::vector<std::string> v;
  std::ifstream ifs ("src/wave/examples/LSGO_Grid/mobility.tcl");
  if (!ifs)
    {
      std::cerr << "ファイルオープンに失敗" << std::endl;
      std::exit (1);
    }

  std::string tmp;
  std::string str;
  int time, node_id;
  int row_count = 1; //何列目かを判断するカウンター　atが１列目 time が２列目 $nodeが３列め

  // getline()で1行ずつ読み込む
  while (getline (ifs, tmp, ' '))
    {
      //std::cout << "row_cout=" << row_count << "\n";
      // ここでtmpを煮るなり焼くなりする
      //std::cout << tmp << "\n"; // そのまま出力
      if (tmp.find ("at") != std::string::npos)
        {
          //puts ("文字列atが見つかりました");
          row_count = 1; //at は１列目
        }
      if (row_count == 2)
        {
          time = atoi (tmp.c_str ());
          //std::cout << "time" << time << "\n";
        }
      if (row_count == 3)
        {
          tmp.replace (0, 1, "a"); //１番目の文字 " をaに変換
          //std::cout << "node id string test " << tmp << "\n";
          sscanf (tmp.c_str (), "a$node_(%d", &node_id); //文字列から数字だけをnode_idに代入
          //printf ("nodeid = %d\n", node_id);
          if (m_node_start_time[node_id] == 0)
            {
              if (time > 0 && time < 1000)
                {
                  m_node_start_time[node_id] = time;
                }
            }
          if (time != 0)
            {
              m_node_finish_time[node_id] = time; //常に更新させた最終更新時間が到着時間
            }
        }

      row_count++;
    }

  if (!ifs.eof ())
    {
      std::cerr << "読み込みに失敗" << std::endl;
      std::exit (1);
    }

  std::cout << std::flush;
}
*/
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

// int
// RoutingProtocol::distinctionRoad (int x_point, int y_point)
// {
//   /// 道路1〜56  57〜112
//   int range = 284;
//   int gridRange = 300;
//   int roadWidth = 20;
//   int x = 8;
//   int y = -10;
//   int count = 1;

//   for (int roadId = 1; roadId <= 112; roadId++)
//     {

//       if (roadId <= 56)
//         {
//           if (x <= x_point && x_point <= x + range && y <= y_point &&
//               y_point <= y + roadWidth) //example node1  x 座標 8〜292 y座標 -10〜10
//             {
//               return roadId; //条件に当てはまれば
//             }
//           if (count == 7) //7のときcount変数を初期化
//             {
//               count = 1;
//               x = x - 1800;
//               y = y + gridRange;
//             }
//           else // 7以外は足していく
//             {
//               x = x + gridRange;
//               count++;
//             }

//           if (roadId == 56)
//             {
//               x = -10;
//               y = 8;
//               count = 1;
//             }
//         }
//       else //57〜
//         {
//           if (x <= x_point && x_point <= x + roadWidth && y <= y_point &&
//               y_point <= y + range) //example node57 x座標 8〜292 y座標 -10〜10
//             {
//               return roadId; //条件に当てはまれば
//             }
//           if (count == 8) //8のときcount変数を初期化
//             {
//               count = 1;
//               x = x - 2100;
//               y = y + gridRange;
//             }
//           else // 8以外は足していく
//             {
//               x = x + gridRange;
//               count++;
//             }
//         }
//     }
//   return 0; // ０を返す = 交差点ノード
// }
// シミュレーション結果の出力関数
void
RoutingProtocol::SimulationResult (void) //
{
  int desCount = 0;

  if(Simulator::Now().GetSeconds () == 5)
  {
    for (auto itr = destinationcount.begin (); itr != destinationcount.end (); itr++)
            {
              desCount++;
            }
  std::cout << "受信数は　" << desCount << "\n";
  std::cout << "broadocast数は" << broudcastCount << "\n";
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

} // namespace shutoushu
} // namespace ns3
