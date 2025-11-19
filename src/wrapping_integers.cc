#include "wrapping_integers.hh"
#include "debug.hh"
using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { zero_point.raw_value_ + static_cast<uint32_t>(n) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint32_t offset = raw_value_ - zero_point.raw_value_;
  uint64_t base = checkpoint - (checkpoint % (1ULL << 32));
  uint64_t candidate = base + offset;
  
  uint64_t dist_candidate;
  if (checkpoint > candidate) {
    dist_candidate = checkpoint - candidate;
  } else {
    dist_candidate = candidate - checkpoint;
  }
  
  uint64_t result = candidate;
  uint64_t min_dist = dist_candidate;
  
  if (candidate >= (1ULL << 32)) {
    uint64_t lower = candidate - (1ULL << 32);
    uint64_t dist_lower;
    if (checkpoint > lower) {
      dist_lower = checkpoint - lower;
    } else {
      dist_lower = lower - checkpoint;
    }
    
    if (dist_lower < min_dist) {
      result = lower;
      min_dist = dist_lower;
    }
  }
  
  uint64_t upper = candidate + (1ULL << 32);
  uint64_t dist_upper;
  if (checkpoint > upper) {
    dist_upper = checkpoint - upper;
  } else {
    dist_upper = upper - checkpoint;
  }
  
  if (dist_upper < min_dist) {
    result = upper;
  }
  
  return result;
}