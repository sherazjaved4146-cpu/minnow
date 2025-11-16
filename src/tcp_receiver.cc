#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reassembler_.reader().set_error();
    return;
  }

  if ( message.SYN ) {
    isn_ = message.seqno;
  }

  if ( !isn_.has_value() ) {
    return;
  }

  // Use bytes_pushed + 1 as checkpoint (the +1 accounts for SYN)
  uint64_t checkpoint = reassembler_.writer().bytes_pushed() + 1;
  uint64_t abs_seqno = message.seqno.unwrap( isn_.value(), checkpoint );
  
  // Calculate stream index
  // abs_seqno 0 = SYN, abs_seqno 1 = first data byte (stream index 0)
  // If this segment has SYN, the data (if any) starts at abs_seqno + 1
  uint64_t first_data_index = abs_seqno + message.SYN;
  
  // Convert to stream index (SYN occupies abs_seqno 0, so subtract 1)
  uint64_t stream_index = (first_data_index > 0) ? (first_data_index - 1) : 0;

  // Always call insert - reassembler will handle empty payloads
  reassembler_.insert( stream_index, message.payload, message.FIN );
}


TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage message;

  if ( isn_.has_value() ) {
    uint64_t bytes_written = reassembler_.writer().bytes_pushed();
    uint64_t abs_ackno = 1 + bytes_written;
    
    if ( reassembler_.writer().is_closed() ) {
      abs_ackno += 1;
    }
    
    message.ackno = Wrap32::wrap( abs_ackno, isn_.value() );
  }

  uint64_t capacity = reassembler_.writer().available_capacity();
  
  if ( capacity > UINT16_MAX ) {
    message.window_size = UINT16_MAX;
  } else {
    message.window_size = static_cast<uint16_t>( capacity );
  }

  message.RST = reassembler_.reader().has_error();

  return message;
}