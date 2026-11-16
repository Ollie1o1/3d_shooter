#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

// Dijkstra only inserts distances beyond the item just removed. A radix
// frontier makes those operations sequential instead of repeatedly sifting a
// binary heap. Positive IEEE float bits preserve distance ordering; the low
// word preserves the old heap's node-id tie break exactly. New distances must
// increase (navigation edges have a positive cost), and node ids are >= 0.
class NavQueue {
    std::array<std::vector<uint64_t>,65> buckets_;
    uint64_t last_=0;
    size_t count_=0;
    static unsigned bucket(uint64_t x){return x?64u-unsigned(__builtin_clzll(x)):0u;}
public:
    bool empty() const {return count_==0;}
    void push(float distance,int node){
        uint32_t bits;std::memcpy(&bits,&distance,4);
        const uint64_t key=(uint64_t(bits)<<32)|uint32_t(node);
        buckets_[bucket(key^last_)].push_back(key);++count_;
    }
    std::pair<float,int> pop(){
        if(buckets_[0].empty()){
            unsigned i=1;while(buckets_[i].empty())++i;
            auto& from=buckets_[i];last_=*std::min_element(from.begin(),from.end());
            for(uint64_t key:from)buckets_[bucket(key^last_)].push_back(key);
            from.clear();
        }
        const uint64_t key=buckets_[0].back();buckets_[0].pop_back();--count_;
        const uint32_t bits=uint32_t(key>>32);float distance;std::memcpy(&distance,&bits,4);
        return {distance,int(uint32_t(key))};
    }
};
