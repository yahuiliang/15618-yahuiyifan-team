#include "bst.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <climits>
#include <unordered_set>
#include <thread>
#include <chrono>
#include <getopt.h>
#include <numeric>

#define TEST_SIZE 600000
#define RAND_RANGE 1000
#define THREAD_NUM 100
// #define INPUT_PRINT
#define TEST_CORRECTNESS
#define TEST_PARALLEL
#define TEST_ERASE

enum class State {
    Correctness_Test=0, Load_Test=1, Unknown=2
};

enum class Pattern {
    Insert, Erase, Find, Mixture, One_Branch, Unknown
};

static State state = State::Unknown;
static Pattern pattern = Pattern::Unknown;

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
    for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
        threads[thread_id] = std::thread([](size_t thread_id) {
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
        }, thread_id);
    }
    for (int i = 0; i < THREAD_NUM; i++) {
        threads[i].join();
    }
    printf("test parallel passed\n");
}

void correctness_test() {
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
}

typedef std::chrono::milliseconds time_std;

void load_test() {
    bst.clear();
    std::vector<std::thread> threads(THREAD_NUM);
    std::vector<size_t> exec_times(THREAD_NUM);
    std::vector<int> data(static_cast<size_t>(TEST_SIZE));
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = i;
    }
    switch (pattern) {
        case Pattern::Insert:
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&exec_times](size_t thread_id) {
                    auto start_time = std::chrono::high_resolution_clock::now();
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    int start = thread_id * local_test_size;
                    int end = std::min(TEST_SIZE, static_cast<int>((thread_id + 1) * local_test_size));
                    for (int data = start; data < end; data++) {
                        bst.insert(data);
                    }
                    auto end_time = std::chrono::high_resolution_clock::now();
                    exec_times[thread_id] = std::chrono::duration_cast<time_std>(end_time - start_time).count();
                }, thread_id);
            }
            break;
        case Pattern::Erase:
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&data](size_t thread_id) {
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    size_t start = thread_id * local_test_size;
                    size_t end = std::min(TEST_SIZE, static_cast<int>((thread_id + 1) * local_test_size));
                    for (size_t i = start; i < end; i++) {
                        bst.insert(data[i]);
                    }
                }, thread_id);
            }
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id].join();
            }
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&exec_times, &data](size_t thread_id) {
                    auto start_time = std::chrono::high_resolution_clock::now();
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    int start = thread_id * local_test_size;
                    int end = std::min(TEST_SIZE, static_cast<int>((thread_id + 1) * local_test_size));
                    for (int i = start; i < end; i++) {
                        bst.erase(data[i]);
                    }
                    auto end_time = std::chrono::high_resolution_clock::now();
                    exec_times[thread_id] = std::chrono::duration_cast<time_std>(end_time - start_time).count();
                }, thread_id);
            }
            break;
        case Pattern::Find:
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&data](size_t thread_id) {
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    size_t start = thread_id * local_test_size;
                    size_t end = std::min(TEST_SIZE, static_cast<int>((thread_id + 1) * local_test_size));
                    for (size_t i = start; i < end; i++) {
                        bst.insert(data[i]);
                    }
                }, thread_id);
            }
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id].join();
            }
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&exec_times, &data](size_t thread_id) {
                    auto start_time = std::chrono::high_resolution_clock::now();
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    int start = thread_id * local_test_size;
                    int end = std::min(TEST_SIZE, static_cast<int>((thread_id + 1) * local_test_size));
                    for (int i = start; i < end; i++) {
                        bst.find(data[i]);
                    }
                    auto end_time = std::chrono::high_resolution_clock::now();
                    exec_times[thread_id] = std::chrono::duration_cast<time_std>(end_time - start_time).count();
                }, thread_id);
            }
            break;
        case Pattern::Mixture:
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&exec_times, &data](size_t thread_id) {
                    auto start_time = std::chrono::high_resolution_clock::now();
                    
                    bst.register_thread(thread_id);
                    
                    size_t thread_num = THREAD_NUM / 3.f;
                    size_t insert_id_max = thread_num;
                    size_t erase_id_max = thread_num * 2;
                    size_t find_id_max = THREAD_NUM;
                    if (thread_id < insert_id_max) {
                        size_t insert_thread_num = thread_num;
                        size_t local_test_size = (TEST_SIZE + insert_thread_num - 1) / insert_thread_num;
                        size_t insert_base_id = 0;
                        size_t start = (thread_id - insert_base_id) * local_test_size;
                        size_t end = std::min(static_cast<size_t>(TEST_SIZE), start + local_test_size);
                        for (size_t i = start; i < end; i++) {
                            bst.insert(data[i]);
                        }
                    } else if (thread_id < erase_id_max) {
                        size_t erase_thread_num = thread_num;
                        size_t local_test_size = (TEST_SIZE + erase_thread_num - 1) / erase_thread_num;
                        size_t erase_base_id = insert_id_max;
                        size_t start = (thread_id - erase_base_id) * local_test_size;
                        size_t end = std::min(static_cast<size_t>(TEST_SIZE), start + local_test_size);
                        for (size_t i = start; i < end; i++) {
                            bst.erase(data[i]);
                        }
                    } else {
                        size_t find_thread_num = find_id_max - erase_id_max;
                        size_t local_test_size = (TEST_SIZE + find_thread_num - 1) / find_thread_num;
                        size_t find_base_id = erase_id_max;
                        size_t start = (thread_id - find_base_id) * local_test_size;
                        size_t end = std::min(static_cast<size_t>(TEST_SIZE), start + local_test_size);
                        for (size_t i = start; i < end; i++) {
                            bst.find(data[i]);
                        }
                    }
                    auto end_time = std::chrono::high_resolution_clock::now();
                    exec_times[thread_id] = std::chrono::duration_cast<time_std>(end_time - start_time).count();
                }, thread_id);
            }
            break;
        // Some issues exist in the code below...
        // case Pattern::One_Branch:
        //     for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
        //         threads[thread_id] = std::thread([&exec_times, &data](size_t thread_id) {
        //             auto start_time = std::chrono::high_resolution_clock::now();
        //             bst.register_thread(thread_id);
        //             for (size_t i = thread_id; i < TEST_SIZE; i+=THREAD_NUM) {
        //                 bst.insert(data[i]);
        //             }
        //             auto end_time = std::chrono::high_resolution_clock::now();
        //             exec_times[thread_id] = std::chrono::duration_cast<time_std>(end_time - start_time).count();
        //         }, thread_id);
        //     }
        //     break;
        default:
            printf("Unknown pattern\n");
            printf("Available patterns:\n");
            printf("0=Insert, 1=Erase, 2=Find, 3=Mixture, 4=One_Branch\n");
            return;
    }
    for (size_t thread_id = 0; thread_id < threads.size(); thread_id++) {
        threads[thread_id].join();
    }
    size_t avg = std::accumulate(exec_times.begin(), exec_times.end(), 0ul) / THREAD_NUM;
    printf("load test %fs\n", static_cast<float>(avg) / static_cast<float>(1e3));
}

int main(int argc, char **argv) {
    srand(time(NULL));
    int opt;
    std::string tmp;
    while ((opt = getopt(argc, argv, "p:th")) != -1) {
        switch (opt) {
            case 't':
                state = State::Correctness_Test;
                break;
            case 'p':
                tmp = std::string(optarg);
                for (char c : tmp) {
                    if (!isdigit(c)) {
                        printf("Unknown pattern\n");
                        printf("Available patterns:\n");
                        printf("0=Insert, 1=Erase, 2=Find, 3=Mixture, 4=One_Branch\n");
                        return 0;
                    }
                }
                state = State::Load_Test;
                pattern = static_cast<Pattern>(std::stoi(tmp));
                break;
            case 'h':
                printf("-t: run correctness tests\n");
                printf("-p: run pattern generator\n");
                printf("-h help\n");
                return 0;
        }
    }
    switch (state) {
        case State::Correctness_Test:
            correctness_test();
            break;
        case State::Load_Test:
            load_test();
            break;
        default:
            printf("Unknown state\n");
            printf("Available states:\n");
            printf("0=Correctness_Test, 1=Load_Test\n"); 
            break;
    }
    
    return 0;
}
