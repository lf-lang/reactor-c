/**
 * @file lf_zephyr_board_support.h
 * @brief Provide preprocessor flags for the particular board that was chosen
 *
 * @author Erling Rennemo Jellum
 */

#ifndef LF_ZEPHYR_BOARD_SUPPORT_H
#define LF_ZEPHYR_BOARD_SUPPORT_H

// Default options
#define LF_ZEPHYR_THREAD_PRIORITY_DEFAULT 5
#define LF_ZEPHYR_STACK_SIZE_DEFAULT 2048

#if defined(LF_ZEPHYR_CLOCK_COUNTER)
#if defined(CONFIG_SOC_FAMILY_NRF)
#define LF_TIMER DT_NODELABEL(timer1)
#define LF_WAKEUP_OVERHEAD_US 100
#define LF_MIN_SLEEP_US 10
#define LF_RUNTIME_OVERHEAD_US 19
#elif defined(CONFIG_BOARD_ATSAMD20_XPRO)
#define LF_TIMER DT_NODELABEL(tc4)
#elif defined(CONFIG_SOC_FAMILY_SAM)
#define LF_TIMER DT_NODELABEL(tc0)
#elif defined(CONFIG_COUNTER_MICROCHIP_MCP7940N)
#define LF_TIMER DT_NODELABEL(extrtc0)
#elif defined(CONFIG_COUNTER_RTC0)
#define LF_TIMER DT_NODELABEL(rtc0)
#elif defined(CONFIG_COUNTER_RTC_STM32)
#define LF_TIMER DT_INST(0, st_stm32_rtc)
#elif defined(CONFIG_COUNTER_XLNX_AXI_TIMER)
#define LF_TIMER DT_INST(0, xlnx_xps_timer_1_00_a)
#elif defined(CONFIG_COUNTER_TMR_ESP32)
#define LF_TIMER DT_NODELABEL(timer0)
#elif defined(CONFIG_COUNTER_MCUX_CTIMER)
#define LF_TIMER DT_NODELABEL(ctimer0)
#elif defined(CONFIG_SOC_MIMXRT1176_CM7)
#define LF_TIMER DT_NODELABEL(gpt2)
#else
// This board does not have support for the counter clock. If the user
//  explicitly asked for this cock, then throw an error.
#error "LF_ZEPHYR_CLOCK_COUNTER was requested but it is not supported by the board"
#endif
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
#endif

#endif
