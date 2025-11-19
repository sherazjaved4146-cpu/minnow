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

  uint64_t checkpoint = reassembler_.writer().bytes_pushed() + 1;
  uint64_t abs_seqno = message.seqno.unwrap( isn_.value(), checkpoint );
  
  uint64_t first_data_index = abs_seqno + message.SYN;

  if ( first_data_index == 0 ) {
    return;    // ignore invalid byte before stream
  }

  uint64_t stream_index = (first_data_index > 0) ? (first_data_index - 1) : 0;

  uint64_t first_unacceptable = reassembler_.writer().bytes_pushed()
                                + reassembler_.writer().available_capacity();
  
  if (stream_index >= first_unacceptable) {
    return;
  }

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