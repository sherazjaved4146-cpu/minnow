// #pragma once

// #include "exception.hh"
// #include "network_interface.hh"

// #include <optional>

// // \brief A router that has multiple network interfaces and
// // performs longest-prefix-match routing between them.
// class Router
// {
// public:
//   // Add an interface to the router
//   // \param[in] interface an already-constructed network interface
//   // \returns The index of the interface after it has been added to the router
//   size_t add_interface( std::shared_ptr<NetworkInterface> interface )
//   {
//     interfaces_.push_back( notnull( "add_interface", std::move( interface ) ) );
//     return interfaces_.size() - 1;
//   }

//   // Access an interface by index
//   std::shared_ptr<NetworkInterface> interface( const size_t N ) { return interfaces_.at( N ); }

//   // Add a route (a forwarding rule)
//   void add_route( uint32_t route_prefix,
//                   uint8_t prefix_length,
//                   std::optional<Address> next_hop,
//                   size_t interface_num );

//   // Route packets between the interfaces
//   void route();

// private:
//   // The router's collection of network interfaces
//   std::vector<std::shared_ptr<NetworkInterface>> interfaces_ {};
//    // Routing table entry structure
//   struct Route {
//   uint32_t route_prefix = 0;
//   uint8_t prefix_length = 0;
//   std::optional<Address> next_hop = std::nullopt;
//   size_t interface_num = 0;
// };
  
//    std::vector<Route> routing_table_ {};
// };
#pragma once
#include "network_interface.hh"
#include <optional>
#include <queue>
#include <vector>

// A wrapper for NetworkInterface that makes the host-side
// interface asynchronous: instead of returning received datagrams
// immediately, it queues them for later retrieval.
struct noisy_interface
{
  std::shared_ptr<NetworkInterface> interface;
  std::queue<InternetDatagram> datagrams_received {};

  noisy_interface( std::shared_ptr<NetworkInterface> interface_ ) : interface( interface_ ) {}
};
class Router
{
public:
  // Add an interface to the router
  // interface_num: index of the interface after it is added
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    _interfaces.push_back( noisy_interface( interface ) );
    return _interfaces.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( size_t N ) { return _interfaces.at( N ).interface; }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

private:
  // Structure to hold a single route entry
  struct Route
  {
    uint32_t route_prefix = 0;
    uint8_t prefix_length = 0;
    std::optional<Address> next_hop = std::nullopt;
    size_t interface_num = 0;
  };

  // Routing table - stores all routes
  std::vector<Route> routing_table_ {};

  // The router's collection of network interfaces
  std::vector<noisy_interface> _interfaces {};
};
