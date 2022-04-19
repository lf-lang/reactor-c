/* The generated schedule */
#include <stdio.h>
#include <stdint.h>

const uint32_t s1_w1[3] = { 0x00000001, 0x00000002, 0x00000003 };
const uint32_t s1_w2[5] = { 0x00000004, 0x00000005, 0x00000006, 0x00000007, 0x00000008 };
const uint32_t s1_w3[1] = { 0x00000009 };
const uint32_t s2_w1[3] = { 0x00000001, 0x00000002, 0x00000003 };
const uint32_t s2_w2[5] = { 0x00000004, 0x00000005, 0x00000006, 0x00000007, 0x00000008 };
const uint32_t s2_w3[1] = { 0x00000009 };

const uint32_t** s1[3] = { s1_w1, s1_w2, s1_w3 };
const uint32_t** s2[3] = { s2_w1, s2_w2, s2_w3 };

const uint32_t lengths[2][3] = {{3, 5, 1 }, {3, 5, 1}};
const uint32_t*** schedules[2] = { s1, s2 };

int main(int argc, char *argv[]) {
    int schedule_index = 1;
    const uint32_t** current_schedule = schedules[schedule_index];
    for (int w = 0; w < 3; w++) {
        printf("Worker %d's schedule:\n", w);
        for (int i = 0; i < lengths[schedule_index][w]; i++) {
            printf("%x\n", current_schedule[w][i]);
        }
    }
}