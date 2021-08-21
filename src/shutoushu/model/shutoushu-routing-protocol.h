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
 * 
 * 実行コマンド
 * ~/shuto/workspace30/ns-3-allinone/ns-3.30
 * 提案プロトコル
 * sudo ./waf --run "Lsgo-SimulationScenario --buildings=1  --protocol=7 --lossModel=4 --scenario=3"
 * LSGO
 * sudo ./waf --run "Lsgo-SimulationScenario --buildings=1  --protocol=6 --lossModel=4 --scenario=3"
 * 
 * シナリオコード
 * 車両数に注意
 * NodeNum
 * 
 *
 * Author: Shuto Takahashi <is0361er@ed.ritsumei.ac.jp>
 *         Ritsumeikan University, Shiga, Japan
 */
#ifndef SHUTOUSHUROUTINGPROTOCOL_H
#define SHUTOUSHUROUTINGPROTOCOL_H

#include "shutoushu-packet.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include <map>

#define maxHop 20
#define SimTime 40 //シミュレーション時間 second
#define WindowSize 10000000 //SHUTOUSHUのウィンドウサイズ microsecond   = 10second
#define HelloInterval 1 //Hello packet のインターバル
#define WaitT 4000 //待ち時間の差 micro単位
#define InterPoint 1.0 //交差点ノードの与えるポイントの重み付け
// #define SimStartTime 10 //broadcast 開始時刻　秒
// #define Seed 10000 // ※毎回変える
#define TransProbability 1.2 //予想伝送確率の閾値
#define AngleGamma 0.4 // ガンマ変換　
#define RpGamma 1.0
// #define SourceNodeNum 10

#define SourceLowX -50
#define SourceHighX 300
#define SourceLowY -50
#define SourceHighY 300

#define DesLowX 550
#define DesHighX 850
#define DesLowY 550
#define DesHighY 850

//各protocol共通のグローバル変数　
extern int Buildings; //grobal shadowing ありなし shadowing 1 noshadowing 0
extern int Grobal_Seed; //grobal seed値
extern int Grobal_StartTime; //grobal data packet  送信開始タイム
extern int Grobal_SourceNodeNum; //grobal data packet 送信ノード数
extern int Grobal_m_beta; //grobal shadowing parameter 
extern int Grobal_m_gamma;
extern double Grobal_InterPoint; //grobal 交差点ノードのポイント重み付け


namespace ns3 {
namespace shutoushu {
/**
 * \ingroup shutoushu
 *
 * \brief SHUTOUSHU routing protocol
 */

class RoutingProtocol : public Ipv4RoutingProtocol
{
public:
  /**
   * \brief Get the type ID.
   * \return the object 
   */
  static TypeId GetTypeId (void);
  static const uint32_t SHUTOUSHU_PORT;

  //自作グローバル変数
  static std::map<int, int> broadcount; //key destination id value ブロードキャスト数
  static std::map<int, int> m_start_time; //key destination_id value　送信時間
  static std::map<int, int> m_finish_time; //key destination_id value 受信時間
  static std::map<int, double> m_my_posx; // key node id value position x
  static std::map<int, double> m_my_posy; // key node id value position y
  static std::vector<int> source_list; //指定エリアにいるsource node 候補 insertされるのはノードID
  static std::vector<int> des_list;

  //パケット軌跡出力用の変数 recv
  static std::vector<int> p_source_x;
  static std::vector<int> p_source_y;
  static std::vector<int> p_recv_x;
  static std::vector<int> p_recv_y;
  static std::vector<int> p_recv_time;
  static std::vector<int> p_recv_priority;
  static std::vector<int> p_hopcount;
  static std::vector<int> p_recv_id;
  static std::vector<int> p_source_id;
  static std::vector<int> p_destination_id;
  static std::vector<int> p_destination_x;
  static std::vector<int> p_destination_y;
  static std::vector<int> p_pri_1;
  static std::vector<int> p_pri_2;
  static std::vector<int> p_pri_3;
  static std::vector<int> p_pri_4;
  static std::vector<int> p_pri_5;

  //logfile send
  static std::vector<int> s_source_id;
  static std::vector<int> s_source_x;
  static std::vector<int> s_source_y;
  static std::vector<int> s_time;
  static std::vector<int> s_hop;
  static std::vector<int> s_pri_1_id;
  static std::vector<int> s_pri_2_id;
  static std::vector<int> s_pri_3_id;
  static std::vector<int> s_pri_4_id;
  static std::vector<int> s_pri_5_id;
  static std::vector<double> s_pri_1_r;
  static std::vector<double> s_pri_2_r;
  static std::vector<double> s_pri_3_r;
  static std::vector<double> s_pri_4_r;
  static std::vector<double> s_pri_5_r;
  static std::vector<int> s_des_id;
  static std::vector<int>
      s_inter_1_id; //優先度１が交差点に属するかどうか 1 or 0 　　　　１なら交差点にいると判断した
  static std::vector<int> s_inter_2_id;
  static std::vector<int> s_inter_3_id;
  static std::vector<int> s_inter_4_id;
  static std::vector<int> s_inter_5_id;
  static std::vector<int> s_send_log; //sendされたら1 キャンセルされたら0

  /// constructor
  RoutingProtocol ();
  virtual ~RoutingProtocol ();
  virtual void DoDispose ();

  // Inherited from Ipv4RoutingProtocol
  Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif,
                              Socket::SocketErrno &sockerr);
  bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                   LocalDeliverCallback lcb, ErrorCallback ecb);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

protected:
  virtual void DoInitialize (void);

private:
  void SendXBroadcast (void);
  void RecvShutoushu (Ptr<Socket> socket);

  //**自作メソッド start**/
  int getDistance (double x, double y, double x2, double y2);
  double getAngle (double a_x, double a_y, double b_x, double b_y, double c_x, double c_y);
  void SendHelloPacket (void); //hello packet を broadcast するメソッド
  void SimulationResult (void); //シミュレーション結果を出力する
  void SendToHello (Ptr<Socket> socket, Ptr<Packet> packet,
                    Ipv4Address destination); //Hello packet の送信
  void SendToShutoushu (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination,
                        int32_t hopcount, int32_t des_id); //Shutoushu broadcast の送信
  void SaveXpoint (int32_t map_id, int32_t map_xpoint); //近隣ノードのHello packetの情報を
  void SaveYpoint (int32_t map_id, int32_t map_ypoint); //mapに保存する
  void SaveRecvTime (int32_t map_id, int32_t map_recvtime); //
  void SaveRelation (int32_t map_id, int32_t map_xpoint,
                     int32_t map_ypoint); //近隣ノードの道路IDを保存
  void
  SendShutoushuBroadcast (int32_t pri_value, int32_t des_id, int32_t des_x, int32_t des_y,
                          int32_t hopcount); //候補ノードの優先順位を計算してpacketを送信する関数
  void SetCountTimeMap (void); //window size より古いmapを削除していく関数
  void SetEtxMap (void); //etx map をセットする関数
  void SetPriValueMap (int32_t des_x, int32_t des_y); //優先度を決める値を格納する　関数

  void SetMyPos (void); //自分の位置情報を１秒ずつ保存
  void ReadFile (void); //mobility fileの読み取り
  void WriteFile (void); //packet traceのためのファイル書き込み
  void Trans (int node_id); //通信許可を与える関数
  void NoTrans (int node_id); //通信不許可を与える関数
  void Send (void); //シミュレーションソースIDとDestinationIDを指定する関数
  int distinctionRoad (int x_point,
                       int y_ypoint); //ｘ座標とy座標から道路番号を割り出す関数　return 道路番号
  void RoadCenterPoint (); //roadの中心座標を格納するだけの関数
  int NearRoadId (int32_t des_x, int32_t des_y); //目的地に最も近い道路IDを返す関数
  double CalculateRp (int nearRoadId); //近い道路IDを受取その道路のRpを返す
  void SourceAndDestination (void); //source,destinationの指定エリアに存在する

  //**map**//
  std::map<int, int> m_xpoint; //近隣車両の位置情報を取得するmap  key=nodeid value=xposition
  std::map<int, int> m_ypoint; //近隣車両の位置情報を取得するmap  key=nodeid value=yposition
  std::multimap<int, int>
      m_recvtime; //hello messageを取得した時間を保存するマップ　key = NodeId value=recvtime
  std::map<int, int>
      m_relation; //近隣ノードとの関係性 key = nodeid value=  同一道路1 or　異なる道路 2
  std::map<int, int> m_send_check; //key = destination_id value = send_logファイルのindex

  ///以下のマップは使ったら消去する
  std::map<int, int> m_recvcount; //windows size以下のMAPの取得回数
  std::map<int, int> m_first_recv_time; //近隣ノードからのWINDOWSIZE内の最初の取得時間
  std::map<int, double> m_etx; //近隣ノードのkeyがIDでvalueがETX値
  std::map<int, double> m_rt; //近隣ノードの keyがIDでvalueが予想伝送確率
  std::map<int, double> m_pri_value; //ノードの優先度を図る値　大きいほど優先度が高い
  std::map<int, int> m_wait; //key destination_id value ホップカウント

  //destination に対してこのホップカウントで送信待機している状態を表す
  //**自作メソッド finish**/

  /// IP protocol
  Ptr<Ipv4> m_ipv4;
  /// Nodes IP address
  Ipv4Address m_mainAddress;
  /// Raw unicast socket per each IP interface, map socket -> iface address (IP + mask)
  std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketAddresses;
  /// Raw subnet directed broadcast socket per each IP interface, map socket -> iface address (IP + mask)
  std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketSubnetBroadcastAddresses;

  Ptr<NetDevice> m_lo;

  /// Provides uniform random variables.
  Ptr<UniformRandomVariable> m_uniformRandomVariable;
};

} //namespace shutoushu
} //namespace ns3

#endif /* SHUTOUSHUROUTINGPROTOCOL_H */
