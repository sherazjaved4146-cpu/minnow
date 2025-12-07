#include "debug.hh"
#include "socket.hh"
#include "tcp_minnow_socket.hh"  // ADDED
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

namespace {
void get_URL( const string& host, const string& path )
{
  CS144TCPSocket socket;  // CHANGED from TCPSocket
  socket.connect(Address(host, "http"));
  
  string request = "GET " + path + " HTTP/1.1\r\n";
  request += "Host: " + host + "\r\n";
  request += "Connection: close\r\n";
  request += "\r\n";
  
  socket.write(request);
  
  while (!socket.eof()) {
    string response;
    socket.read(response);
    cout << response;
  }
}
} // namespace

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort();
    }
    auto args = span( argv, argc );
    
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }
    
    const string host { args[1] };
    const string path { args[2] };
    
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}