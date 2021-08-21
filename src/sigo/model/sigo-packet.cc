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
#include "sigo-packet.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3 {
namespace sigo {

NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (MessageType t) : m_type (t), m_valid (true)
{
}

TypeId
TypeHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::sigo::TypeHeader")
                          .SetParent<Header> ()
                          .SetGroupName ("Sigo")
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
    // case SIGOTYPE_RREQ:
    // case SIGOTYPE_RREP:
    // case SIGOTYPE_RERR:
    // case SIGOTYPE_RREP_ACK:
    case SIGOTYPE_SEND:
      case SIGOTYPE_HELLO: {
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
      // case SIGOTYPE_RREQ: {
      //   os << "RREQ";
      //   break;
      // }
      // case SIGOTYPE_RREP: {
      //   os << "RREP";
      //   break;
      // }
      // case SIGOTYPE_RERR: {
      //   os << "RERR";
      //   break;
      // }
      // case SIGOTYPE_RREP_ACK: {
      //   os << "RREP_ACK";
      //   break;
      // }
      case SIGOTYPE_SEND: {
        os << "SEND";
        break;
      }
      case SIGOTYPE_HELLO: {
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

// ***********************start SIGO_HELLO*************************************//
HelloHeader::HelloHeader (int32_t nodeid, int32_t posx, int32_t posy, int32_t pposx, int32_t pposy, int32_t acce)
    : m_nodeid (nodeid), m_posx (posx), m_posy (posy), m_pposx(pposx), m_pposy(pposy), m_acce(acce)
{
}
NS_OBJECT_ENSURE_REGISTERED (HelloHeader);

TypeId
HelloHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::sigo::HelloHeader")
                          .SetParent<Header> ()
                          .SetGroupName ("Sigo")
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
  return 24;
}

void
HelloHeader::Serialize (Buffer::Iterator i) const //シリアル化
{
  i.WriteHtonU32 (m_nodeid);
  i.WriteHtonU32 (m_posx);
  i.WriteHtonU32 (m_posy);
  i.WriteHtonU32 (m_pposx);
  i.WriteHtonU32 (m_pposy);
  i.WriteHtonU32 (m_acce);
}

uint32_t
HelloHeader::Deserialize (Buffer::Iterator start) //逆シリアル化
{
  Buffer::Iterator i = start;

  m_nodeid = i.ReadNtohU32 ();
  m_posx = i.ReadNtohU32 ();
  m_posy = i.ReadNtohU32 ();
  m_pposx = i.ReadNtohU32 ();
  m_pposy = i.ReadNtohU32 ();
  m_acce = i.ReadNtohU32 ();

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

// ***********************end SIGO_HELLO*************************************//

//***********************start SIGO Send*************************************//

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
  static TypeId tid = TypeId ("ns3::sigo::SendHeader")
                          .SetParent<Header> ()
                          .SetGroupName ("Sigo")
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

//**********************end SIGO Send***************************************//

} // namespace sigo
} // namespace ns3
