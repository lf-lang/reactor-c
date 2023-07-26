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


#if defined(CONFIG_SOC_FAMILY_NRF)
    #define LF_ZEPHYR_CLOCK_HI_RES
    #define LF_TIMER DT_NODELABEL(timer1)
    #define LF_WAKEUP_OVERHEAD_US 100
    #define LF_MIN_SLEEP_US 10
    #define LF_RUNTIME_OVERHEAD_US 19
#elif defined(CONFIG_BOARD_ATSAMD20_XPRO)
    #define LF_TIMER DT_NODELABEL(tc4)
    #define LF_ZEPHYR_CLOCK_HI_RES
#elif defined(CONFIG_SOC_FAMILY_SAM)
    #define LF_TIMER DT_NODELABEL(tc0)
    #define LF_ZEPHYR_CLOCK_HI_RES
#elif defined(CONFIG_COUNTER_MICROCHIP_MCP7940N)
    #define LF_ZEPHYR_CLOCK_HI_RES
    #define LF_TIMER DT_NODELABEL(extrtc0)
#elif defined(CONFIG_COUNTER_RTC0)
    #define LF_ZEPHYR_CLOCK_HI_RES
    #define LF_TIMER DT_NODELABEL(rtc0)
#elif defined(CONFIG_COUNTER_RTC_STM32)
    #define LF_TIMER DT_INST(0, st_stm32_rtc)
    #define LF_ZEPHYR_CLOCK_HI_RES
#elif defined(CONFIG_COUNTER_NATIVE_POSIX)
    #define LF_ZEPHYR_CLOCK_LO_RES
#elif defined(CONFIG_COUNTER_XLNX_AXI_TIMER)
    #define LF_TIMER DT_INST(0, xlnx_xps_timer_1_00_a)
    #define LF_ZEPHYR_CLOCK_HI_RES
#elif defined(CONFIG_COUNTER_TMR_ESP32)
    #define LF_TIMER DT_NODELABEL(timer0)
    #define LF_ZEPHYR_CLOCK_HI_RES
#elif defined(CONFIG_COUNTER_MCUX_CTIMER)
    #define LF_TIMER DT_NODELABEL(ctimer0)
    #define LF_ZEPHYR_CLOCK_HI_RES
#elif defined(CONFIG_SOC_MIMXRT1176_CM7)
    #define LF_TIMER DT_NODELABEL(gpt2)
    #define LF_ZEPHYR_CLOCK_HI_RES
#else
    #warning Using low-res Kernel timer because hi-res Counter timer is not ported yet for this board.
    #define LF_ZEPHYR_CLOCK_LO_RES
#endif // BOARD

#define FREQ_16MHZ 16000000



#if defined(LF_ZEPHYR_CLOCK_HI_RES)
    #ifndef LF_WAKEUP_OVERHEAD_US 
    #define LF_WAKEUP_OVERHEAD_US 0
    #endif

    #ifndef LF_MIN_SLEEP_US
    #define LF_MIN_SLEEP_US 10
    #endif
    
    #ifndef LF_RUNTIME_OVERHEAD_US
    #define LF_RUNTIME_OVERHEAD_US 0
    #endif
    
    #ifndef LF_TIMER_ALARM_CHANNEL
    #define LF_TIMER_ALARM_CHANNEL 0
    #endif
#else
    #if !defined(LF_ZEPHYR_CLOCK_LO_RES)
        #error Neither hi-res nor lo-res clock specified
    #endif
#endif // LF_ZEPHYR_CLOCK_HI_RES

#endif
