#include "bst.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <climits>
#include <unordered_set>
#include <thread>
#include <chrono>

#define TEST_SIZE 500000
#define RAND_RANGE 1000
#define THREAD_NUM 100
// #define INPUT_PRINT
#define TEST_CORRECTNESS
#define TEST_PARALLEL
#define TEST_ERASE

static FineGrainedBST<int> bst;
static std::mutex mtx;

void test_single_thread() {
    bst.clear();
    bst.register_thread(0);
    std::vector<int> elements(TEST_SIZE);
    size_t n = elements.size();
    for (size_t i = 0; i < n; i++) {
        elements[i] = rand() % RAND_RANGE;
    }
    #ifdef INPUT_PRINT
    printf("inputs: ");
    for (int test : elements) {
        printf("%d,", test);
    }
    printf("\n");
    #endif
    std::unordered_set<int> unique_elements(elements.begin(), elements.end());
    for (int test : elements) {
        bst.insert(test);
    }
    assert(bst.size() == unique_elements.size());
    for (int test : elements) {
        assert(bst.find(test) == true);
    }
    assert(bst.find(INT_MIN) == false);
    assert(bst.find(INT_MAX) == false);
    #ifdef TEST_ERASE
    for (int test : elements) {
        bst.erase(test);
        assert(bst.find(test) == false);
    }
    assert(bst.size() == 0);
    #endif
    printf("test correctness passed\n");
}

void test_multi_thread() {
    bst.clear();
    std::vector<std::thread> threads(THREAD_NUM);
    for (size_t i = 0; i < THREAD_NUM; i++) {
        threads[i] = std::thread([](size_t thread_id) {
            bst.register_thread(thread_id);
            size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
            std::vector<int> elements(local_test_size, 0);
            int start = thread_id * local_test_size;
            int end = std::min(TEST_SIZE, static_cast<int>((thread_id + 1) * local_test_size));
            for (int i = start; i < end; i++) {
                elements[i - start] = i;
            }
            #ifdef INPUT_PRINT
            mtx.lock();
            printf("thread %d inputs: ", static_cast<int>(thread_id));
            for (int test : elements) {
                printf("%d,", test);
            }
            printf("\n");
            mtx.unlock();
            #endif
            for (int test : elements) {
                bst.insert(test);
            }
            for (int test : elements) {
                assert(bst.find(test) == true);
            }
            #ifdef TEST_ERASE
            for (int test : elements) {
                bst.erase(test);
            }
            for (int test : elements) {
                assert(bst.find(test) == false);
            }
            #endif
        }, i);
    }
    for (int i = 0; i < THREAD_NUM; i++) {
        threads[i].join();
    }
    printf("test parallel passed\n");
}

int main() {
    srand(time(NULL));
    auto start = std::chrono::high_resolution_clock::now();
    #ifdef TEST_CORRECTNESS
    test_single_thread();
    #endif
    #ifdef TEST_PARALLEL
    test_multi_thread();
    #endif
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    printf("test finished in %f\n", static_cast<float>(duration.count()) / 1e3);
    return 0;
}
