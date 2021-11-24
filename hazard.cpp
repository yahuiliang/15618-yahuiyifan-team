#include "hazard.h"
#include <vector>
#include <list>
#include <cstddef>
#include <unordered_set>

thread_local size_t thread_id;
std::vector<std::vector<void*>> hp_list(N, std::vector<void*>(K, nullptr));
std::vector<int> hp_offsets(N, 0);
std::vector<std::vector<void*>> rlist(N);
std::vector<std::unordered_set<void*>> plist(N);

void append_hp(void* ptr) {
    if (ptr == nullptr) return;
    int offset = hp_offsets[thread_id];
    hp_list[thread_id][offset] = ptr;
    hp_offsets[thread_id] = (offset + 1) % K;
}

void register_thread(size_t tid) {
    thread_id = tid;
}
