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
#ifndef SAMPLEPACKET_H
#define SAMPLEPACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include <map>
#include "ns3/nstime.h"

namespace ns3 {
namespace sample {

/**
* \ingroup sample
* \brief MessageType enumeration
*/
enum MessageType {
  SAMPLETYPE_RREQ = 1, //!< SAMPLETYPE_RREQ
  SAMPLETYPE_RREP = 2, //!< SAMPLETYPE_RREP
  SAMPLETYPE_RERR = 3, //!< SAMPLETYPE_RERR
  SAMPLETYPE_RREP_ACK = 4, //!< SAMPLETYPE_RREP_ACK
  SAMPLETYPE_HELLO = 5
};

/**
* \ingroup sample
* \brief SAMPLE types
*/
class TypeHeader : public Header
{
public:
  /**
   * constructor
   * \param t the SAMPLE RREQ type
   */
  TypeHeader (MessageType t = SAMPLETYPE_HELLO);

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

/**
* \ingroup sample
* \brief Route Reply (RREP) Message Format
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |     Type      |R|A|    Reserved     |Prefix Sz|   Hop Count   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Destination IP address                    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                  Destination Sequence Number                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Originator IP address                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           Lifetime                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/
class RrepHeader : public Header
{
public:
  /**
   * constructor
   *
   * \param prefixSize the prefix size (0)
   * \param hopCount the hop count (0)
   * \param dst the destination IP address
   * \param dstSeqNo the destination sequence number
   * \param origin the origin IP address
   * \param lifetime the lifetime
   */
  RrepHeader (uint8_t prefixSize = 0, uint8_t hopCount = 0, Ipv4Address dst = Ipv4Address (),
              uint32_t dstSeqNo = 0, Ipv4Address origin = Ipv4Address (),
              Time lifetime = MilliSeconds (0));
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

  // Fields
  /**
   * \brief Set the hop count
   * \param count the hop count
   */
  void
  SetHopCount (uint8_t count)
  {
    m_hopCount = count;
  }
  /**
   * \brief Get the hop count
   * \return the hop count
   */
  uint8_t
  GetHopCount () const
  {
    return m_hopCount;
  }
  /**
   * \brief Set the destination address
   * \param a the destination address
   */
  void
  SetDst (Ipv4Address a)
  {
    m_dst = a;
  }
  /**
   * \brief Get the destination address
   * \return the destination address
   */
  Ipv4Address
  GetDst () const
  {
    return m_dst;
  }
  /**
   * \brief Set the destination sequence number
   * \param s the destination sequence number
   */
  void
  SetDstSeqno (uint32_t s)
  {
    m_dstSeqNo = s;
  }
  /**
   * \brief Get the destination sequence number
   * \return the destination sequence number
   */
  uint32_t
  GetDstSeqno () const
  {
    return m_dstSeqNo;
  }
  /**
   * \brief Set the origin address
   * \param a the origin address
   */
  void
  SetOrigin (Ipv4Address a)
  {
    m_origin = a;
  }
  /**
   * \brief Get the origin address
   * \return the origin address
   */
  Ipv4Address
  GetOrigin () const
  {
    return m_origin;
  }
  /**
   * \brief Set the lifetime
   * \param t the lifetime
   */
  void SetLifeTime (Time t);
  /**
   * \brief Get the lifetime
   * \return the lifetime
   */
  Time GetLifeTime () const;

  // Flags
  /**
   * \brief Set the ack required flag
   * \param f the ack required flag
   */
  void SetAckRequired (bool f);
  /**
   * \brief get the ack required flag
   * \return the ack required flag
   */
  bool GetAckRequired () const;
  /**
   * \brief Set the prefix size
   * \param sz the prefix size
   */
  void SetPrefixSize (uint8_t sz);
  /**
   * \brief Set the pefix size
   * \return the prefix size
   */
  uint8_t GetPrefixSize () const;

  /**
   * Configure RREP to be a Hello message
   *
   * \param src the source IP address
   * \param srcSeqNo the source sequence number
   * \param lifetime the lifetime of the message
   */
  void SetHello (Ipv4Address src, uint32_t srcSeqNo, Time lifetime);

  /**
   * \brief Comparison operator
   * \param o RREP header to compare
   * \return true if the RREP headers are equal
   */
  bool operator== (RrepHeader const &o) const;

private:
  uint8_t m_flags; ///< A - acknowledgment required flag
  uint8_t m_prefixSize; ///< Prefix Size
  uint8_t m_hopCount; ///< Hop Count
  Ipv4Address m_dst; ///< Destination IP Address
  uint32_t m_dstSeqNo; ///< Destination Sequence Number
  Ipv4Address m_origin; ///< Source IP Address
  uint32_t m_lifeTime; ///< Lifetime (in milliseconds)
};

/**
  * \brief Stream output operator
  * \param os output stream
  * \return updated stream
  */
std::ostream &operator<< (std::ostream &os, RrepHeader const &);

///////////////////////////////////////////////////////////////////////////////////////////////
//*******************************start Hello SAMPLE***********************************************/
/**
* \ingroup sample
* \brief sampleheader
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
  \endverbatim
*/

class HelloHeader : public Header
{
public:
  /*
nodeid Hellomessageを送信する車両のID
posx posy Hellomessageを送信する車両の位置情報
*/
  HelloHeader (int32_t nodeid = 0, int32_t posx = 0, int32_t posy = 0);

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

  bool operator== (HelloHeader const &o);

private:
  int32_t m_nodeid; //ノードID
  int32_t m_posx; // x座標
  int32_t m_posy; // y座標
};

//*******************************end Hello Sample***********************************************/

} // namespace sample
} // namespace ns3

#endif /* SAMPLEPACKET_H */
