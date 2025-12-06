#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return bytes_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retrans_;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap( next_abs_seqno_, isn_ );
  msg.RST = writer().has_error();  // Set RST if stream has error
  return msg;
}

void TCPSender::send_segment( const TCPSenderMessage& msg, const TransmitFunction& transmit )
{
  transmit( msg );
  
  // Track this segment as outstanding if it occupies sequence space
  if ( msg.sequence_length() > 0 ) {
    outstanding_segments_.push( msg );
    bytes_in_flight_ += msg.sequence_length();
    
    // Start timer if not running
    if ( !timer_running_ ) {
      start_timer();
    }
  }
}

void TCPSender::start_timer()
{
  timer_running_ = true;
  timer_ms_ = 0;
}

void TCPSender::stop_timer()
{
  timer_running_ = false;
  timer_ms_ = 0;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Determine effective window size (treat 0 as 1)
  uint64_t effective_window = ( recv_window_size_ == 0 ) ? 1 : recv_window_size_;
  
  // Keep sending while we have space in the window
  while ( true ) {
    // Calculate how many bytes are currently in flight
    uint64_t bytes_in_flight = next_abs_seqno_ - recv_ackno_;
    
    // Check if window is full
    if ( bytes_in_flight >= effective_window ) {
      break;
    }
    
    uint64_t space_available = effective_window - bytes_in_flight;
    
    // Create a new segment
    TCPSenderMessage msg;
    msg.seqno = Wrap32::wrap( next_abs_seqno_, isn_ );
    msg.RST = writer().has_error();  // ADD THIS LINE - Set RST if stream has error
    
    bool something_sent = false;
    
    // Add SYN if not sent yet
    if ( !syn_sent_ ) {
      msg.SYN = true;
      syn_sent_ = true;
      next_abs_seqno_++;
      space_available--;
      something_sent = true;
    }
    
    // Add payload if available
    if ( space_available > 0 && reader().bytes_buffered() > 0 ) {
      uint64_t payload_size = min( space_available, TCPConfig::MAX_PAYLOAD_SIZE );
      payload_size = min( payload_size, reader().bytes_buffered() );
      
      read( reader(), payload_size, msg.payload );
      next_abs_seqno_ += payload_size;
      space_available -= payload_size;
      something_sent = true;
    }
    
    // Add FIN if stream is finished and not sent yet
    if ( !fin_sent_ && reader().is_finished() && space_available > 0 ) {
      msg.FIN = true;
      fin_sent_ = true;
      next_abs_seqno_++;
      something_sent = true;
    }
    
    // Send the segment if we have something to send
    if ( something_sent ) {
      send_segment( msg, transmit );
    } else {
      // Nothing to send, exit loop
      break;
    }
    
    // If we've sent everything (no more data and FIN sent), stop
    if ( reader().bytes_buffered() == 0 && fin_sent_ ) {
      break;
    }
  }
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Check for RST flag - if set, mark stream as error
  if ( msg.RST ) {
    writer().set_error();
    return;  // Don't process anything else
  }
  
  // Update window size
  recv_window_size_ = msg.window_size;
  
  // Process ackno if present
  if ( msg.ackno.has_value() ) {
    uint64_t abs_ackno = msg.ackno.value().unwrap( isn_, next_abs_seqno_ );
    
    // Ignore invalid acknos (acknowledging something not yet sent)
    if ( abs_ackno > next_abs_seqno_ ) {
      return;
    }
    
    // Check if this acknowledges new data
    bool new_data_acked = abs_ackno > recv_ackno_;
    recv_ackno_ = abs_ackno;
    
    // Remove fully acknowledged segments
    while ( !outstanding_segments_.empty() ) {
      const auto& front_msg = outstanding_segments_.front();
      uint64_t front_abs_seqno = front_msg.seqno.unwrap( isn_, next_abs_seqno_ );
      uint64_t front_end = front_abs_seqno + front_msg.sequence_length();
      
      if ( front_end <= recv_ackno_ ) {
        // This segment is fully acknowledged
        bytes_in_flight_ -= front_msg.sequence_length();
        outstanding_segments_.pop();
      } else {
        break;
      }
    }
    
    // If new data was acknowledged
    if ( new_data_acked ) {
      // Reset RTO to initial value
      current_RTO_ms_ = initial_RTO_ms_;
      
      // Reset consecutive retransmissions
      consecutive_retrans_ = 0;
      
      // Restart timer if there's still outstanding data
      if ( !outstanding_segments_.empty() ) {
        start_timer();
      } else {
        stop_timer();
      }
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Update timer
  timer_ms_ += ms_since_last_tick;
  
  // Check if timer has expired
  if ( timer_running_ && timer_ms_ >= current_RTO_ms_ ) {
    // Timer expired - retransmit earliest outstanding segment
    if ( !outstanding_segments_.empty() ) {
      // Retransmit the first outstanding segment
      transmit( outstanding_segments_.front() );
      
      // If window size is nonzero
      if ( recv_window_size_ > 0 ) {
        // Increment consecutive retransmissions
        consecutive_retrans_++;
        
        // Double RTO (exponential backoff)
        current_RTO_ms_ *= 2;
      }
      
      // Restart timer
      start_timer();
    }
  }
}