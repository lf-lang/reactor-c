/*************
Copyright (c) 2023, Norwegian University of Science and Technology.

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

/**
 * @brief Zephyr clock support for different boards
 *
 * @author{Erling Jellum <erling.r.jellum@ntnu.no>}
 */

#ifndef LF_ZEPHYR_BOARD_SUPPORT_H
#define LF_ZEPHYR_BOARD_SUPPORT_H


#if defined(BOARD_NRF52DK_NRF52832)
    #define LF_ZEPHYR_CLOCK_HI_RES
    #define LF_TIMER DT_NODELABEL(timer1)
    #define LF_WAKEUP_OVERHEAD_US 100
    #define LF_MIN_SLEEP_US 10
    #define LF_RUNTIME_OVERHEAD_US 19
#else
    #warning Using low-res Kernel timer because hi-res Counter timer is not ported yet for this board.
    #define LF_ZEPHYR_CLOCK_LO_RES
#endif // BOARD



#if defined(LF_ZEPHYR_CLOCK_HI_RES)
    #ifndef LF_WAKEUP_OVERHEAD_US 0
    #define LF_WAKEUP_OVERHEAD_US 0
    #endif

    #ifndef LF_MIN_SLEEP_US 10
    #define LF_MIN_SLEEP_US 10
    #endif
    
    #ifndef LF_RUNTIME_OVERHEAD_US 0
    #define LF_RUNTIME_OVERHEAD_US 0
    #endif
#endif // LF_ZEPHYR_CLOCK_HI_RES

#endif
