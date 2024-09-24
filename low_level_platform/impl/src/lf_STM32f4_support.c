#if defined(PLATFORM_STM32F4)

#include "platform/lf_STM32f4_support.h"
#include "low_level_platform.h"
#include "tag.h"

//  + -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- +
//  | Important defines and global variables
//  + -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- +

static volatile bool _lf_sleep_interrupted = false;
static volatile bool _lf_async_event = false;

// nested critical section counter
static uint32_t _lf_num_nested_crit_sec = 0;

// Timer upper half (for overflow)
static uint32_t _lf_time_us_high = 0;

#define LF_MIN_SLEEP_NS 10

// Combine 2 32bit works to a 64 bit word (Takes from nrf52 support)
#define COMBINE_HI_LO(hi, lo) ((((uint64_t)hi) << 32) | ((uint64_t)lo))

void lf_SystemClock_Config();
void Error_Handler();

TIM_HandleTypeDef htim5;

//  + -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- +
//  | Code for timer functions
//  + -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- +
// We use timer 5 for our clock (probably better than dealing with sysTick)
void _lf_initialize_clock(void) {
  // Standard initializations from generated code
  HAL_Init();
  lf_SystemClock_Config();

  // Configure TIM5 as our 32-bit clock timer
  __HAL_RCC_TIM5_CLK_ENABLE(); // initialize counter
  TIM5->CR1 = TIM_CR1_CEN;     // enable counter

  // set prescaler to (16 - 1) = 15
  // CPU runs a 16MHz so timer ticks at 1MHz
  //      Which means period of 1 microsecond
  TIM5->PSC = 15;

  // Setup ISR to increment upper bits
  TIM5->DIER |= TIM_DIER_CC1IE;
  NVIC_EnableIRQ(TIM5_IRQn);

  /* This is to make the Prescaler actually work
   *  For some reason, the Prescaler only kicks in once the timer has reset once.
   *  Thus, the current solution is to manually ret the value to a really large
   *  and force it to reset once. Its really jank but its the only way I could
   *  find to make it work
   */
  TIM5->CNT = 0xFFFFFFFE;
}

// ISR for handling timer overflow -> We increment the upper timer
void TIM5_IRQHandler(void) {
  if (TIM5->SR & TIM_FLAG_CC1) {
    TIM5->SR &= ~TIM_FLAG_CC1;
    _lf_time_us_high += 1;
  }
}

/**
 * Write the time since boot into time variable
 */
int _lf_clock_gettime(instant_t* t) {
  // Timer is cooked
  if (!t) {
    return 1;
  }

  // Get the current microseconds from TIM5
  uint32_t now_us_hi_pre = _lf_time_us_high;
  uint32_t now_us_low = TIM5->CNT;
  uint32_t now_us_hi_post = _lf_time_us_high;

  // Check if we read the time during a wrap
  if (now_us_hi_pre != now_us_hi_post) {
    // There was a wrap. read again and return
    now_us_low = TIM5->CNT;
  }

  // Combine upper and lower timers
  uint64_t now_us = COMBINE_HI_LO((now_us_hi_post - 1), now_us_low);
  *t = ((instant_t)now_us) * 1000;
  return 0;
}

/**
 * Make the STM32 sleep for set nanoseconds
 * Based on the lf_nrf52 support implementation
 */
int lf_sleep(interval_t sleep_duration) {
  instant_t target_time;
  instant_t current_time;

  _lf_clock_gettime(&current_time);
  target_time = current_time + sleep_duration;
  // HAL_Delay only supports miliseconds. We try to use that for as long as possible
  //      before switching to another meothd for finer tuned delay times
  long delaytime_ms = sleep_duration / 1000000;
  HAL_Delay(delaytime_ms);

  while (current_time <= target_time)
    _lf_clock_gettime(&current_time);

  return 0;
}

/*  sleep until wakeup time but wake up if there is an async event
 */
int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup_time) {
  // Get the current time and sleep time
  instant_t now;
  _lf_clock_gettime(&now);
  interval_t duration = wakeup_time - now;

  // Edge case handling for super small duration
  if (duration <= 0) {
    return 0;
  } else if (duration < LF_MIN_SLEEP_NS) {
    instant_t current_time;
    _lf_clock_gettime(&current_time);

    // We repurpose the lf_sleep function here, just to better streamline the code
    interval_t sleep_duration = wakeup_time - current_time;
    lf_sleep(sleep_duration);
    return 0;
  }

  // Enable interrupts and prepare to wait
  _lf_async_event = false;
  lf_enable_interrupts_nested();

  do {
    _lf_clock_gettime(&now);
    // Exit when the timer is up or there is an exception
  } while (!_lf_async_event && (now < wakeup_time));

  // Disable interrupts again on exit
  lf_disable_interrupts_nested();

  if (!_lf_async_event) {
    return 0;
  } else {
    return 1;
  }
}

//  + -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- +
//  | Code for enabling and disabling Interrupts
//  + -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- +

// disables the IRQ with support for nested disabling
int lf_disable_interrupts_nested() {
  // Disable interrupts if they are currently enabled
  if (_lf_num_nested_crit_sec == 0) {
    __disable_irq();
  }

  // update the depth of disabling interrupts
  _lf_num_nested_crit_sec++;
  return 0;
}

// enables the IRQ (checks if other processes still want it disabled first)
int lf_enable_interrupts_nested() {
  // Left the critical section more often than it was entered.

  if (_lf_num_nested_crit_sec <= 0) {
    return 1;
  }

  // update the depth of disabling interrupts
  _lf_num_nested_crit_sec--;

  // We enable interrupts again
  if (_lf_num_nested_crit_sec == 0) {
    __enable_irq();
  }
  return 0;
}

int _lf_single_threaded_notify_of_event() {
  _lf_async_event = true;
  return 0;
}

//  + -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- +
//  | Other functions -> taken from the generated main.c
//  + -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- +
void lf_SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    Error_Handler();
  }
}

void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}

#endif