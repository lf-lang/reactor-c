#include <stdlib.h>
#include <stdio.h>
#include "core/utils/vector.h"
#include "../rand_utils.h"

#define CAPACITY 100
#define MAX_PUSHALL 8

static void* mock[CAPACITY];
static size_t mock_size = 0;

static int distribution[4] = {30, 50, 5, 15};

/**
 * @brief Tests the append functionality of `v`.
 * 
 * @param v A vector.
 * @param x Any pointer.
 */
void test_push(vector_t* v) {
    printf("push.\n");
    void* x = mock + rand();
    vector_push(v, x);
    mock[mock_size++] = x;
}

/**
 * @brief Tests the pop functionality of `v`.
 * 
 * @param v A vector.
 */
void test_pop(vector_t* v) {
    printf("pop.\n");
    void* expected;
    void* found;
    if (mock_size && (
        (found = vector_pop(v)) != (expected = mock[--mock_size])
    )) {
        printf(
            "Expected %p but got %p while popping from a vector.\n",
            expected, found
        );
        exit(1);
    }
}

/**
 * @brief Tests the "push all" functionality of `v`.
 * 
 * @param v A vector
 * @return The number of items pushed to `v`.
 */
int test_pushall(vector_t* v) {
    printf("pushall.\n");
    int count = rand() % MAX_PUSHALL;
    void** mock_start = mock + mock_size;
    for (int i = 0; i < count; i++) {
        mock[mock_size++] = mock - rand();
    }
    vector_pushall(v, mock_start, count);
    return count;
}

/**
 * @brief Checks that the result of a random access to `v` yields the
 * correct result.
 * 
 * @param v A vector.
 */
void test_random_access(vector_t* v) {
    if (mock_size) {
        int idx = rand() % mock_size;
        if (v->start[idx] != mock[idx]) {
            printf(
                "Expected %p but got %p while randomly accessing a vector.\n",
                mock[idx], v->start[idx]
            );
            exit(1);
        }
    }
}

/**
 * @brief Checks that voting does not cause an error.
 * 
 * @param v A vector.
 */
void test_vote(vector_t* v) {
    printf("vote.\n");
    vector_vote(v);
}

/**
 * @brief Runs a randomly selected test on `v`.
 * 
 * @param v A vector.
 * @param distribution The desired probability distribution with
 * which each of four actions are performed, expressed as percents.
 * @return An integer that is lower-bounded by 1 and upper-bounded
 * by the number of items added to `v`.
 */
int run_test(vector_t* v, int* distribution) {
    int result = 1;
    int choice = rand() % 100;
    if ((choice = choice - distribution[0]) < 0) {
        test_push(v);
    } else if ((choice = choice - distribution[1]) < 0) {
        test_pop(v);
    } else if ((choice = choice - distribution[2]) < 0) {
        result += test_pushall(v);
    } else {
        test_vote(v);
    }
    test_random_access(v);
    return result;
}

int main() {
    srand(1614);
    for (int i = 0; i < 1000; i++) {
        int perturbed[4];
        perturb(distribution, 4, perturbed);
        printf(
            "Distribution: %d, %d, %d, %d\n",
            perturbed[0], perturbed[1], perturbed[2], perturbed[3]
        );
        // FIXME: Decide whether it should be possible to initialize
        //  vectors with zero capacity.
        vector_t v = vector_new(rand() % CAPACITY + 1);
        mock_size = 0;
        int j = 0;
        while (j < CAPACITY) {
            j += run_test(&v, perturbed);
        }
        vector_free(&v);
    }
    return 0;
}
