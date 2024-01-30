#if defined(PLATFORM_Linux) || defined(PLATFORM_Darwin)
/**
 * @file lf_unix_syscall_support.c
 * @author Soroush Bateni (soroush@utdallas.edu)
 * @brief Platform support for syscalls in Unix-like systems.
 * @version 0.1
 * @date 2022-03-09
 * 
 * @copyright Copyright (c) 2022 The University of Texas at Dallas
 * 
 */

#include <unistd.h>

int lf_available_cores() {
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
}
#endif
