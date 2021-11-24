#ifndef HAZARD_H
#define HAZARD_H

#include <cstddef>
#include <vector>
#include <unordered_set>

#define N 100
#define R 100
#define K 15

extern thread_local size_t thread_id;
extern std::vector<std::vector<void*>> hp_list;
extern std::vector<int> hp_offsets;
extern std::vector<std::vector<void*>> rlist;
extern std::vector<std::unordered_set<void*>> plist;

void append_hp(void* ptr);
void register_thread(size_t tid);

template<typename T>
void scan() {
    for (int tid = 0; tid < N; tid++) {
        for (void* ptr : hp_list[tid]) {
            if (ptr != nullptr) {
                plist[thread_id].insert(ptr);
            }
        }
    }
    std::vector<void*> tmp_rlist = rlist[thread_id];
    rlist[thread_id].clear();
    for (void* ptr : tmp_rlist) {
        if (plist[thread_id].find(ptr) != plist[thread_id].end()) {
            rlist[thread_id].push_back(ptr);
        } else {
            delete static_cast<T*>(ptr);
        }
    }
    plist[thread_id].clear();
}

template<typename T>
void retire(void* ptr) {
    rlist[thread_id].push_back(ptr);
    if (rlist[thread_id].size() > R) {
        scan<T>();
    }
}

#endif