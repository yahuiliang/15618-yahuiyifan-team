#include "bst.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <climits>
#include <unordered_set>

#define TEST_SIZE 1000
#define RAND_RANGE 1000
// #define INPUT_PRINT

int main() {
    srand(time(NULL));
    CoarseGrainedBST<int> bst;
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
    for (int test : elements) {
        bst.erase(test);
        assert(bst.find(test) == false);
    }
    assert(bst.size() == 0);
    printf("test passed\n");
    return 0;
}