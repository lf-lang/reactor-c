#include "logging_macros.h"
#include <unistd.h>

#define TOTAL_LOOP_CASES 10

/**
* @brief test for testing LF_TIMESTAMP_PRINT_DEBUG macro
* must be in LOG_LEVEL LOG_LEVEL_DEBUG
*/
int main()
{
    LF_PRINT_DEBUG("Start time: %ld", lf_time_start());
    for(int i; i<TOTAL_LOOP_CASES; i++)
    {
        LF_TIMESTAMP_PRINT_DEBUG("[Test case: %d]testing timed debug prints", i);
        sleep(1);
    }
    
    return 0;
}
