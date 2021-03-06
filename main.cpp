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

/********************************
 * Macros for testing correctness
 ********************************/
#define RAND_RANGE 1000
// #define INPUT_PRINT
//#define TEST_CORRECTNESS
#define TEST_PARALLEL
#define TEST_ERASE

enum class State {
    Correctness_Test=0, Load_Test=1, Unknown=2
};

enum class Pattern {
    Insert, Erase, Find, Contention, Write_dominance, Mixed, Read_dominance, Unknown
};

static State state = State::Unknown;
static Pattern pattern = Pattern::Unknown;

static BST<int>* bst_ptrs[3];
static size_t bst_selection = 0;
static std::mutex mtx;
static size_t TEST_SIZE = 10000;
static size_t THREAD_NUM = 2;

void init_bsts() {
    bst_ptrs[0] = new CoarseGrainedBST<int>();
    bst_ptrs[1] = new FineGrainedBST<int>();
    bst_ptrs[2] = new LockFreeBST<int>();
}

void free_bsts() {
    delete bst_ptrs[0];
    delete bst_ptrs[1];
    delete bst_ptrs[2];
}

/**
 * Test insert, erase, and find on the single thread
 */
void test_single_thread(BST<int>& bst) {
    // bst.clear();
    bst.set_N(1);
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

/**
 * Test operations under multi-thread
 */
void test_multi_thread(BST<int>& bst) {
    // bst.clear();
    bst.set_N(THREAD_NUM);
    std::vector<std::thread> threads(THREAD_NUM);
    for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
        threads[thread_id] = std::thread([&bst](size_t thread_id) {
            bst.register_thread(thread_id);
            size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
            std::vector<int> elements(local_test_size, 0);
            size_t start = thread_id * local_test_size;
            size_t end = std::min(TEST_SIZE, (thread_id + 1) * local_test_size);
            for (size_t i = start; i < end; i++) {
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
    for (size_t i = 0; i < THREAD_NUM; i++) {
        threads[i].join();
    }
    printf("test parallel passed\n");
}

void correctness_test(BST<int>& bst) {
    auto start = std::chrono::high_resolution_clock::now();
    #ifdef TEST_CORRECTNESS
    test_single_thread(bst);
    #endif
    #ifdef TEST_PARALLEL
    test_multi_thread(bst);
    #endif
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    printf("test finished in %f\n", static_cast<float>(duration.count()) / 1e3);
}

typedef std::chrono::microseconds time_std;

void load_test(BST<int>& bst) {
    // bst.clear();
    bst.set_N(THREAD_NUM);
    std::vector<std::thread> threads(THREAD_NUM);
    std::vector<int> data(TEST_SIZE);
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = i;
    }
    std::chrono::high_resolution_clock::time_point start_time;
    switch (pattern) {
        case Pattern::Insert:
            // Insert only
            start_time = std::chrono::high_resolution_clock::now();
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&bst](size_t thread_id) {
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    size_t start = thread_id * local_test_size;
                    size_t end = std::min(TEST_SIZE, (thread_id + 1) * local_test_size);
                    for (size_t data = start; data < end; data++) {
                        bst.insert(static_cast<int>(data));
                    }
                }, thread_id);
            }
            break;
        case Pattern::Erase:
            // Erase only
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&bst, &data](size_t thread_id) {
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    size_t start = thread_id * local_test_size;
                    size_t end = std::min(TEST_SIZE, (thread_id + 1) * local_test_size);
                    for (size_t i = start; i < end; i++) {
                        bst.insert(data[i]);
                    }
                }, thread_id);
            }
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id].join();
            }
            start_time = std::chrono::high_resolution_clock::now();
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&bst, &data](size_t thread_id) {
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    size_t start = thread_id * local_test_size;
                    size_t end = std::min(TEST_SIZE, (thread_id + 1) * local_test_size);
                    for (size_t i = start; i < end; i++) {
                        bst.erase(data[i]);
                    }
                }, thread_id);
            }
            break;
        case Pattern::Find:
            // Find only 
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&bst, &data](size_t thread_id) {
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    size_t start = thread_id * local_test_size;
                    size_t end = std::min(TEST_SIZE, (thread_id + 1) * local_test_size);
                    for (size_t i = start; i < end; i++) {
                        bst.insert(data[i]);
                    }
                }, thread_id);
            }
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id].join();
            }
            start_time = std::chrono::high_resolution_clock::now();
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&bst, &data](size_t thread_id) {
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    size_t start = thread_id * local_test_size;
                    size_t end = std::min(TEST_SIZE, (thread_id + 1) * local_test_size);
                    for (size_t i = start; i < end; i++) {
                        bst.find(data[i]);
                    }
                }, thread_id);
            }
            break;
        case Pattern::Contention:
            // Read/Write on same data
            start_time = std::chrono::high_resolution_clock::now();
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&bst, &data](size_t thread_id) {
                    if (THREAD_NUM < 3) {
                        return;
                    }
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
                }, thread_id);
            }
            break;
        case Pattern::Write_dominance:
            // 50% insert, 50% erase
            start_time = std::chrono::high_resolution_clock::now();
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&bst](size_t thread_id) {
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    size_t start = thread_id * local_test_size;
                    size_t end = std::min(TEST_SIZE, (thread_id + 1) * local_test_size);
                    for (size_t data = start; data < end; data++) {
                        bst.insert(static_cast<int>(data));
                    }
                    for (size_t data = start; data < end; data++) {
                        bst.erase(static_cast<int>(data));
                    }
                }, thread_id);
            }
            break;
        case Pattern::Mixed:
            // 20% insert, 20% delete, 60% find
            start_time = std::chrono::high_resolution_clock::now();
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&bst, &data](size_t thread_id) {
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    size_t start = thread_id * local_test_size;
                    size_t end = std::min(TEST_SIZE, (thread_id + 1) * local_test_size);
                    for (size_t i = start; i < end; i++) {
                        bst.insert(data[i]);
                    }
                    for (int search_times = 0; search_times < 3; search_times++) {
                        for (size_t i = start; i < end; i++) {
                            bst.find(data[i]);
                        }
                    }
                    for (size_t i = start; i < end; i++) {
                        bst.erase(data[i]);
                    }
                }, thread_id);
            }
            break;
        case Pattern::Read_dominance:
            // 10% insert, 90% find
            start_time = std::chrono::high_resolution_clock::now();
            for (size_t thread_id = 0; thread_id < THREAD_NUM; thread_id++) {
                threads[thread_id] = std::thread([&bst, &data](size_t thread_id) {
                    bst.register_thread(thread_id);
                    size_t local_test_size = (TEST_SIZE + THREAD_NUM - 1) / THREAD_NUM;
                    size_t start = thread_id * local_test_size;
                    size_t end = std::min(TEST_SIZE, (thread_id + 1) * local_test_size);
                    for (size_t i = start; i < end; i++) {
                        bst.insert(data[i]);
                    }
                    for (int search_times = 0; search_times < 9; search_times++) {
                        for (size_t i = start; i < end; i++) {
                            bst.find(data[i]);
                        }
                    }
                }, thread_id);
            }
            break;
        default:
            printf("Unknown pattern\n");
            printf("Available patterns:\n");
            printf("0=Insert, 1=Erase, 2=Find, 3=Contention, 4=Write_dominance, 5=Mixed, 6=Read_dominance\n");
            return;
    }
    for (size_t thread_id = 0; thread_id < threads.size(); thread_id++) {
        threads[thread_id].join();
    }
    std::chrono::high_resolution_clock::time_point end_time = std::chrono::high_resolution_clock::now();
    size_t time_count = std::chrono::duration_cast<time_std>(end_time - start_time).count();
    // printf("load test %fs\n", static_cast<float>(avg) / static_cast<float>(1e6));
    // printf("load test %fms\n", static_cast<float>(avg));
    printf("%f\n", static_cast<float>(time_count) / static_cast<float>(1e6));

}

void print_test_status() {
    printf("testing with n=%lu d=%lu\n", THREAD_NUM, TEST_SIZE);
}

/**
 * Pattern generator
 */
int main(int argc, char **argv) {
    srand(time(NULL));
    int opt;
    std::string tmp;
    while ((opt = getopt(argc, argv, "p:thn:d:a:")) != -1) {
        switch (opt) {
            case 't':
                state = State::Correctness_Test;
                break;
            case 'a':
                tmp = std::string(optarg);
                for (char c : tmp) {
                    if (!isdigit(c)) {
                        printf("Unknown algorithm\n");
                        printf("Availabe algorihtms:\n");
                        printf("0=CoarseGrained 1=FineGrained 2=LockFree");
                        return 0;
                    }
                }
                bst_selection = stoul(tmp);
                if (bst_selection >= (sizeof(bst_ptrs) / sizeof(BST<int>*))) {
                    printf("Unknown algorithm\n");
                    printf("Availabe algorihtms:\n");
                    printf("0=CoarseGrained 1=FineGrained 2=LockFree\n");
                    return 0;
                }
                break;
            case 'p':
                tmp = std::string(optarg);
                for (char c : tmp) {
                    if (!isdigit(c)) {
                        printf("Unknown pattern\n");
                        printf("Available patterns:\n");
                        printf("0=Insert, 1=Erase, 2=Find, 3=Contention, 4=Write_dominance, 5=Mixed, 6=Read_dominance\n");
                        return 0;
                    }
                }
                state = State::Load_Test;
                pattern = static_cast<Pattern>(std::stoi(tmp));
                break;
            case 'n':
                // thread number
                tmp = std::string(optarg);
                for (char c : tmp) {
                    if (!isdigit(c)) {
                        printf("thread number should be a number\n");
                        return 0;
                    } 
                }
                THREAD_NUM = stoi(tmp);
                break;
            case 'd':
                // test data size
                tmp = std::string(optarg);
                for (char c : tmp) {
                    if (!isdigit(c)) {
                        printf("test data size should be a number\n");
                        return 0;
                    }
                }
                TEST_SIZE = stoi(tmp);
                break;
            default:
                printf("-a: algorithm, availabe trees: 0=CoarseGrained 1=FineGrained 2=LockFree\n");
                printf("-t: run correctness tests\n");
                printf("-p: run pattern generator, available parameters: 0=Insert, 1=Erase, 2=Find, 3=Contention, 4=Write_dominance, 5=Mixed, 6=Read_dominance\n");
                printf("-n: thread num\n");
                printf("-d: data size\n");
                printf("-h help\n");
                return 0;
        }
    }
    init_bsts();
    switch (state) {
        case State::Correctness_Test:
            // print_test_status(); 
            correctness_test(*bst_ptrs[bst_selection]);
            break;
        case State::Load_Test:
            // print_test_status();
            load_test(*bst_ptrs[bst_selection]);
            break;
        default:
            printf("Unknown state\n");
            printf("-t Correctness_Test\n");
            printf("-p Load_Test\n");
            break;
    }
    free_bsts();
    return 0;
}
