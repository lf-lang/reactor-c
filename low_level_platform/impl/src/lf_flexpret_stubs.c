/**
 * @file lf_flexpret_stubs.c
 * @author Magnus Mæhlum (magnmaeh@stud.ntnu.no)
 * @brief Contains stubs for large `newlib` functions to drastically reduce
 *        code size for the FlexPRET target. This is necessary for FlexPRET
 *        because it has a very limited amount of instruction memory space.
 *
 *        See issue https://github.com/lf-lang/reactor-c/issues/418
 *        for a complete description of the problem.
 *
 *        Passing `-Wl,--wrap=function` causes linker to rename `function`
 *        to `__real_function`. It also calls `__wrap_function` where `function`
 *        used to be called. In this way, the user can implement wrapper functions
 *        for already compiled functions like so:
 *
 *        void __wrap_function() {
 *            printf("Wrapper code before function call\n");
 *            __real_function(); // The actual function call
 *           printf("Wrapper code after function call\n");
 *        }
 *
 *        We instead use it to remove all references to `__real_function`,
 *        which drastically reduces code size. If we do this, the function
 *        signature of the original function does not matter either, so we can
 *        just write `void __wrap_function(void)`.
 * 
 *        https://linux.die.net/man/1/ld (search for `--wrap=symbol`)
 *
 * @date 2024-05-03
 * 
 */

// This reduces FlexPRET's code size by approximately 50%.
void __wrap__vfprintf_r(void) {}
void __wrap___ssvfiscanf_r(void) {}
void __wrap__svfiprintf_r(void) {}
