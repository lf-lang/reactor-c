/**
 * @file scheduler_api_test.c
 * @author Soroush Bateni (soroush@utdallas.edu)
 * @brief Tests for the scheduler API (see scheduler.h)
 * 
 * @copyright Copyright (c) 2022, The University of Texas at Dallas.
 * 
 * @note: This file contains a rudimentary test of the scheduler API. For more
 * comprehensive testing of a scheduler, the .lf tests on lf-lang/lingua-franca
 * are more appropriate.
 * 
 */

/*************
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

#define NUMBER_OF_WORKERS 1000

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "core/threaded/scheduler.h"
#include "core/platform.h"

int success = 0;

reaction_function_t dummy_reaction_function1() {
    success++;
}

reaction_function_t dummy_reaction_function2() {
    success++;
}

lf_mutex_t mutex;

int main(int argc, char **argv) {
    lf_mutex_init(&mutex);
    // Initialize the scheduler
    lf_sched_init(NUMBER_OF_WORKERS);

    // Create a dummy reaction
    reaction_t dummy_reaction1 = {
        .function = (void*)&dummy_reaction_function1,
        .index = 0x7fffffffffff0000LL,
        .chain_id = 1
    };

    // Create another dummy reaction
    reaction_t dummy_reaction2 = {
        .function = (void*)&dummy_reaction_function2,
        .index = 0x7fffffffffff0001LL,
        .chain_id = 1
    };

    // Feed the dummy reactions to the scheduler
    lf_sched_trigger_reaction(&dummy_reaction1, -1);
    lf_sched_trigger_reaction(&dummy_reaction2, -1);

    // Get it back from the scheduler
    reaction_t* returned_reaction = lf_sched_get_ready_reaction(0);

    if (returned_reaction != &dummy_reaction1) {
        error_print_and_exit("Expected to get reaction %p from the scheduler."
                            "Got %p instead.",
                            &dummy_reaction1,
                            returned_reaction);
    }


    returned_reaction = lf_sched_get_ready_reaction(0);

    if (returned_reaction != &dummy_reaction2) {
        error_print_and_exit("Expected to get reaction %p from the scheduler."
                            "Got %p instead.",
                            &dummy_reaction2,
                            returned_reaction);
    }

    returned_reaction = lf_sched_get_ready_reaction(0);

    if (returned_reaction != NULL) {
        error_print_and_exit("Expected to get NULL from the scheduler."
                            "Got %p instead.",
                            returned_reaction);
    }

    lf_sched_free();

    info_print("SUCCESS");
    return 0;
}
