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
#ifndef SIGOPACKET_H
#define SIGOPACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include <map>
#include "ns3/nstime.h"

namespace ns3 {
namespace sigo {

/**
* \ingroup sigo
* \brief MessageType enumeration
*/
enum MessageType {
  SIGOTYPE_SEND = 1, //SIGO SEND
  SIGOTYPE_HELLO = 2 //SIGO HELLO

};

/**
* \ingroup sigo
* \brief SIGO types
*/
class TypeHeader : public Header
{
public:
  /**
   * constructor
   * \param t the SIGO RREQ type
   */
  TypeHeader (MessageType t = SIGOTYPE_HELLO);
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  /**
   * \returns the type
   */
  MessageType
  Get () const
  {
    return m_type;
  }
  /**
   * Check that type if valid
   * \returns true if the type is valid
   */
  bool
  IsValid () const
  {
    return m_valid;
  }
  /**
   * \brief Comparison operator
   * \param o header to compare
   * \return true if the headers are equal
   */
  bool operator== (TypeHeader const &o) const;

private:
  MessageType m_type; ///< type of the message
  bool m_valid; ///< Indicates if the message is valid
};

/**
  * \brief Stream output operator
  * \param os output stream
  * \return updated stream
  */
std::ostream &operator<< (std::ostream &os, TypeHeader const &h);

///////////////////////////////////////////////////////////////////////////////////////////////
//*******************************start Hello SIGO***********************************************/
/**
* \ingroup sigo
* \brief helloheader
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     send Node ID                              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Send Node Xpostion                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Send Node Yposition                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Send Node past Xpostion                   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Send Node past Yposition                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  
  |                     Send Node acceration                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  
  \endverbatim
*/

class HelloHeader : public Header
{
public:
  /*
nodeid Hellomessageを送信する車両のID
posx posy Hellomessageを送信する車両の位置情報
*/
  HelloHeader (int32_t nodeid = 0, int32_t posx = 0, int32_t posy = 0, int32_t pposx = 0, int32_t pposy = 0, int32_t acce = 0);

  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  void
  SetNodeId (uint32_t id) //IDをセットする
  {
    m_nodeid = id;
  }

  int32_t
  GetNodeId () const //IDを返す
  {
    return m_nodeid;
  }
  void
  SetPosX (int32_t p) //座標をセットする
  { //座標をセットする
    m_posx = p;
  }

  int32_t
  GetPosX () //座標の値を返す
  {
    return m_posx;
  }
  void
  SetPosY (int32_t p) //座標をセットする
  { //座標をセットする
    m_posy = p;
  }

  int32_t
  GetPosY () //座標の値を返す
  {
    return m_posy;
  }

  void
  SetPPosX (int32_t p) //座標をセットする
  { //座標をセットする
    m_pposx = p;
  }

  int32_t
  GetPPosX () //座標の値を返す
  {
    return m_pposx;
  }
  void
  SetPPosY (int32_t p) //座標をセットする
  { //座標をセットする
    m_pposy = p;
  }

  int32_t
  GetPPosY () //座標の値を返す
  {
    return m_pposy;
  }

  void
  SetAcce (int32_t a) //座標の値を返す
  {
    m_acce = a;
  }

  int32_t
  GetAcce () //座標の値を返す
  {
    return m_acce;
  }

  bool operator== (HelloHeader const &o);

private:
  int32_t m_nodeid; //ノードID
  int32_t m_posx; // x座標
  int32_t m_posy; // y座標
  int32_t m_pposx; // 過去のx座標
  int32_t m_pposy; // 過去のy座標
  int32_t m_acce; // 加速度
};

//*******************************end Hello SIGO***********************************************/

//*******************************start send SIGO*********************************************/
/**
* \ingroup sigo
* \brief sendheader
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                       Destination Id                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Destination X position                    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Destination Y position                    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         hopcount                             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Priority1 Id                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Priority2 Id                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Priority3 Id                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Priority4 Id                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                         Priority5 Id                          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/

class SendHeader : public Header
{
public:
  SendHeader (int32_t des_id = 0, int32_t posx = 0, int32_t posy = 0, int32_t send_id = 0,
              int32_t send_posx = 0, int32_t send_posy = 0, int32_t hopcount = 0,
              int32_t pri1_id = 0, int32_t pri2_id = 0, int32_t pri3_id = 0, int32_t pri4_id = 0,
              int32_t pri5_id = 0);

  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;

  void
  SetDesId (uint32_t id) //IDをセットする
  {
    m_des_id = id;
  }

  int32_t
  GetDesId () const //IDを返す
  {
    return m_des_id;
  }
  void
  SetPosX (int32_t p) //座標をセットする
  { //座標をセットする
    m_posx = p;
  }

  int32_t
  GetPosX () //座標の値を返す
  {
    return m_posx;
  }
  void
  SetPosY (int32_t p) //座標をセットする
  { //座標をセットする
    m_posy = p;
  }

  int32_t
  GetPosY () //座標の値を返す
  {
    return m_posy;
  }
  void
  SetSendId (uint32_t id) //IDをセットする
  {
    m_send_id = id;
  }

  int32_t
  GetSendId () const //IDを返す
  {
    return m_send_id;
  }
  void
  SetSendPosX (int32_t p) //座標をセットする
  { //座標をセットする
    m_send_posx = p;
  }

  int32_t
  GetSendPosX () //座標の値を返す
  {
    return m_send_posx;
  }
  void
  SetSendPosY (int32_t p) //座標をセットする
  { //座標をセットする
    m_send_posy = p;
  }

  int32_t
  GetSendPosY () //座標の値を返す
  {
    return m_send_posy;
  }

  void
  SetHopcount (int32_t hopcount) //座標をセットする
  { //座標をセットする
    m_hopcount = hopcount;
  }

  int32_t
  GetHopcount () //座標の値を返す
  {
    return m_hopcount;
  }

  void
  SetId1 (int32_t id1)
  {
    m_pri1_id = id1;
  }

  //Time GetRecvTime () const;
  int32_t
  GetId1 ()
  {
    return m_pri1_id;
  }

  void
  SetId2 (int32_t id2)
  {
    m_pri2_id = id2;
  }

  //Time GetRecvTime () const;
  int32_t
  GetId2 ()
  {
    return m_pri2_id;
  }

  void
  SetId3 (int32_t id3)
  {
    m_pri3_id = id3;
  }

  //Time GetRecvTime () const;
  int32_t
  GetId3 ()
  {
    return m_pri3_id;
  }

  void
  SetId4 (int32_t id4)
  {
    m_pri4_id = id4;
  }

  //Time GetRecvTime () const;
  int32_t
  GetId4 ()
  {
    return m_pri4_id;
  }

  void
  SetId5 (int32_t id5)
  {
    m_pri5_id = id5;
  }

  //Time GetRecvTime () const;
  int32_t
  GetId5 ()
  {
    return m_pri5_id;
  }

  bool operator== (SendHeader const &o);

private:
  int32_t m_des_id; //目的ノードID
  int32_t m_posx; //目的地座標
  int32_t m_posy;
  int32_t m_send_id; //送信ノードのID
  int32_t m_send_posx; // 送信ノードの座標
  int32_t m_send_posy; // 送信ノードの座標
  int32_t m_hopcount; //ホップカウント
  int32_t m_pri1_id; //優先度１
  int32_t m_pri2_id; //優先度２
  int32_t m_pri3_id; //優先度３
  int32_t m_pri4_id; //優先度４
  int32_t m_pri5_id; //優先度５
};

std::ostream &operator<< (std::ostream &os, SendHeader const &);

/********************************finish send SIGO*******************************************/

} // namespace sigo
} // namespace ns3

#endif /* SIGOPACKET_H */
