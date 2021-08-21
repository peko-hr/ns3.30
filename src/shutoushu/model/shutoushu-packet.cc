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
#include "shutoushu-packet.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3 {
namespace shutoushu {

NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (MessageType t) : m_type (t), m_valid (true)
{
}

TypeId
TypeHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::shutoushu::TypeHeader")
                          .SetParent<Header> ()
                          .SetGroupName ("Shutoushu")
                          .AddConstructor<TypeHeader> ();
  return tid;
}

TypeId
TypeHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
TypeHeader::GetSerializedSize () const
{
  return 1;
}

void
TypeHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 ((uint8_t) m_type);
}

uint32_t
TypeHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t type = i.ReadU8 ();
  m_valid = true;
  switch (type)
    {
    // case SHUTOUSHUTYPE_RREQ:
    // case SHUTOUSHUTYPE_RREP:
    // case SHUTOUSHUTYPE_RERR:
    // case SHUTOUSHUTYPE_RREP_ACK:
    case SHUTOUSHUTYPE_SEND:
      case SHUTOUSHUTYPE_HELLO: {
        m_type = (MessageType) type;
        break;
      }
    default:
      m_valid = false;
    }
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
TypeHeader::Print (std::ostream &os) const
{
  switch (m_type)
    {
      // case SHUTOUSHUTYPE_RREQ: {
      //   os << "RREQ";
      //   break;
      // }
      // case SHUTOUSHUTYPE_RREP: {
      //   os << "RREP";
      //   break;
      // }
      // case SHUTOUSHUTYPE_RERR: {
      //   os << "RERR";
      //   break;
      // }
      // case SHUTOUSHUTYPE_RREP_ACK: {
      //   os << "RREP_ACK";
      //   break;
      // }
      case SHUTOUSHUTYPE_SEND: {
        os << "SEND";
        break;
      }
      case SHUTOUSHUTYPE_HELLO: {
        os << "HELLO";
        break;
      }
    default:
      os << "UNKNOWN_TYPE";
    }
}

bool
TypeHeader::operator== (TypeHeader const &o) const
{
  return (m_type == o.m_type && m_valid == o.m_valid);
}

std::ostream &
operator<< (std::ostream &os, TypeHeader const &h)
{
  h.Print (os);
  return os;
}

// ***********************start SHUTOUSHU_HELLO*************************************//
HelloHeader::HelloHeader (int32_t nodeid, int32_t posx, int32_t posy)
    : m_nodeid (nodeid), m_posx (posx), m_posy (posy)
{
}
NS_OBJECT_ENSURE_REGISTERED (HelloHeader);

TypeId
HelloHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::shutoushu::HelloHeader")
                          .SetParent<Header> ()
                          .SetGroupName ("Shutoushu")
                          .AddConstructor<HelloHeader> ();
  return tid;
}

TypeId
HelloHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
HelloHeader::GetSerializedSize () const
{
  return 12;
}

void
HelloHeader::Serialize (Buffer::Iterator i) const //シリアル化
{
  i.WriteHtonU32 (m_nodeid);
  i.WriteHtonU32 (m_posx);
  i.WriteHtonU32 (m_posy);
}

uint32_t
HelloHeader::Deserialize (Buffer::Iterator start) //逆シリアル化
{
  Buffer::Iterator i = start;

  m_nodeid = i.ReadNtohU32 ();
  m_posx = i.ReadNtohU32 ();
  m_posy = i.ReadNtohU32 ();

  uint32_t dist2 = i.GetDistanceFrom (start);

  NS_ASSERT (dist2 == GetSerializedSize ());
  return dist2;
}

void
HelloHeader::Print (std::ostream &os) const
{
  // os << "NodeId " << m_nodeid;
  // os << "NodePointX " << m_posx;
  // os << "NodePointY" << m_posy;
}
std::ostream &
operator<< (std::ostream &os, HelloHeader const &h)
{
  h.Print (os);
  return os;
}

// ***********************end SHUTOUSHU_HELLO*************************************//

//***********************start SHUTOUSHU Send*************************************//

SendHeader::SendHeader (int32_t des_id, int32_t posx, int32_t posy, int32_t send_id,
                        int32_t send_posx, int32_t send_posy, int32_t hopcount, int32_t pri1_id,
                        int32_t pri2_id, int32_t pri3_id, int32_t pri4_id,
                        int32_t pri5_id)
    : m_des_id (des_id), //目的ノードID
      m_posx (posx), //目的地座標
      m_posy (posy),
      m_send_id (send_id),
      m_send_posx (send_posx),
      m_send_posy (send_posy),
      m_hopcount (hopcount),
      m_pri1_id (pri1_id), //優先度１
      m_pri2_id (pri2_id), //優先度２
      m_pri3_id (pri3_id), //優先度３
      m_pri4_id (pri4_id), //優先度４
      m_pri5_id (pri5_id) //優先度５
{
}
NS_OBJECT_ENSURE_REGISTERED (SendHeader);

TypeId
SendHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::shutoushu::SendHeader")
                          .SetParent<Header> ()
                          .SetGroupName ("Shutoushu")
                          .AddConstructor<SendHeader> ();
  return tid;
}

TypeId
SendHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
SendHeader::GetSerializedSize () const
{
  return 48;
}

void
SendHeader::Serialize (Buffer::Iterator i) const //シリアル化
{
  i.WriteHtonU32 (m_des_id);
  i.WriteHtonU32 (m_posx);
  i.WriteHtonU32 (m_posy);
  i.WriteHtonU32 (m_send_id);
  i.WriteHtonU32 (m_send_posx);
  i.WriteHtonU32 (m_send_posy);
  i.WriteHtonU32 (m_hopcount);
  i.WriteHtonU32 (m_pri1_id);
  i.WriteHtonU32 (m_pri2_id);
  i.WriteHtonU32 (m_pri3_id);
  i.WriteHtonU32 (m_pri4_id);
  i.WriteHtonU32 (m_pri5_id);
}

uint32_t
SendHeader::Deserialize (Buffer::Iterator start) //逆シリアル化
{
  Buffer::Iterator i = start;

  m_des_id = i.ReadNtohU32 ();
  m_posx = i.ReadNtohU32 ();
  m_posy = i.ReadNtohU32 ();
  m_send_id = i.ReadNtohU32 ();
  m_send_posx = i.ReadNtohU32 ();
  m_send_posy = i.ReadNtohU32 ();
  m_hopcount = i.ReadNtohU32 ();
  m_pri1_id = i.ReadNtohU32 ();
  m_pri2_id = i.ReadNtohU32 ();
  m_pri3_id = i.ReadNtohU32 ();
  m_pri4_id = i.ReadNtohU32 ();
  m_pri5_id = i.ReadNtohU32 ();

  uint32_t dist = i.GetDistanceFrom (start);

  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
SendHeader::Print (std::ostream &os) const
{
}

std::ostream &
operator<< (std::ostream &os, SendHeader const &h)
{
  h.Print (os);
  return os;
}

//**********************end SHUTOUSHU Send***************************************//

} // namespace shutoushu
} // namespace ns3
