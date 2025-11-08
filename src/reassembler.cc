#include "reassembler.hh"
#include<vector>
#include "debug.hh"

using namespace std;

void Reassembler::insert(uint64_t first_index, string data, bool is_last_substring) {

  // khali string
  if (data.empty()) {
    if (is_last_substring) // empty bhi last ho sakti - tests fail kr k pata chala
      output_.writer().close();
    return;
  }

  uint64_t stream_start = output_.writer().bytes_pushed();
  uint64_t stream_end   = stream_start + output_.writer().available_capacity();
  uint64_t sub_start    = first_index;
  uint64_t sub_end      = first_index + data.size();

  // pura stream ke bahar hai to discard
  if (sub_start >= stream_end)  
    return;

  // eof flag set karo
  if (is_last_substring) {
    eof_flag = true;
    eof_index = sub_end;
  }

  // cutting shutting saari kr do k actual kitna data push hona
  uint64_t new_start = max(sub_start, stream_start);
  uint64_t new_end   = min(sub_end, stream_end);

  if (new_end <= new_start) 
    return;

  string useful = data.substr(new_start - sub_start, new_end - new_start);

  // agar start stream ke current se match ho raha to push
  if (new_start == stream_start) {
    output_.writer().push(useful);

    // ab reassembler ki storage waali strings ko push karo lekin condition bases pr
    while (!internal_storage.empty()) {
      auto it = internal_storage.begin();
      uint64_t seg_start = it->first;
      string &seg_data   = it->second;

      if (seg_start <= output_.writer().bytes_pushed()) {
        uint64_t overlap = output_.writer().bytes_pushed() - seg_start;
        if (overlap < seg_data.size()) {
          output_.writer().push(seg_data.substr(overlap));
        }
        internal_storage.erase(it);
      } else break;
    }
  }

  // store on reassembler storage and agr gap hai to store with full merge - test case failure se pata chala
  else {
    uint64_t seg_start = new_start;
    uint64_t seg_end   = new_end;
    string seg_data    = useful;

    // puraAne overlaps ke saath merge
    auto it = internal_storage.lower_bound(seg_start);
    if (it != internal_storage.begin()) {
      --it;
      if (it->first + it->second.size() < seg_start) ++it;
    }

    while (it != internal_storage.end() && it->first <= seg_end) {
      uint64_t exist_start = it->first;
      uint64_t exist_end   = exist_start + it->second.size();

      if (exist_start < seg_start) {
        seg_data = it->second.substr(0, seg_start - exist_start) + seg_data;
        seg_start = exist_start;
      }

      if (exist_end > seg_end) {
        seg_data += it->second.substr(seg_end - exist_start);
        seg_end = exist_end;
      }
      it = internal_storage.erase(it);
    }

    internal_storage[seg_start] = seg_data;
  }

  if (eof_flag && output_.writer().bytes_pushed() == eof_index) {
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t total_bytes = 0;

  for (const auto& [index, seg] : internal_storage) {
    total_bytes += seg.size();
  }

  return total_bytes;
}