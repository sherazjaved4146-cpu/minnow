#include "reassembler.hh"
#include "debug.hh"

#include <vector>
#include <algorithm>

using namespace std;

void Reassembler::insert(uint64_t first_idx, string data, bool is_last) {
    // Handle empty substring
    if (data.empty()) {
        if (is_last)
            output_.writer().close();
        return;
    }

    uint64_t stream_begin = output_.writer().bytes_pushed();
    uint64_t stream_limit = stream_begin + output_.writer().available_capacity();

    uint64_t seg_begin = first_idx;
    uint64_t seg_end = first_idx + data.size();

    // Completely outside window
    if (seg_begin >= stream_limit)
        return;

    // Remember the EOF position if this is the final substring
    if (is_last) {
        eof_flag = true;
        eof_index = seg_end;
    }

    // Clip the segment to valid stream range
    uint64_t clipped_start = max(seg_begin, stream_begin);
    uint64_t clipped_end = min(seg_end, stream_limit);
    if (clipped_end <= clipped_start)
        return;

    string valid_part = data.substr(clipped_start - seg_begin, clipped_end - clipped_start);

    // Case 1: fits right at current write position → push directly
    if (clipped_start == stream_begin) {
        output_.writer().push(valid_part);

        // Now see if stored chunks can be written after this
        while (!internal_storage.empty()) {
            auto it = internal_storage.begin();
            uint64_t stored_start = it->first;
            string &stored_data = it->second;

            if (stored_start <= output_.writer().bytes_pushed()) {
                uint64_t offset = output_.writer().bytes_pushed() - stored_start;
                if (offset < stored_data.size()) {
                    output_.writer().push(stored_data.substr(offset));
                }
                internal_storage.erase(it);
            } else {
                break;
            }
        }
    }
    // Case 2: doesn't align, must buffer and possibly merge
    else {
        uint64_t merged_start = clipped_start;
        uint64_t merged_end = clipped_end;
        string merged_data = valid_part;

        // Merge with any overlapping stored segments
        auto it = internal_storage.lower_bound(merged_start);
        if (it != internal_storage.begin()) {
            --it;
            if (it->first + it->second.size() < merged_start)
                ++it;
        }

        while (it != internal_storage.end() && it->first <= merged_end) {
            uint64_t exist_start = it->first;
            uint64_t exist_end = exist_start + it->second.size();

            // Extend to left
            if (exist_start < merged_start) {
                merged_data = it->second.substr(0, merged_start - exist_start) + merged_data;
                merged_start = exist_start;
            }
            // Extend to right
            if (exist_end > merged_end) {
                merged_data += it->second.substr(merged_end - exist_start);
                merged_end = exist_end;
            }
            it = internal_storage.erase(it);
        }

        internal_storage[merged_start] = merged_data;
    }

    // If EOF reached and everything written
    if (eof_flag && output_.writer().bytes_pushed() == eof_index) {
        output_.writer().close();
    }
}

uint64_t Reassembler::count_bytes_pending() const {
    uint64_t count = 0;
    for (const auto &seg : internal_storage) {
        count += seg.second.size();
    }
    return count;
}
