#include <iostream>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( InternetDatagram dgram, const Address& next_hop )
{
  uint32_t next_hop_ip = next_hop.ipv4_numeric();
  
 
  auto it = arp_cache_.find( next_hop_ip );
  
  if ( it != arp_cache_.end() && current_time_ms_ - it->second.second < ARP_CACHE_TTL_MS ) {
    
    EthernetFrame frame;
    frame.header.dst = it->second.first; 
    frame.header.src = ethernet_address_; 
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.payload = serialize( dgram );
    
    transmit( frame );
  } else {
    
    
    waiting_datagrams_[next_hop_ip].push( dgram );
    
    auto arp_it = pending_arp_requests_.find( next_hop_ip );
    
    if ( arp_it == pending_arp_requests_.end() || 
         current_time_ms_ - arp_it->second >= ARP_REQUEST_TTL_MS ) {
    
      
      ARPMessage arp_request;
      arp_request.opcode = ARPMessage::OPCODE_REQUEST;
      arp_request.sender_ethernet_address = ethernet_address_;
      arp_request.sender_ip_address = ip_address_.ipv4_numeric();
      arp_request.target_ethernet_address = {};  
      arp_request.target_ip_address = next_hop_ip;
      
      EthernetFrame arp_frame;
      arp_frame.header.dst = ETHERNET_BROADCAST; 
      arp_frame.header.src = ethernet_address_;
      arp_frame.header.type = EthernetHeader::TYPE_ARP;
      arp_frame.payload = serialize( arp_request );
      
      transmit( arp_frame );
      
      
      pending_arp_requests_[next_hop_ip] = current_time_ms_;
    }
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return;
  }
  
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) ) {
      datagrams_received_.push( dgram );
    }
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp_msg;
    if ( parse( arp_msg, frame.payload ) ) {
      
      
      arp_cache_[arp_msg.sender_ip_address] = { arp_msg.sender_ethernet_address, current_time_ms_ };
      
      
      auto waiting_it = waiting_datagrams_.find( arp_msg.sender_ip_address );
      if ( waiting_it != waiting_datagrams_.end() ) {
        
        pending_arp_requests_.erase( arp_msg.sender_ip_address );
        
        while ( !waiting_it->second.empty() ) {
          InternetDatagram dgram = waiting_it->second.front();
          waiting_it->second.pop();
          
          
          EthernetFrame out_frame;
          out_frame.header.dst = arp_msg.sender_ethernet_address;
          out_frame.header.src = ethernet_address_;
          out_frame.header.type = EthernetHeader::TYPE_IPv4;
          out_frame.payload = serialize( dgram );
          
          transmit( out_frame );
        }
        waiting_datagrams_.erase( waiting_it );
      }
      
      if ( arp_msg.opcode == ARPMessage::OPCODE_REQUEST && 
           arp_msg.target_ip_address == ip_address_.ipv4_numeric() ) {
        ARPMessage arp_reply;
        arp_reply.opcode = ARPMessage::OPCODE_REPLY;
        arp_reply.sender_ethernet_address = ethernet_address_;
        arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
        arp_reply.target_ethernet_address = arp_msg.sender_ethernet_address;
        arp_reply.target_ip_address = arp_msg.sender_ip_address;
        
        EthernetFrame reply_frame;
        reply_frame.header.dst = arp_msg.sender_ethernet_address;
        reply_frame.header.src = ethernet_address_;
        reply_frame.header.type = EthernetHeader::TYPE_ARP;
        reply_frame.payload = serialize( arp_reply );
        
        transmit( reply_frame );
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  current_time_ms_ += ms_since_last_tick;
  
  
  for ( auto it = arp_cache_.begin(); it != arp_cache_.end(); ) {
    if ( current_time_ms_ - it->second.second >= ARP_CACHE_TTL_MS ) {
      it = arp_cache_.erase( it );
    } else {
      ++it;
    }
  }
  
  
  for ( auto it = pending_arp_requests_.begin(); it != pending_arp_requests_.end(); ) {
    if ( current_time_ms_ - it->second >= ARP_REQUEST_TTL_MS ) {
      
      waiting_datagrams_.erase( it->first ); 

      
      it = pending_arp_requests_.erase( it );
    } else {
      ++it;
    }
  }
}