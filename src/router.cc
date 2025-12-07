// #include "router.hh"

// #include <iostream>
// #include <limits>

// using namespace std;

// // route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// // prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
// //    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// // next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
// //    which case, the next hop address should be the datagram's final destination).
// // interface_num: The index of the interface to send the datagram out on.
// void Router::add_route( const uint32_t route_prefix,
//                         const uint8_t prefix_length,
//                         const optional<Address> next_hop,
//                         const size_t interface_num )
// {
//   cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
//        << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
//        << " on interface " << interface_num << "\n";

//   // Create a new route entry
//   Route new_route;
//   new_route.route_prefix = route_prefix;
//   new_route.prefix_length = prefix_length;
//   new_route.next_hop = next_hop;
//   new_route.interface_num = interface_num;
  
//   // Add it to the routing table
//   routing_table_.push_back( new_route );
// }

// // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
// void Router::route()
// {
//   // Process each interface
//   for ( auto& iface_ptr : interfaces_ ) {
//     // Get reference to the interface
//     auto& iface = *iface_ptr;
    
//     // Get the queue of received datagrams
//     auto& received_queue = iface.datagrams_received();
    
//     // Process all datagrams in the queue
//     while ( !received_queue.empty() ) {
//       // Get the datagram from front of queue
//       auto dgram = received_queue.front();
//       received_queue.pop();
      
//       // Extract destination IP address
//       const uint32_t dst_ip = dgram.header.dst;
      
//       // Find the best matching route (longest prefix match)
//       const Route* best_route = nullptr;
      
//       for ( const auto& route : routing_table_ ) {
//         // Calculate the bit mask for this route's prefix length
//         uint32_t mask;
//         if ( route.prefix_length == 0 ) {
//           mask = 0; // Default route (0.0.0.0/0) matches everything
//         } else if ( route.prefix_length == 32 ) {
//           mask = 0xFFFFFFFF; // Exact IP match
//         } else {
//           // Create mask: prefix_length=24 -> 0xFFFFFF00
//           mask = ~( ( 1u << ( 32 - route.prefix_length ) ) - 1 );
//         }
        
//         // Check if this route matches the destination IP
//         if ( ( dst_ip & mask ) == ( route.route_prefix & mask ) ) {
//           // This route matches! Is it better than our current best?
//           if ( best_route == nullptr || route.prefix_length > best_route->prefix_length ) {
//             best_route = &route;
//           }
//         }
//       }
      
//       // If no route matched, drop the datagram
//       if ( best_route == nullptr ) {
//         continue; // Drop it
//       }
      
//       // Check TTL (Time To Live)
//       if ( dgram.header.ttl <= 1 ) {
//         continue; // TTL expired, drop it
//       }
      
//       // Decrement TTL
//       dgram.header.ttl--;
      
//       // Recompute IP header checksum (because we modified TTL)
//       dgram.header.compute_checksum();
      
//       // Determine the next hop address
//       Address next_hop_addr;
//       if ( best_route->next_hop.has_value() ) {
//         // Send to the next router
//         next_hop_addr = best_route->next_hop.value();
//       } else {
//         // Direct connection - next hop is the destination itself
//         next_hop_addr = Address::from_ipv4_numeric( dst_ip );
//       }
      
//       // Send the datagram out the appropriate interface
//       interface( best_route->interface_num )->send_datagram( dgram, next_hop_addr );
//     }
//   }
// }
#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  Route new_route;
  new_route.route_prefix = route_prefix;
  new_route.prefix_length = prefix_length;
  new_route.next_hop = next_hop;
  new_route.interface_num = interface_num;
  
  routing_table_.push_back( new_route );
}

void Router::route()
{
  // Process each interface
  for ( size_t i = 0; i < _interfaces.size(); i++ ) {
    auto& iface = _interfaces[i];
    
    // Get the queue of received datagrams
    auto& received_queue = iface.datagrams_received;
    
    // Process all datagrams in the queue
    while ( !received_queue.empty() ) {
      // Get the datagram from front of queue
      auto dgram = received_queue.front();
      received_queue.pop();
      
      // Extract destination IP address
      const uint32_t dst_ip = dgram.header.dst;
      
      // Find the best matching route (longest prefix match)
      const Route* best_route = nullptr;
      
      for ( const auto& route : routing_table_ ) {
        // Calculate the bit mask for this route's prefix length
        uint32_t mask;
        if ( route.prefix_length == 0 ) {
          mask = 0; // Default route
        } else if ( route.prefix_length == 32 ) {
          mask = 0xFFFFFFFF; // Exact match
        } else {
          // Create mask: prefix_length=24 -> 0xFFFFFF00
          mask = ~0u << ( 32 - route.prefix_length );
        }
        
        // Check if this route matches the destination IP
        if ( ( dst_ip & mask ) == ( route.route_prefix & mask ) ) {
          // This route matches! Is it better than our current best?
          if ( best_route == nullptr || route.prefix_length > best_route->prefix_length ) {
            best_route = &route;
          }
        }
      }
      
      // If no route matched, drop the datagram
      if ( best_route == nullptr ) {
        continue;
      }
      
      // Check TTL (Time To Live)
      if ( dgram.header.ttl <= 1 ) {
        continue;
      }
      
      // Decrement TTL
      dgram.header.ttl--;
      
      // Recompute IP header checksum
      dgram.header.compute_checksum();
      
      // Determine the next hop address
      Address next_hop_addr;
      if ( best_route->next_hop.has_value() ) {
        // Send to the next router
        next_hop_addr = best_route->next_hop.value();
      } else {
        // Direct connection - next hop is the destination itself
        next_hop_addr = Address::from_ipv4_numeric( dst_ip );
      }
      
      // Send the datagram out the appropriate interface
      // Use the interface() helper method from router.hh
      interface( best_route->interface_num )->send_datagram( dgram, next_hop_addr );
    }
  }
}