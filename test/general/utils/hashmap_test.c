
#include <stdlib.h>
#include <stdio.h>
#include "core/utils/impl/pointer_hashmap.h"
#include "rand_utils.h"
#include "core/utils/util.h"

#define CAPACITY 100
#define N 5000
#define RANDOM_SEED 1830

static int distribution[2] = {50, 50};

static hashmap_object2int_entry_t mock[CAPACITY];
static size_t mock_size = 0;

void test_put(hashmap_object2int_t* h) {
    void* key = NULL;
    while (!key) key = NULL + (rand() % CAPACITY);  // Generate a dummy pointer.
    int value = rand();
    hashmap_object2int_entry_t entry = (hashmap_object2int_entry_t) { .key = key, .value = value };
    hashmap_object2int_put(h, entry.key, entry.value);
    // printf("Putting (%p, %d).\n", entry.key, entry.value);
    mock[mock_size++] = entry;
}

void test_get(hashmap_object2int_t* h) {
    if (!mock_size) return;
    size_t r = rand() % mock_size;
    hashmap_object2int_entry_t desired = mock[r];
    int found = hashmap_object2int_get(h, desired.key);
    // printf("Getting (%p, %d) from %d.\n", desired.key, desired.value, r);
    if (desired.value != found) {
        // It is possible that two distinct values were associated with the same key. Search the
        // "mock" array to check if this is the case.
        for (size_t i = mock_size - 1; i >= 0; i--) {
            if (mock[i].key == desired.key) {
                if (mock[i].value == found) return; // Everything is OK.
                break;
            }
        }
        lf_print_error_and_exit(
            "Expected %d but got %d when getting from a hashmap.\n",
            desired.value,
            found
        );
    }
}

/**
 * @brief Run a randomly selected test on `h`.
 *
 * @param h A hashmap.
 * @param distribution The desired probability distribution with
 * which each of two actions are performed, expressed as percents.
 */
void run_test(hashmap_object2int_t* h, int* distribution) {
    int result = 1;
    int r = rand();
    int choice = (r < 0 ? -r : r) % 100;
    if ((choice = choice - distribution[0]) < 0) {
        test_put(h);
    } else {
        test_get(h);
    }
}

int main() {
    srand(RANDOM_SEED);
    for (int i = 0; i < N; i++) {
        int perturbed[2];
        perturb(distribution, 2, perturbed);
        LF_PRINT_DEBUG(
            "Distribution: %d, %d",
            perturbed[0], perturbed[1]
        );
        hashmap_object2int_t* h = hashmap_object2int_new(CAPACITY, NULL);
        int j = rand() % (CAPACITY / 2);
        while (j--) {
            run_test(h, perturbed);
        }
        hashmap_object2int_free(h);
        mock_size = 0;
    }
    return 0;
}
