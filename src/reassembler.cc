#include "reassembler.hh"
#include "debug.hh"
#include <vector>
#include <algorithm>

using namespace std;


void Reassembler::insert(uint64_t first_idx, string data, bool is_last) {
    uint64_t stream_begin = output_.writer().bytes_pushed();
    uint64_t stream_limit = stream_begin + output_.writer().available_capacity();
    
   
    if (is_last) {
        eof_seen = true;
        eof_position = first_idx + data.size();
    }

    if (data.empty()) {
        
        if (eof_seen && stream_begin == eof_position) {
            output_.writer().close();
        }
        return;
    }

    uint64_t seg_begin = first_idx;
    uint64_t seg_end = first_idx + data.size();

    
    uint64_t clipped_start = max(seg_begin, stream_begin);
    uint64_t clipped_end = min(seg_end, stream_limit);

    if (clipped_end <= clipped_start) {
     
        return;
    }

 
    string valid_part = data.substr(clipped_start - seg_begin, clipped_end - clipped_start);
    
    
    
    uint64_t merged_start = clipped_start;
    uint64_t merged_end = clipped_end;
    string merged_data = valid_part;

 
    auto it = buffered_segments.lower_bound(merged_start);
    
    
    if (it != buffered_segments.begin()) {
        auto prev = it;
        --prev;
        if (prev->first + prev->second.size() > merged_start) {
      
            it = prev;
        } else {
          
            it = prev;
            ++it;
        }
    }
    
    
    while (it != buffered_segments.end() && it->first <= merged_end) {
        uint64_t exist_start = it->first;
        uint64_t exist_end = exist_start + it->second.size();

     
        if (exist_start < merged_start) {
            
            merged_data = it->second.substr(0, merged_start - exist_start) + merged_data;
            merged_start = exist_start;
        }

        if (exist_end > merged_end) {
           
            merged_data += it->second.substr(merged_end - exist_start);
            merged_end = exist_end;
        }

       
        it = buffered_segments.erase(it);
    }

    if (!merged_data.empty()) {
        buffered_segments[merged_start] = merged_data;
    }

    if (!buffered_segments.empty()) {
        auto first_seg = buffered_segments.begin();
        if (first_seg->first == output_.writer().bytes_pushed()) {
            
            output_.writer().push(first_seg->second);
            buffered_segments.erase(first_seg);
        }
    }

    if (eof_seen && output_.writer().bytes_pushed() >= eof_position) {
        output_.writer().close();
    }
}

uint64_t Reassembler::count_bytes_pending() const {
    uint64_t count = 0;
    for (const auto &seg : buffered_segments) {
        count += seg.second.size();
    }
    return count;
}