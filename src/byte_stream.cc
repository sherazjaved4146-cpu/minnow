#include "byte_stream.hh"
#include<algorithm>
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer_() {}
// Push data to stream, but only as much as available capacity allows.
void Writer::push( string data )
{
  uint64_t bytes_to_push = min( data.size(), available_capacity() );
  buffer_ += data.substr( 0, bytes_to_push );
  bytes_pushed_ += bytes_to_push;
}

// Signal that the stream has reached its ending. Nothing more will be written.
void Writer::close(){
  closed_=true;
}

// Has the stream been closed?
bool Writer::is_closed() const
{
  return closed_; // Your code here.
}

// How many bytes can be pushed to the stream right now?
uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_-buffer_.size();
}

// Total number of bytes cumulatively pushed to the stream
uint64_t Writer::bytes_pushed() const
{
   // Your code here.
   return bytes_pushed_;
}

// Peek at the next bytes in the buffer -- ideally as many as possible.
// It's not required to return a string_view of the *whole* buffer, but
// if the peeked string_view is only one byte at a time, it will probably force
// the caller to do a lot of extra work.
string_view Reader::peek() const
{
   // Your code here.
  return string_view { buffer_ };
}

// Remove `len` bytes from the buffer.
void Reader::pop( uint64_t len )
{
  uint64_t bytes_to_pop = min( len, buffer_.size() );
  buffer_.erase( 0, bytes_to_pop );
  bytes_popped_ += bytes_to_pop;  
}

// Is the stream finished (closed and fully popped)?
bool Reader::is_finished() const
{
  // Your code here.
    return closed_ && buffer_.empty();
}

// Number of bytes currently buffered (pushed and not popped)
uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer_.size();
}

// Total number of bytes cumulatively popped from stream
uint64_t Reader::bytes_popped() const
{
   // Your code here.
   return bytes_popped_;
}
