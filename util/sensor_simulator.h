#ifndef SENSOR_SIMULATOR_H
#define SENSOR_SIMULATOR_H
/**
 * @file
 * @author Edward A. Lee
 * @copyright (c) 2020-2023, The University of California at Berkeley and UT Dallas.
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 * 
 * @brief Simple terminal-based user interface based on ncurses.
 * 
 * When prototyping Lingua Franca programs on a laptop, it is convenient to use
 * the laptop keyboard to simulate asynchronous sensor input. This small library
 * provides a convenient way to do that.
 * 
 * To use this, include the following flags in your target properties:
 * <pre>
 * target C {
    files: [
        "/lib/c/reactor-c/util/sensor_simulator.c", 
        "/lib/c/reactor-c/util/sensor_simulator.h",
    ],
    cmake-include: [
        "/lib/c/reactor-c/util/sensor_simulator.cmake",
    ] 
 * };
 * </pre>
 * This requires `ncurses`, a library providing somewhat portable keyboard access.
 * 
 * In addition, you need this in your Lingua Franca file:
 * <pre>
 * preamble {=
 *     #include "sensor_simulator.c"
 * =}
 * </pre>
 * To start the sensor simulator, call `start_sensor_simulator` passing it
 * an array of strings to print and the width of the window to use to display
 * characters using the `show_tick` function.
 * 
 * To print messages to the screen, rather than using printf(), you should use
 * the messaging functions in util.h, such as lf_print(). Otherwise, your messages
 * will be printed over other information.
 */

/**
 * Start the sensor simulator if it has not been already
 * started. This must be called at least once before any
 * call to register_sensor_key.  The specified message
 * is an initial message to display at the upper left,
 * typically a set of instructions, that remains displayed
 * throughout the lifetime of the window. Please ensure that
 * the message_lines array and its contained strings are not
 * on the stack because they will be used later in a separate
 * thread.
 * @param message_lines The message lines.
 * @param number_of_lines The number of lines.
 * @param tick_window_width The width of the tick window or 0 for none.
 * @param log_file If non-NULL, the name of a file to which to write logging messages.
 * @param log_level The level of log messages to redirect to the file.
 *  The level should be one of LOG_LEVEL_ERROR, LOG_LEVEL_WARNING,
 *  LOG_LEVEL_INFO, LOG_LEVEL_LOG, LOG_LEVEL_DEBUG, or LOG_LEVEL_ALL.
 * @return 0 for success, error code for failure.
 */
int start_sensor_simulator(
		const char* message_lines[],
		int number_of_lines,
		int tick_window_width,
		char* log_file,
		int log_level
);

/**
 * End ncurses control of the terminal.
 */
void end_sensor_simulator();

/**
 * Place a tick (usually a single character) in the tick window.
 * @param character The tick character.
 */
void show_tick(const char* character);

/**
 * Register a keyboard key to trigger the specified action.
 * Printable ASCII characters (codes 32 to 127) are supported
 * plus '\n' and '\0', where the latter registers a trigger
 * to invoked when any key is pressed. If a specific key is
 * registered and any key ('\0') is also registered, the
 * any key trigger will be scheduled after the specific key
 * is scheduled. If these triggers belong to different reactors,
 * they could be invoked in parallel.
 * This will fail if the specified key has already been
 * registered (error code 1), or the key is not a supported key or a
 * newline ‘\n’ or any key '\0' (error code 2) or if the trigger is NULL
 * (error code 3).
 * @param key The key to register.
 * @param action The action to trigger when the key is pressed
 *  (a pointer to a trigger_t struct).
 * @return 0 for success, error code for failure.
 */
int register_sensor_key(char key, void* action);

#endif // SENSOR_SIMULATOR_H
