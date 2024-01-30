#if defined(PLATFORM_ARDUINO)
    #include "platform/lf_arduino_support.h"
#elif defined(PLATFORM_ZEPHYR)
    #include "platform/lf_zephyr_support.h"
#elif defined(PLATFORM_NRF52)
    #include "platform/lf_nrf52_support.h"
#elif defined(PLATFORM_RP2040)
    #include "platform/lf_rp2040_support.h"
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
   // Windows platforms
   #include "lf_windows_support.h"
#elif __APPLE__
    // Apple platforms
    #include "lf_macos_support.h"
#elif __linux__
    // Linux
    #include "lf_linux_support.h"
#elif __unix__ // all unices not caught above
    // Unix
    #include "lf_POSIX_threads_support.h"
#elif defined(_POSIX_VERSION)
    // POSIX
    #include "lf_POSIX_threads_support.h"
#elif defined(__riscv) || defined(__riscv__)
    // RISC-V (see https://github.com/riscv/riscv-toolchain-conventions)
    #error "RISC-V not supported"
#else
#error "Platform not supported"
#endif
