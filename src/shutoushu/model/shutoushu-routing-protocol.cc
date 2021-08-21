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
#include <numeric>
#include <iterator>

#include "ns3/mobility-module.h"

int Buildings = 0;
int Grobal_Seed = 10000;
int Grobal_StartTime = 10; //Lsgo-simulationScenario似て変更する
int Grobal_SourceNodeNum = 10;
int Grobal_m_beta = 10;
int Grobal_m_gamma = 2;
double Grobal_InterPoint = 1.0;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ShutoushuRoutingProtocol");

namespace shutoushu {
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for SHUTOUSHU control traffic
const uint32_t RoutingProtocol::SHUTOUSHU_PORT = 654;

//グローバル変数
int numVehicle = 0; //車両数
int roadCenterPointX[113]; //道路の中心x座標を格納
int roadCenterPointY[113]; //道路の中心y座標を格納
int packetCount = 0; //パケット出力数のカウント
int sendpacketCount = 0; //sendpacket 出力数のカウント

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
  numVehicle++;

  for (int i = 1; i < SimTime; i++)
    {
      if (id != 0)
        Simulator::Schedule (Seconds (i), &RoutingProtocol::SendHelloPacket, this);
      Simulator::Schedule (Seconds (i), &RoutingProtocol::SetMyPos, this);
    }

  if (id == 0)
    {
      ///やることリスト
      ///送信者rのIDと位置情報をパケットに加える　車両数を ReadFile関数で読み取れるようにする

      RoadCenterPoint ();
      Simulator::Schedule (Seconds (Grobal_StartTime - 2), &RoutingProtocol::SourceAndDestination,
                           this);

      std::cout << "\n \n buildings" << Buildings << "\n";
      // double angle = 60;
      // double gammaAngle = angle / 90;
      // gammaAngle = pow (gammaAngle, 1 / AngleGamma);
      // gammaAngle = 90 * gammaAngle;
      // double Rp = 0.25;
      // double gammaRp = pow (Rp, 1 / RpGamma);
      // std::cout << "angle check" << gammaAngle << "Rp" << gammaRp << "\n";
    }

  //**結果出力******************************************//
  for (int i = 1; i < SimTime; i++)
    {
      if (id == 0)
        Simulator::Schedule (Seconds (i), &RoutingProtocol::SimulationResult,
                             this); //結果出力関数
    }

  for (int i = 0; i < Grobal_SourceNodeNum; i++)
    {
      Simulator::Schedule (Seconds (Grobal_StartTime + i * 1), &RoutingProtocol::Send, this);
    }
  /////////////////////////////random
}

void
RoutingProtocol::SourceAndDestination ()
{
  std::cout << "source and                        destination function\n";
  std::cout << "NumVehicle" << numVehicle << "\n";
  for (int i = 0; i < numVehicle; i++) ///node数　設定する
    {
      if (m_my_posx[i] >= SourceLowX && m_my_posx[i] <= SourceHighX && m_my_posy[i] >= SourceLowY &&
          m_my_posy[i] <= SourceHighY)
        {
          source_list.push_back (i);
          // std::cout<<"source list id" << i << "position x"<<m_my_posx[i]<<"y"<<m_my_posy[i]<<"\n";
        }
      if (m_my_posx[i] >= DesLowX && m_my_posx[i] <= DesHighX && m_my_posy[i] >= DesLowY &&
          m_my_posy[i] <= DesHighY)
        {
          des_list.push_back (i);
          // std::cout<<"destination list id" << i << "position x"<<m_my_posx[i]<<"y"<<m_my_posy[i]<<"\n";
        }
    }

  std::mt19937 get_rand_mt (Grobal_Seed);

  std::shuffle (source_list.begin (), source_list.end (), get_rand_mt);
  std::shuffle (des_list.begin (), des_list.end (), get_rand_mt);

  for (int i = 0; i < Grobal_SourceNodeNum; i++)
    {
      std::cout << "shuffle source id" << source_list[i] << "\n";
      std::cout << "shuffle destination id" << des_list[i] << "\n";
    }
}

void
RoutingProtocol::Send ()
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  int time = Simulator::Now ().GetSeconds ();

  if (time >= Grobal_StartTime)
    {
      if (id == source_list[time - Grobal_StartTime])
        {
          int index_time = time - Grobal_StartTime;

          Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
          Vector mypos = mobility->GetPosition ();
          int MicroSeconds = Simulator::Now ().GetMicroSeconds ();
          m_start_time[des_list[index_time]] = MicroSeconds + 300000; //秒数をずらし多分足す
          std::cout << "m_start_time" << m_start_time[des_list[index_time]] << "\n";
          double shift_time = 0.3; //送信時間を0.1秒ずらす

          //SendShutoushuBroadcast (0, des_list[index_time], m_my_posx[des_list[index_time]], m_my_posy[des_list[index_time]], 1);
          Simulator::Schedule (Seconds (shift_time), &RoutingProtocol::SendShutoushuBroadcast, this,
                               0, des_list[index_time], m_my_posx[des_list[index_time]],
                               m_my_posy[des_list[index_time]], 1);
          std::cout << "\n\n\n\n\nsource node point x=" << mypos.x << "y=" << mypos.y
                    << "des node point x=" << m_my_posx[des_list[index_time]]
                    << "y=" << m_my_posy[des_list[index_time]] << "\n";

          // Simulator::Schedule (MicroSeconds (wait_time), &RoutingProtocol::SendToShutoushu, this,
          //                          socket, packet, destination, hopcount, des_id);
        }
    }
}

//**window size 以下のhello message の取得回数と　初めて取得した時間を保存する関数**//
void
RoutingProtocol::SetCountTimeMap (void)
{
  int32_t current_time = Simulator::Now ().GetMicroSeconds ();
  int count = 0; //取得回数のカウント変数 この関数が呼び出されるたびに０に初期化
  int check_id = -1; //MAPのIDが変化するタイミングを調べる変数
  for (auto itr = m_recvtime.begin (); itr != m_recvtime.end (); itr++)
    {
      if (check_id == -1) //一番最初にこのループに入ったときの処理
        {
          int32_t dif_time = 0;
          dif_time = current_time - itr->second; //現在の時刻とmapの取得時刻を比較
          if (dif_time < WindowSize)
            {
              check_id = itr->first; //keyを代入
              m_first_recv_time[itr->first] = itr->second; //ループの最初　＝　最初のhello取得時間
              count = 1;
            }
        }
      else
        {
          if (check_id != itr->first) //ループ中のkeyが変わった時
            {
              int32_t dif_time = 0;
              dif_time = current_time - itr->second; //現在の時刻とmapの取得時刻を比較
              //std::cout << "current time" << current_time << "\n";
              //std::cout << "dif_time" << dif_time << "\n";
              if (dif_time < WindowSize)
                {
                  m_first_recv_time[itr->first] = itr->second;
                }
              count = 1; //カウント変数の初期化
              check_id = itr->first; //新しいkeyを代入
              m_recvcount[check_id] = count; //カウント変数を代入
            }
          else
            {
              int32_t dif_time = 0;
              dif_time = current_time - itr->second; //現在の時刻とmapの取得時刻を比較
              //std::cout << "current time" << current_time << "\n";
              //std::cout << "dif_time" << dif_time << "\n";
              if (dif_time < WindowSize) //時間の差がウィンドウサイズ以内だったら
                {
                  auto itr2 = m_first_recv_time.find (itr->first); // キーitr->firstを検索
                  if (itr2 == m_first_recv_time.end ()) //使ったら削除する
                    { // 見つからなかった場合
                      m_first_recv_time[itr->first] = itr->second;
                    }
                  count++; //keyが変わらずループが回っている時カウント変数を加算していく
                  m_recvcount[check_id] = count; //カウント変数を代入
                }
            }
        }

      // int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
      // std::cout << "id" << id << "    current time" << current_time;
      // std::cout << " send node has recvmap  = " << itr->first // キーを表示
      //           << " time  = " << itr->second << "\n"; // 値を表示
    }
}

void
RoutingProtocol::SetEtxMap (void) //////ETXをセットする関数
{
  int32_t current_time = Simulator::Now ().GetMicroSeconds ();
  //current_time = current_time / 1000000; //secondになおす

  for (auto itr = m_first_recv_time.begin (); itr != m_first_recv_time.end (); itr++)
    {
      //double micro_second = 1000000;
      double diftime; //論文のt-t0と同意
      double first_recv = (double) itr->second;
      diftime = (current_time - first_recv) / 1000000;
      //difftime = difftime / 1000000;
      //std::cout << "id" << itr->first << "dif_time" << diftime << "\n";
      double rt = (double) m_recvcount[itr->first] / diftime; //論文のrtの計算
      //std::cout << "id" << itr->first << "のrtは" << rt << "rt*rt" << rt * rt << "\n";
      if (rt > 1)
        rt = 1.0;
      m_rt[itr->first] = rt;
      double etx = 1.000000 / (rt * rt);
      if (etx < 1)
        etx = 1;
      m_etx[itr->first] = etx;

      //std::cout << "id " << itr->first << " m_etx" << m_etx[itr->first] << "\n";
      //std::cout << "\n";
    }
}

void
RoutingProtocol::SetPriValueMap (int32_t des_x, int32_t des_y)
{
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition ();

  double Rp;
  int nearRoadId;
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  //int myRoadId = distinctionRoad (mypos.x, mypos.y);

  nearRoadId = NearRoadId (des_x, des_y);
  std::cout << "id" << id << "が持つ(" << mypos.x << "," << mypos.y << ")"
            << "\n";
  std::cout << "最も近い道路IDは" << nearRoadId << "\n";
  //最も目的地に近い道路に存在するノードのどれか一つに届く確率Rpを返す関数
  Rp = CalculateRp (nearRoadId);
  std::cout << "Rp" << Rp << "\n";

  double Dsd; //ソースノードと目的地までの距離(sendSHUTOUSHUbroadcastするノード)
  double Did; //候補ノードと目的地までの距離
  int Distination_x = des_x;
  int Distination_y = des_y;

  for (auto itr = m_etx.begin (); itr != m_etx.end (); itr++)
    { ////next                  目的地までの距離とETX値から優先度を示す値をマップに保存する
      Dsd = getDistance (mypos.x, mypos.y, Distination_x, Distination_y);
      Did = getDistance (m_xpoint[itr->first], m_ypoint[itr->first], Distination_x, Distination_y);
      int inter = 0; //初期値0 intersectionにいる場合は1に変わる
      double dif = Dsd - Did;
      if (dif < 0)
        {
          continue;
        }
      //std::cout << "id " << itr->first << " Dsd" << Dsd << " Did" << Did << "\n";

      ///交差点にいるか　いないかの場合分け
      if (distinctionRoad (m_xpoint[itr->first], m_ypoint[itr->first]) == 0)
        { //roadid=0 すなわち交差点ノードならば
          std::cout << "候補ノードid" << itr->first << "は交差点にいます(" << m_xpoint[itr->first]
                    << "," << m_ypoint[itr->first] << ")"
                    << "\n";
          inter = 1;
        }

      if (inter == 1)
        {
          m_pri_value[itr->first] = (Dsd - Did) / (m_etx[itr->first] * m_etx[itr->first]);
          double angle = getAngle (
              des_x, des_y, mypos.x, mypos.y, m_xpoint[itr->first],
              m_ypoint[itr->first]); //角度Bを求める 中心となる座標b_x b_y = source node  = id = id
          std::cout << "source id" << id << "候補ノードid" << itr->first
                    << "とのdestinationまでの角度は" << angle << "destination position(" << des_x
                    << "," << des_y << ")"
                    << "\n";
          double gammaAngle = angle / 90;
          gammaAngle = pow (gammaAngle, 1 / AngleGamma);
          gammaAngle = 90 * gammaAngle;

          double gammaRp = pow (Rp, 1 / RpGamma);

          if (Rp != 0 && angle > 45 && Rp != 1)
            {
              m_pri_value[itr->first] = m_pri_value[itr->first] + InterPoint * gammaAngle / gammaRp;
              std::cout << "total 交差点 point" << InterPoint * gammaAngle / gammaRp;
            }
        }
      else
        {
          m_pri_value[itr->first] = (Dsd - Did) / (m_etx[itr->first] * m_etx[itr->first]);
        }
      std::cout << "id=" << itr->first << "のm_pri_value " << m_pri_value[itr->first] << "position("
                << m_xpoint[itr->first] << "," << m_ypoint[itr->first] << ")"
                << "\n";
    }
}

//目的地に最も近い道路IDを返す関数
int
RoutingProtocol::NearRoadId (int32_t des_x, int32_t des_y)
{
  int nearRoadId;
  double minDistance;
  int count = 0;
  for (auto itr = m_etx.begin (); itr != m_etx.end (); itr++)
    {
      //近隣ノードが位置する道路の中心座標を取得　
      int roadCenterX =
          roadCenterPointX[distinctionRoad (m_xpoint[itr->first], m_ypoint[itr->first])];
      int roadCenterY =
          roadCenterPointY[distinctionRoad (m_xpoint[itr->first], m_ypoint[itr->first])];
      if (distinctionRoad (roadCenterX, roadCenterY) == 0)
        continue;

      //std::cout << "id" << itr->first << "のroad idは" << distinctionRoad (roadCenterX, roadCenterY)
      //<< "roadCenterX" << roadCenterX << "roadCenterY" << roadCenterY << "\n";
      //目的地とそれぞれの道路の中心座標の距離を計算
      double distance = getDistance (roadCenterX, roadCenterY, des_x, des_y);
      //std::cout << "id" << itr->first << "distance" << distance << "\n";
      if (count == 0)
        {
          minDistance = distance;
          nearRoadId = distinctionRoad (roadCenterX, roadCenterY);
        }
      else
        {
          if (distance < minDistance)
            {
              minDistance = distance;
              nearRoadId = distinctionRoad (roadCenterX, roadCenterY);
            }
        }
      count++;
    }
  return nearRoadId;
}

//近い道路IDを受取その道路のRpを返す
double
RoutingProtocol::CalculateRp (int nearRoadId)
{
  double Rp = 0;
  double missProbability = 1; //道路に存在するすべてのノードとの伝送が失敗する確率
  int count = 0;

  for (auto itr = m_rt.begin (); itr != m_rt.end (); itr++)
    {
      if (distinctionRoad (m_xpoint[itr->first], m_ypoint[itr->first]) == nearRoadId)
        {
          if (count == 0)
            {
              missProbability = 1 - m_rt[itr->first];
              std::cout << "link を持っている node id" << itr->first << "とのrtは"
                        << m_rt[itr->first] << "\n";
            }
          else
            {
              std::cout << "link を持っている node id" << itr->first << "とのrtは"
                        << m_rt[itr->first] << "\n";
              missProbability = missProbability * (1 - m_rt[itr->first]);
            }
          count++;
        }
    }

  Rp = 1 - missProbability;
  return Rp;
}

void
RoutingProtocol::SendHelloPacket (void)
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

      HelloHeader helloHeader (id, mypos.x, mypos.y);
      packet->AddHeader (helloHeader);

      TypeHeader tHeader (SHUTOUSHUTYPE_HELLO);
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

      //socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTOUSHU_PORT));
      Simulator::Schedule (Jitter, &RoutingProtocol::SendToHello, this, socket, packet,
                           destination);
    }
}

void
RoutingProtocol::SendToHello (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination)
{
  // int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  // std::cout << " send id " << id << "  time  " << Simulator::Now ().GetMicroSeconds () << "\n";
  socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTOUSHU_PORT));
}

void
RoutingProtocol::SendToShutoushu (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination,
                                  int32_t hopcount, int32_t des_id)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition ();

  if (m_wait[des_id] == hopcount) //送信するhopcount がまだ待機中だったら
    {
      std::cout << "id " << id << " broadcast----------------------------------------------------"
                << "time" << Simulator::Now ().GetMicroSeconds () << "m_wait" << m_wait[des_id]
                << "position(" << mypos.x << "," << mypos.y << ")"
                << "\n";
      socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTOUSHU_PORT));
      m_wait.erase (des_id);
      if (m_finish_time[des_id] == 0) // まだ受信車両が受信してなかったら
        {
          s_send_log[m_send_check[des_id]] = 1;
          broadcount[des_id] = broadcount[des_id] + 1;
        }
    }
  else
    {
      // std::cout << "id " << id
      //           << " ブロードキャストキャンセル----------------------------------------------------"
      //           << "m_wait" << m_wait[id] << "time" << current_time << "\n";
      m_send_check.clear (); //broadcast canselされたのでsend checkしたindexをクリア
    }
}

void
RoutingProtocol::SendShutoushuBroadcast (int32_t pri_value, int32_t des_id, int32_t des_x,
                                         int32_t des_y, int32_t hopcount)
{

  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
       j != m_socketAddresses.end (); ++j)
    {
      if (hopcount > maxHop) // hop数が最大値を超えたらブレイク
        break;
      int send_node_id = m_ipv4->GetObject<Node> ()->GetId (); //broadcastするノードID
      Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
      Vector mypos = mobility->GetPosition (); //broadcastするノードの位置情報

      SetCountTimeMap (); //window sizeないの最初のhelloを受け取った時間と回数をマップに格納する関数
      for (auto itr = m_first_recv_time.begin (); itr != m_first_recv_time.end (); itr++)
        {
          //int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
          // std::cout << "id" << id;
          // std::cout << "が持つ近隣ノードID  = " << itr->first // キーを表示
          //           << "からの最初のHellomessage取得時間は  = " << itr->second << "\n"; // 値を表示
        }
      for (auto itr = m_recvcount.begin (); itr != m_recvcount.end (); itr++)
        {
          //int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
          // std::cout << "id" << id;
          // std::cout << "が持つ近隣ノードID  = " << itr->first // キーを表示
          //           << "からの最初のHellomessageの取得回数は  = " << itr->second
          //           << "\n"; // 値を表示
        }

      SetEtxMap (); //m_rt と EtX mapをセットする
      SetPriValueMap (des_x, des_y); //優先度を決める値をセットする関数

      int32_t pri_id[6];
      pri_id[1] = 10000000; ///ダミーID
      pri_id[2] = 10000000;
      pri_id[3] = 10000000;
      pri_id[4] = 10000000;
      pri_id[5] = 10000000;

      for (int i = 1; i < 6; i++) //5回回して、大きいものから取り出して削除する
        {
          int count = 1;
          int max_id = 10000000;
          double max_value = 0;
          for (auto itr = m_pri_value.begin (); itr != m_pri_value.end (); itr++)
            {

              if (count == 1)
                {
                  max_value = m_pri_value[itr->first];
                }
              else
                {
                  if (max_value < m_pri_value[itr->first])
                    {
                      max_value = m_pri_value[itr->first];
                    }
                }
              count++;
            }

          for (auto itr = m_pri_value.begin (); itr != m_pri_value.end (); itr++)
            {

              if (m_pri_value[itr->first] == max_value)
                {
                  max_id = itr->first;
                }
            }

          m_pri_value.erase (max_id);

          switch (i)
            {
            case 1:
              pri_id[1] = max_id;
              break;
            case 2:
              pri_id[2] = max_id;
              break;
            case 3:
              pri_id[3] = max_id;
              break;
            case 4:
              pri_id[4] = max_id;
              break;
            case 5:
              pri_id[5] = max_id;
              break;
            }
        }

      // 候補ノード選択アルゴリズム
      int candidataNum = 5; // 候補ノード数　初期値は最大

      for (int n = 1; n < 6; n++) //5回回して、 条件を満たすまでまわる nは候補ノード数
        {
          double infiniteProduct = 1; // 総乗
          for (int p = 1; p <= n; p++) //pは優先度  優先度は1から回す
            {
              double rtMiss = 1 - m_rt[pri_id[p]]; // 伝送に失敗する予想確率
              infiniteProduct = infiniteProduct * rtMiss;
            }
          if (TransProbability <= (1 - infiniteProduct)) //選択アルゴリズムの条件を満たすならば
            {
              std::cout << "infineteProduct" << infiniteProduct << "n" << n << "\n";
              candidataNum = n; //候補ノード数を変更
              break;
            }
        }

      //std::cout << "候補ノード数は" << candidataNum << "\n";
      switch (candidataNum) //候補ノード数によってダミーノードIDを加える
        {
        case 1:
          pri_id[2] = 10000000;
          pri_id[3] = 10000000;
          pri_id[4] = 10000000;
          pri_id[5] = 10000000;
          break;
        case 2:
          pri_id[3] = 10000000;
          pri_id[4] = 10000000;
          pri_id[5] = 10000000;
          break;
        case 3:
          pri_id[4] = 10000000;
          pri_id[5] = 10000000;
          break;
        case 4:
          pri_id[5] = 10000000;
          break;
        }

      for (int i = 1; i < 6; i++)
        {
          if (pri_id[i] != 10000000)
            {
              std::cout << "優先度" << i << "の node id = " << pri_id[i] << "予想伝送確率"
                        << m_rt[pri_id[i]] << "\n";
            }
        }
      if (pri_value != 0) //source node じゃなかったら
        {
          hopcount++;
        }

      s_source_id.push_back (send_node_id);
      std::cout << "s_source_id push back  send_node_id = " << send_node_id << "\n";
      s_source_x.push_back (mypos.x);
      s_source_y.push_back (mypos.y);
      s_time.push_back (Simulator::Now ().GetMicroSeconds ());
      s_hop.push_back (hopcount);
      s_pri_1_id.push_back (pri_id[1]);
      s_pri_2_id.push_back (pri_id[2]);
      s_pri_3_id.push_back (pri_id[3]);
      s_pri_4_id.push_back (pri_id[4]);
      s_pri_5_id.push_back (pri_id[5]);
      s_pri_1_r.push_back (m_rt[pri_id[1]]);
      s_pri_2_r.push_back (m_rt[pri_id[2]]);
      s_pri_3_r.push_back (m_rt[pri_id[3]]);
      s_pri_4_r.push_back (m_rt[pri_id[4]]);
      s_pri_5_r.push_back (m_rt[pri_id[5]]);
      s_des_id.push_back (des_id);
      s_send_log.push_back (0); //一旦0にする send関数で本当にsendしたら1 に変える
      m_send_check[des_id] = sendpacketCount;

      sendpacketCount++;
      ////////////////////////交差点判定
      if (distinctionRoad (m_xpoint[pri_id[1]], m_ypoint[pri_id[1]]) ==
          0) //優先度 iのノードが交差点ノードならば
        {
          s_inter_1_id.push_back (1);
        }
      else
        {
          s_inter_1_id.push_back (0);
        }

      if (distinctionRoad (m_xpoint[pri_id[2]], m_ypoint[pri_id[2]]) ==
          0) //優先度 iのノードが交差点ノードならば
        {
          s_inter_2_id.push_back (1);
        }
      else
        {
          s_inter_2_id.push_back (0);
        }

      if (distinctionRoad (m_xpoint[pri_id[3]], m_ypoint[pri_id[3]]) ==
          0) //優先度 iのノードが交差点ノードならば
        {
          s_inter_3_id.push_back (1);
        }
      else
        {
          s_inter_3_id.push_back (0);
        }

      if (distinctionRoad (m_xpoint[pri_id[4]], m_ypoint[pri_id[4]]) ==
          0) //優先度 iのノードが交差点ノードならば
        {
          s_inter_4_id.push_back (1);
        }
      else
        {
          s_inter_4_id.push_back (0);
        }

      if (distinctionRoad (m_xpoint[pri_id[5]], m_ypoint[pri_id[5]]) ==
          0) //優先度 iのノードが交差点ノードならば
        {
          s_inter_5_id.push_back (1);
        }
      else
        {
          s_inter_5_id.push_back (0);
        }

      m_recvcount.clear ();
      m_first_recv_time.clear ();
      m_etx.clear ();
      m_pri_value.clear ();
      m_rt.clear (); //保持している予想伝送確率をクリア

      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();

      SendHeader sendHeader (des_id, des_x, des_y, send_node_id, mypos.x, mypos.y, hopcount,
                             pri_id[1], pri_id[2], pri_id[3], pri_id[4], pri_id[5]);

      packet->AddHeader (sendHeader);

      TypeHeader tHeader (SHUTOUSHUTYPE_SEND);
      packet->AddHeader (tHeader);

      int32_t wait_time = (pri_value * WaitT) - WaitT; //待ち時間
      m_wait[des_id] = hopcount; //今から待機するホップカウント

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

      if (pri_value == 0) //初期のソースノードなら無条件にbroadcast
        {
          socket->SendTo (packet, 0, InetSocketAddress (destination, SHUTOUSHU_PORT));
          std::cout << "id " << send_node_id
                    << " source node broadcast----------------------------------------------------"
                    << "time" << Simulator::Now ().GetMicroSeconds () << "\n";
        }
      else
        {
          if (pri_id[1] != 10000000) //中継候補ノードをⅠ個も持っていないノードはbrさせない
            Simulator::Schedule (MicroSeconds (wait_time), &RoutingProtocol::SendToShutoushu, this,
                                 socket, packet, destination, hopcount, des_id);
        }
    }
} // namespace shutoushu

void
RoutingProtocol::RecvShutoushu (Ptr<Socket> socket)
{
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition ();

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

        HelloHeader helloheader;
        packet->RemoveHeader (helloheader); //近隣ノードからのhello packet
        int32_t recv_hello_id = helloheader.GetNodeId (); //NOde ID
        int32_t recv_hello_posx = helloheader.GetPosX (); //Node xposition
        int32_t recv_hello_posy = helloheader.GetPosY (); //Node yposition
        int32_t recv_hello_time = Simulator::Now ().GetMicroSeconds (); //

        // // ////*********recv hello packet log*****************////////////////
        // std::cout << "Node ID " << id << "が受信したHello packetは"
        //           << "id:" << recv_hello_id << "xposition" << recv_hello_posx << "yposition"
        //           << recv_hello_posy << "\n";
        // std::cout << " shuto protocol  hello receive  id " << id << "  time  " << Simulator::Now ().GetMicroSeconds () << "\n";
        // // ////*********************************************////////////////
        SaveXpoint (recv_hello_id, recv_hello_posx);
        SaveYpoint (recv_hello_id, recv_hello_posy);
        SaveRecvTime (recv_hello_id, recv_hello_time);
        SaveRelation (recv_hello_id, recv_hello_posx, recv_hello_posy);
        break; //breakがないとエラー起きる
      }
      case SHUTOUSHUTYPE_SEND: {

        SendHeader sendheader;
        packet->RemoveHeader (sendheader);

        int32_t des_id = sendheader.GetDesId ();
        int32_t des_x = sendheader.GetPosX ();
        int32_t des_y = sendheader.GetPosY ();
        int32_t send_id = sendheader.GetSendId ();
        int32_t send_x = sendheader.GetSendPosX ();
        int32_t send_y = sendheader.GetSendPosY ();
        int32_t hopcount = sendheader.GetHopcount ();
        int32_t pri_id[] = {sendheader.GetId1 (), sendheader.GetId2 (), sendheader.GetId3 (),
                            sendheader.GetId4 (), sendheader.GetId5 ()};

        if (des_id == id) //宛先が自分だったら
          {

            std::cout << "time" << Simulator::Now ().GetMicroSeconds () << "  id" << id
                      << "受信しましたよ　成功しました-------------\n";
            if (m_finish_time[des_id] == 0)
              {
                m_finish_time[des_id] = Simulator::Now ().GetMicroSeconds ();

                p_source_x.push_back (send_x);
                p_source_y.push_back (send_y);
                p_recv_x.push_back (mypos.x);
                p_recv_y.push_back (mypos.y);
                p_recv_time.push_back (Simulator::Now ().GetMicroSeconds ());
                p_recv_priority.push_back (id); //dummy 0
                p_hopcount.push_back (hopcount);
                p_recv_id.push_back (id);
                p_source_id.push_back (send_id);
                p_destination_id.push_back (des_id);
                p_destination_x.push_back (des_x);
                p_destination_y.push_back (des_y);
                p_pri_1.push_back (pri_id[0]);
                p_pri_2.push_back (pri_id[1]);
                p_pri_3.push_back (pri_id[2]);
                p_pri_4.push_back (pri_id[3]);
                p_pri_5.push_back (pri_id[4]);
                packetCount++;
              }
            break;
          }
        // int32_t pri1_id = sendheader.GetId1 ();
        // int32_t pri2_id = sendheader.GetId2 ();
        // int32_t pri3_id = sendheader.GetId3 ();
        // int32_t pri4_id = sendheader.GetId4 ();
        // int32_t pri5_id = sendheader.GetId5 ();

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

        if (m_wait[des_id] != 0) //待ち状態ならば
          {
            if (m_wait[des_id] >= hopcount)
              { //待機中のホップカウントより大きいホップカウントを受け取ったなら
                m_wait.erase (des_id);
              }

            for (int i = 0; i < 5; i++)
              {
                if (id == pri_id[i]) //packetに自分のIDが含まれているか
                  {
                    // //std::cout << "\n--------------------------------------------------------\n";
                    std::cout << "関係あるrecv id" << id << "time------------------------------\n"
                              << Simulator::Now ().GetMicroSeconds ();
                    if (m_finish_time[des_id] == 0)
                      {
                        p_source_x.push_back (send_x);
                        p_source_y.push_back (send_y);
                        p_recv_x.push_back (mypos.x);
                        p_recv_y.push_back (mypos.y);
                        p_recv_time.push_back (Simulator::Now ().GetMicroSeconds ());
                        p_recv_priority.push_back (i + 1);
                        p_hopcount.push_back (hopcount);
                        p_recv_id.push_back (id);
                        p_source_id.push_back (send_id);
                        p_destination_id.push_back (des_id);
                        p_destination_x.push_back (des_x);
                        p_destination_y.push_back (des_y);
                        p_pri_1.push_back (pri_id[0]);
                        p_pri_2.push_back (pri_id[1]);
                        p_pri_3.push_back (pri_id[2]);
                        p_pri_4.push_back (pri_id[3]);
                        p_pri_5.push_back (pri_id[4]);
                        packetCount++;
                      }
                    SendShutoushuBroadcast (i + 1, des_id, des_x, des_y, hopcount);
                  }
                else //含まれていないか
                  {
                    //m_wait.erase (des_id); //マップの初期化をして　broadcastを止める
                  }
              }
          }
        else //待ち状態じゃないならば
          {
            for (int i = 0; i < 5; i++)
              {
                if (id == pri_id[i]) //packetに自分のIDが含まれているか
                  {
                    //std::cout << "\n--------------------------------------------------------\n";
                    std::cout << "関係あるrecv id" << id << "time------------------------------"
                              << Simulator::Now ().GetMicroSeconds () << "\n";
                    if (m_finish_time[des_id] == 0)
                      {

                        p_source_x.push_back (send_x);
                        p_source_y.push_back (send_y);
                        p_recv_x.push_back (mypos.x);
                        p_recv_y.push_back (mypos.y);
                        p_recv_time.push_back (Simulator::Now ().GetMicroSeconds ());
                        p_recv_priority.push_back (i + 1);
                        p_hopcount.push_back (hopcount);
                        p_recv_id.push_back (id);
                        p_source_id.push_back (send_id);
                        p_destination_id.push_back (des_id);
                        p_destination_x.push_back (des_x);
                        p_destination_y.push_back (des_y);
                        p_pri_1.push_back (pri_id[0]);
                        p_pri_2.push_back (pri_id[1]);
                        p_pri_3.push_back (pri_id[2]);
                        p_pri_4.push_back (pri_id[3]);
                        p_pri_5.push_back (pri_id[4]);
                        packetCount++;
                      }
                    SendShutoushuBroadcast (i + 1, des_id, des_x, des_y, hopcount);
                  }
                else //含まれていないか
                  {
                  }
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

//ノード間の関係性を更新するメソッド
void
RoutingProtocol::SaveRelation (int32_t map_id, int32_t map_xpoint, int32_t map_ypoint)
{
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<Node> ()->GetObject<MobilityModel> ();
  Vector mypos = mobility->GetPosition ();
  int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  int myRoadId = distinctionRoad (mypos.x, mypos.y); //自分の道路ID
  int NeighborRoadId = distinctionRoad (map_xpoint, map_ypoint); // 近隣ノードの道路ID
  int currentRelation = 0; //現在ノードとの関係性
  if (myRoadId == 0 || NeighborRoadId == 0) // どちらかが交差点道路なら
    {
      currentRelation = 1; //同一道路なら1
    }

  if (myRoadId == NeighborRoadId)
    {
      currentRelation = 1; //同一道路なら1
    }
  else
    {
      currentRelation = 2; //異なる道路なら2
    }
  if (m_relation[map_id] != 0) //以前にリンクを持っていたら
    {
      if (currentRelation != m_relation[map_id]) //以前と関係性が異なるなら
        {
          if (NeighborRoadId == 0 || myRoadId == 0) // どちらかが交差点ノードの場合
            {
              currentRelation = m_relation[map_id]; //前の関係性を維持する
            }
          else
            {
              if (id == 0)
                {
                  std::cout << "以前と関係性が違います　破棄する前の取得数 " << m_recvtime.size ()
                            << "\n";
                  std::cout << "関係性が変わった IDは" << id << "pare id" << map_id << "\n";
                  //m_recvtime.erase (map_id); // helloパケット取得履歴を破棄
                  std::cout << "破棄後の取得数 " << m_recvtime.size () << "\n";
                }
            }
        }
    }
  m_relation[map_id] = currentRelation; // 現在の関係をマップに保存
}

void
RoutingProtocol::SaveRecvTime (int32_t map_id, int32_t map_recvtime)
{
  //int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  m_recvtime.insert (std::make_pair (map_id, map_recvtime));
}

void
RoutingProtocol::SendXBroadcast (void)
{
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
  // std::cout<<"id"<<id<<"x:"<<m_my_posx[id] << "y:"<<m_my_posy[id] << "\n";
}

int
RoutingProtocol::distinctionRoad (int x_point, int y_point)
{
  /// 道路1〜30  31〜61
  int gridRange = 200;
  int x = 0;
  int y = 0;
  int colomnCount = 1;
  int interRange = 15; //交差点の大きさ interRange × interRange の正方形

  for (int roadId = 1; roadId <= 60; roadId++)
    {
      if (roadId <= 30)
        {
          if (x + interRange < x_point && x_point < x + gridRange - interRange &&
              y - interRange < y_point && y_point < y + interRange)
            {
              return roadId;
            }
          if (colomnCount == 5)
            {
              colomnCount = 1;
              x = 0;
              y = y + gridRange;
            }
          else
            {
              x = x + gridRange;
              colomnCount++;
            }

          if (roadId == 30)
            {
              x = 0;
              y = 0;
            }
        }
      else //31〜
        {
          if (x - interRange < x_point && x_point < x + interRange && y + interRange < y_point &&
              y_point < y + gridRange - interRange)
            {
              return roadId;
            }
          if (colomnCount == 6)
            {
              colomnCount = 1;
              x = 0;
              y = y + gridRange;
            }
          else
            {
              x = x + gridRange;
              colomnCount++;
            }
        }
    }
  return 0; // ０を返す = 交差点ノード
}

void
RoutingProtocol::RoadCenterPoint (void)
{
  int count = 1;
  int gridRange = 200;

  for (int roadId = 1; roadId <= 60; roadId++)
    {
      if (roadId <= 30)
        {
          if (count == 5) //7のときcount変数を初期化
            {
              count = 1;
              roadCenterPointX[roadId] = roadCenterPointX[roadId - 1] + gridRange;
              roadCenterPointY[roadId] = roadCenterPointY[roadId - 1];
            }
          else // 5以外は足していく
            {
              if (roadId == 1)
                {
                  roadCenterPointX[roadId] = 100;
                  roadCenterPointY[roadId] = 0;
                }
              else if (count == 1)
                {
                  roadCenterPointX[roadId] = 100;
                  roadCenterPointY[roadId] = roadCenterPointY[roadId - 1] + 200;
                }
              else
                {
                  roadCenterPointX[roadId] = roadCenterPointX[roadId - 1] + gridRange;
                  roadCenterPointY[roadId] = roadCenterPointY[roadId - 1];
                }
              count++;
            }

          if (roadId == 30)
            {
              count = 1;
            }
        }
      else //31〜
        {
          if (count == 6) //6のときcount変数を初期化
            {
              count = 1;
              roadCenterPointX[roadId] = roadCenterPointX[roadId - 1] + gridRange;
              roadCenterPointY[roadId] = roadCenterPointY[roadId - 1];
            }
          else // 6以外は足していく
            {
              if (roadId == 31)
                {
                  roadCenterPointX[roadId] = 0;
                  roadCenterPointY[roadId] = 100;
                }
              else if (count == 1)
                {
                  roadCenterPointX[roadId] = 0;
                  roadCenterPointY[roadId] = roadCenterPointY[roadId - 1] + gridRange;
                }
              else
                {
                  roadCenterPointX[roadId] = roadCenterPointX[roadId - 1] + gridRange;
                  roadCenterPointY[roadId] = roadCenterPointY[roadId - 1];
                }
              count++;
            }
        }
    }
}

// シミュレーション結果の出力関数
void
RoutingProtocol::SimulationResult (void) //
{
  std::cout << "time" << Simulator::Now ().GetSeconds () << "\n";
  // int32_t id = m_ipv4->GetObject<Node> ()->GetId ();
  if (Simulator::Now ().GetSeconds () == Grobal_StartTime + Grobal_SourceNodeNum + 1)
    {
      // //*******************************ノードが持つ座標の確認ログ***************************//
      //std::cout << "id=" << id << "の\n";
      // for (auto itr = m_xpoint.begin (); itr != m_xpoint.end (); itr++)
      //   {
      //     std::cout << "recv hello packet id = " << itr->first // キーを表示
      //               << ", x座標 = " << itr->second << "\n"; // 値を表示
      //   }
      // for (auto itr = m_ypoint.begin (); itr != m_ypoint.end (); itr++)
      //   {
      //     std::cout << "recv hello packet id = " << itr->first // キーを表示
      //               << ", y座標 = " << itr->second << "\n"; // 値を表示
      //   }
      //**************************************************************************************//

      //*******************************ノードが持つ受信時刻ログ***************************//
      // if (id == 14)
      //   std::cout << "id=" << id << "recv数" << m_recvtime.size () << "\n";

      // auto itr = m_recvtime.find (1);
      // if (itr != m_recvtime.end ())
      //   { // 見つかった場合
      //     for (auto itr = m_recvtime.begin (); itr != m_recvtime.end (); itr++)
      //       {
      //         std::cout << itr->first << " " << itr->second << "\n"; //  キー、値を表示
      //       }
      //   }
      // itr = m_recvtime.find (1);
      // if (itr == m_recvtime.end ())
      //   { // 見つからなかった場合
      //     std::cout << "not found.\n";
      //   }
      //**************************************************************************************//
      int sum_end_time = 0;
      int sum_br = 0; //ブロードキャスト数の平均
      double average_end_time = 0;
      int recvCount = 0;
      std::cout << "\n\n\n結果出力----------------------------------\n\n";
      for (auto itr = broadcount.begin (); itr != broadcount.end (); itr++)
        {
          std::cout << "des id " << itr->first << "shutoushu broadcast数" << broadcount[itr->first]
                    << "\n";
          sum_br += broadcount[itr->first];
        }
      for (auto itr = m_start_time.begin (); itr != m_start_time.end (); itr++)
        {
          std::cout << "des id " << itr->first << "送信開始時刻" << m_start_time[itr->first]
                    << "\n";
        }
      for (auto itr = m_finish_time.begin (); itr != m_finish_time.end (); itr++)
        {
          if (m_finish_time[itr->first] != 0) // 受信回数分回る
            {
              std::cout << "des id " << itr->first << "受信時刻" << m_finish_time[itr->first]
                        << "\n";
              int end_to_end_time = m_finish_time[itr->first] - m_start_time[itr->first];
              sum_end_time += end_to_end_time;
              std::cout << "destination id = " << itr->first
                        << "end to end deley time = " << end_to_end_time << "\n";
              recvCount++;
            }
        }
      std::cout << "sum_br" << sum_br << "\n";
      std::cout << "recvCount" << recvCount << "\n";
      std::cout << "sum_end_time" << sum_end_time << "\n";
      double average_overhead = (double) sum_br / (double) recvCount;
      double packet_recv_rate = (double) recvCount / (double) m_start_time.size ();
      average_end_time = (double) sum_end_time / (double) recvCount;
      std::cout << "本シミュレーションのパケット到達率は" << packet_recv_rate << "\n";
      std::cout << "本シミュレーションのパケットEnd to End遅延時間は" << average_end_time << "\n";
      std::cout << "本シミュレーションのパケット平均オーバーヘッドは" << average_overhead << "\n";
      std::cout << "交差点ノードにおける重み付けは" << InterPoint << "\n";
      std::cout << "本シミュレーションのシミュレーション開始時刻は" << Grobal_StartTime << "\n";
      std::cout << "送信数は" << m_start_time.size () << "\n";
      std::cout << "受信数は" << recvCount << "\n";
      std::cout << "PDRテスト" << m_finish_time.size () / m_start_time.size () << "\n";
      std::cout << "Seed値は" << Grobal_Seed << "\n";
      std::cout << "車両数は" << numVehicle << "\n";
      std::cout << "trans probability" << TransProbability << "\n";

      std::string filename = "data/shutoushu/shutoushu-seed_" + std::to_string (Grobal_Seed) + "nodenum_" +
                             std::to_string (numVehicle) + ".csv";
      std::string send_filename = "data/send_shutoushu/shutoushu-seed_" + std::to_string (Grobal_Seed) +
                                  "nodenum_" + std::to_string (numVehicle) + ".csv";

      std::ofstream packetTrajectory (filename);
      packetTrajectory << "source_x"
                       << ","
                       << "source_y"
                       << ","
                       << "recv_x"
                       << ","
                       << "recv_y"
                       << ","
                       << "time"
                       << ","
                       << "recv_priority"
                       << ","
                       << "hopcount"
                       << ","
                       << "recv_id"
                       << ","
                       << "source_id"
                       << ","
                       << "destination_id"
                       << ","
                       << "destination_x"
                       << ","
                       << "destination_y"
                       << ","
                       << "pri_1"
                       << ","
                       << "pri_2"
                       << ","
                       << "pri_3"
                       << ","
                       << "pri_4"
                       << ","
                       << "pri_5" << std::endl;
      std::ofstream send_packetTrajectory (send_filename);
      send_packetTrajectory << "source_id"
                            << ","
                            << "source_x"
                            << ","
                            << "source_y"
                            << ","
                            << "time"
                            << ","
                            << "hop"
                            << ","
                            << "pri_1_id"
                            << ","
                            << "pri_2_id"
                            << ","
                            << "pri_3_id"
                            << ","
                            << "pri_4_id"
                            << ","
                            << "pri_5_id"
                            << ","
                            << "pri_1_r"
                            << ","
                            << "pri_2_r"
                            << ","
                            << "pri_3_r"
                            << ","
                            << "pri_4_r"
                            << ","
                            << "pri_5_r"
                            << ","
                            << "des_id"
                            << ","
                            << "inter_1id "
                            << ","
                            << "inter_2id "
                            << ","
                            << "inter_3id "
                            << ","
                            << "inter_4id "
                            << ","
                            << "inter_5id "
                            << ","
                            << "send_log " << std::endl;

      for (int i = 0; i < packetCount; i++)
        {

          packetTrajectory << p_source_x[i] << ", " << p_source_y[i] << ", " << p_recv_x[i] << ", "
                           << p_recv_y[i] << ", " << p_recv_time[i] << ", " << p_recv_priority[i]
                           << ", " << p_hopcount[i] << ", " << p_recv_id[i] << ", "
                           << p_source_id[i] << ", " << p_destination_id[i] << ", "
                           << p_destination_x[i] << ", " << p_destination_y[i] << ", " << p_pri_1[i]
                           << ", " << p_pri_2[i] << ", " << p_pri_3[i] << ", " << p_pri_4[i] << ", "
                           << p_pri_5[i] << std::endl;
        }
      for (int i = 0; i < sendpacketCount; i++)
        {
          send_packetTrajectory << s_source_id[i] << ", " << s_source_x[i] << ", " << s_source_y[i]
                                << ", " << s_time[i] << ", " << s_hop[i] << ", " << s_pri_1_id[i]
                                << ", " << s_pri_2_id[i] << ", " << s_pri_3_id[i] << ", "
                                << s_pri_4_id[i] << ", " << s_pri_5_id[i] << ", " << s_pri_1_r[i]
                                << ", " << s_pri_2_r[i] << ", " << s_pri_3_r[i] << ", "
                                << s_pri_4_r[i] << ", " << s_pri_5_r[i] << ", " << s_des_id[i]
                                << ", " << s_inter_1_id[i] << ", " << s_inter_2_id[i] << ", "
                                << s_inter_3_id[i] << ", " << s_inter_4_id[i] << ", "
                                << s_inter_5_id[i] << ", " << s_send_log[i] << std::endl;
        }
    }
}

std::map<int, int> RoutingProtocol::broadcount; //key 0 value broudcast数
std::map<int, int> RoutingProtocol::m_start_time; //key destination_id value　送信時間
std::map<int, int> RoutingProtocol::m_finish_time; //key destination_id value 受信時間
std::map<int, double> RoutingProtocol::m_my_posx; // key node id value position x
std::map<int, double> RoutingProtocol::m_my_posy; // key node id value position y
std::vector<int> RoutingProtocol::source_list;
std::vector<int> RoutingProtocol::des_list;

//パケット軌跡出力用の変数
std::vector<int> RoutingProtocol::p_source_x;
std::vector<int> RoutingProtocol::p_source_y;
std::vector<int> RoutingProtocol::p_recv_x;
std::vector<int> RoutingProtocol::p_recv_y;
std::vector<int> RoutingProtocol::p_recv_time;
std::vector<int> RoutingProtocol::p_recv_priority;
std::vector<int> RoutingProtocol::p_hopcount;
std::vector<int> RoutingProtocol::p_recv_id;
std::vector<int> RoutingProtocol::p_source_id;
std::vector<int> RoutingProtocol::p_destination_id;
std::vector<int> RoutingProtocol::p_destination_x;
std::vector<int> RoutingProtocol::p_destination_y;
std::vector<int> RoutingProtocol::p_pri_1;
std::vector<int> RoutingProtocol::p_pri_2;
std::vector<int> RoutingProtocol::p_pri_3;
std::vector<int> RoutingProtocol::p_pri_4;
std::vector<int> RoutingProtocol::p_pri_5;
//送信側ログ用変数
std::vector<int> RoutingProtocol::s_source_id;
std::vector<int> RoutingProtocol::s_source_x;
std::vector<int> RoutingProtocol::s_source_y;
std::vector<int> RoutingProtocol::s_time;
std::vector<int> RoutingProtocol::s_hop;
std::vector<int> RoutingProtocol::s_pri_1_id;
std::vector<int> RoutingProtocol::s_pri_2_id;
std::vector<int> RoutingProtocol::s_pri_3_id;
std::vector<int> RoutingProtocol::s_pri_4_id;
std::vector<int> RoutingProtocol::s_pri_5_id;
std::vector<double> RoutingProtocol::s_pri_1_r;
std::vector<double> RoutingProtocol::s_pri_2_r;
std::vector<double> RoutingProtocol::s_pri_3_r;
std::vector<double> RoutingProtocol::s_pri_4_r;
std::vector<double> RoutingProtocol::s_pri_5_r;
std::vector<int> RoutingProtocol::s_des_id;
std::vector<int> RoutingProtocol::s_inter_1_id; //文字列で交差点にいるIDをぶち込む
std::vector<int> RoutingProtocol::s_inter_2_id;
std::vector<int> RoutingProtocol::s_inter_3_id;
std::vector<int> RoutingProtocol::s_inter_4_id;
std::vector<int> RoutingProtocol::s_inter_5_id;
std::vector<int> RoutingProtocol::s_send_log;

} // namespace shutoushu
} // namespace ns3
