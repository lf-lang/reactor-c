/**
 * @file
 * @author Edward A. Lee
 * @copyright (c) 2020-2023, The University of California at Berkeley and UT Dallas.
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 * 
 * @brief Simple terminal-based user interface based on ncurses.
 * See sensor_simulator.h.
 */

#include <pthread.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "sensor_simulator.h"
#include "include/api/api.h"
#include "util.h"
#include "platform.h"

// Maximum number of milliseconds that wgetchr will block for.
#define WGETCHR_TIMEOUT 1000

// Support ASCII characters SPACE (32) through DEL (127).
#define LF_SENSOR_TRIGGER_TABLE_SIZE 96

/** Table of Lingua Franca trigger objects to schedule in response to keypresses. */
trigger_t* _lf_sensor_trigger_table[LF_SENSOR_TRIGGER_TABLE_SIZE];

/** Trigger for the newline character '\n', which is platform dependent. */
trigger_t* _lf_sensor_sensor_newline_trigger = NULL;

/** Trigger for any key. */
trigger_t* _lf_sensor_any_key_trigger = NULL;

lf_mutex_t _lf_sensor_mutex;
lf_cond_t _lf_sensor_simulator_cond_var;

enum _lf_sensor_message_type {
	_lf_sensor_message, _lf_sensor_tick, _lf_sensor_close_windows
};

typedef struct _lf_sensor_message_t {
	enum _lf_sensor_message_type type;
    char* message;
	struct _lf_sensor_message_t* next;
} _lf_sensor_message_t;

struct {
	lf_thread_t input_thread_id;
	lf_thread_t output_thread_id;
	int thread_created;

	/**
	 * Default window from which to get input characters.
	 * If show_welcome_message() is called, this will be the welcome
	 * message window. Otherwise, it will be stdscr, the default
	 * curses window.
	 */
	WINDOW* default_window;

	/** Tick window. */
	WINDOW* tick_window;

	/**
	 * Keep track of the tick cursor position directly so it
	 * doesn't get as messed up by printf() calls.
	 */
	int tick_cursor_x, tick_cursor_y;
	int print_cursor_x, print_cursor_y;

	/** Print window. */
	WINDOW* print_window;

	/** The print window height. */
	int print_window_height;

	/** File to which to write log data, or NULL to send to window. */
	FILE* log_file;

	/** The welcome message. */
	const char** welcome_message;

	/** The length of the welcome message. */
	int welcome_message_length;

	struct _lf_sensor_message_t* message_q;
	struct _lf_sensor_message_t* message_recycle_q;

	/** The width of the tick window. */
	int tick_window_width;
} _lf_sensor;

/**
 * Put a persistent message in the upper left of the terminal window.
 * The message will be left justified, with each string
 * in the specified array on a new line.
 * This should not be called directly by the user.
 * It assumes the mutex lock is held.
 * @param message_lines The message lines.
 * @param number_of_lines The number of lines.
 */
void _lf_show_message(const char* message_lines[], int number_of_lines) {
    int term_height, term_width;
    int message_width = 0;
    // Find the widest message in the list.
    for (int i = 0; i < number_of_lines; i++) {
        size_t width = strlen(message_lines[i]);
        if (width > message_width) {
            message_width = width;
        }
    }
    getmaxyx(stdscr, term_height, term_width);   // Get the size of the terminal window.
    WINDOW* center_win = newwin(number_of_lines + 2, message_width + 2, 0, 0);
    box(center_win, 0, 0);
    wrefresh(center_win);

    // wattron(center_win, COLOR_PAIR(2));

    for (int i = 0; i < number_of_lines; i++) {
        mvwprintw(center_win, i + 1, 1, "%s", message_lines[i]);
        // According to curses docs, the following should not be necessary
        // after each print. But if I wait and do it later, the output
        // gets garbled.
        wrefresh(center_win);
    }
}

/**
 * Start a tick window on the right of the terminal window.
 * This should not be called directly by the user.
 * It assumes the mutex lock is held.
 * @param width The width of the window.
 */
void _lf_start_tick_window(int width) {
    int term_height, term_width;
    getmaxyx(stdscr, term_height, term_width);   // Get the size of the terminal window.
    _lf_sensor.tick_window = newwin(term_height, width + 2, 0, term_width - width - 2);
    box(_lf_sensor.tick_window, 0, 0);
    wrefresh(_lf_sensor.tick_window);
    wmove(_lf_sensor.tick_window, 1, 1);  // Ensure to not overwrite the box.
    _lf_sensor.tick_cursor_x = _lf_sensor.tick_cursor_y = 1;
    // move(0, 0);
}

/**
 * Start a window on the bottom left of the terminal window
 * for printed messages from the application.
 * This should not be called directly by the user.
 * It assumes the mutex lock is held.
 * @param above Space to leave above the window.
 * @param right Space to leave to the right of the window.
 */
void _lf_start_print_window(int above, int right) {
    int term_height, term_width;
    getmaxyx(stdscr, term_height, term_width);   // Get the size of the terminal window.
    _lf_sensor.print_window_height = term_height - above;
    _lf_sensor.print_window = newwin(_lf_sensor.print_window_height, term_width - right, above, 0);
    wrefresh(_lf_sensor.print_window);
    wmove(_lf_sensor.print_window, 0, 0);
    _lf_sensor.print_cursor_y = _lf_sensor.print_cursor_x = 0;
    _lf_sensor.default_window = _lf_sensor.print_window;
}

/**
 * Post a message to be displayed.
 * This acquires the mutex lock.
 * @param type The message type, one of
 *  _lf_sensor_message, _lf_sensor_tick, or _lf_sensor_close_window.
 * @param body The message, or NULL for exit type.
 */
void _lf_sensor_post_message(enum _lf_sensor_message_type type, char* body) {
    lf_mutex_lock(&_lf_sensor_mutex);
    _lf_sensor_message_t* message = _lf_sensor.message_recycle_q;
    if (message == NULL) {
    	// Create a new message struct.
    	message = calloc(1, sizeof(_lf_sensor_message_t));
    } else {
    	// Take this item off the recycle queue.
    	_lf_sensor.message_recycle_q = _lf_sensor.message_recycle_q->next;
    }
    message->message = body;
    message->type = type;
	message->next = NULL; // Will be the new last message in the queue.
	// Find the tail of the message queue and put the message there.
	_lf_sensor_message_t* tail = _lf_sensor.message_q;
	if (tail == NULL) {
		_lf_sensor.message_q = message;
	} else {
		while (tail != NULL) {
			if (tail->next == NULL) {
				// tail is the last message in the queue.
				tail->next = message;
				break;
			}
			// Not yet at the last message.
			tail = tail->next;
		}
	}
	lf_cond_signal(&_lf_sensor_simulator_cond_var);
    lf_mutex_unlock(&_lf_sensor_mutex);
}

/**
 * Function to register to handle printing of messages in util.h/c.
 * This acquires the mutex lock.
 */
void _lf_print_message_function(const char* format, va_list args) {
	if (_lf_sensor.log_file != NULL) {
		// Write to a log file in addition to the window.
		vfprintf(_lf_sensor.log_file, format, args);
	}
    char* copy;
    vasprintf(&copy, format, args);
    _lf_sensor_post_message(_lf_sensor_message, copy);
}

/**
 * Thread to read input characters until an EOF is received.
 * For each character received, if there is a registered trigger
 * for that character, schedule that trigger with a payload
 * equal to the character that was typed.
 * Otherwise, the character is ignored.
 */
void* _lf_sensor_read_input(void* ignored) {
    while(_lf_sensor.thread_created != 0) {
        int c = wgetch(_lf_sensor.default_window);
        if (c == EOF) {
            // End of file received. Exit thread.
            break;
        } else if (c != ERR) {
            // wgetch returns ERR if it times out, in which case, we continue
            // and check whether _lf_sensor.thread_created has been set to 0.
            // So here, ERR was not returned.
            // It is imperative that we not hold the _lf_sensor_mutex when
            // calling schedule(), because schedule() acquires another mutex.
            // We would create a deadlock risk.  The following code is correct
            // because a _lf_sensor_trigger_table entry, once assigned a value, becomes
            // immutable.
            if (c == '\n' && _lf_sensor_sensor_newline_trigger != NULL) {
                lf_schedule_copy(_lf_sensor_sensor_newline_trigger, 0, &c, 1);
            } else if (c - 32 >= 0 && c - 32 < LF_SENSOR_TRIGGER_TABLE_SIZE && _lf_sensor_trigger_table[c-32] != NULL) {
                lf_schedule_copy(_lf_sensor_trigger_table[c-32], 0, &c, 1);
            }
            // Any key trigger triggers after specific keys.
            if (_lf_sensor_any_key_trigger != NULL) {
                lf_schedule_copy(_lf_sensor_any_key_trigger, 0, &c, 1);
            }
        }
    }
    return NULL;
}

/**
 * Thread to display strings in either the tick window or the
 * message window.
 */
void* _lf_sensor_simulator_thread(void* ignored) {
    lf_mutex_lock(&_lf_sensor_mutex);
    _lf_sensor.thread_created = 1;
    // Clean up any previous curses state.
    if (!isendwin()) {
        endwin();
    }
    // Initialize ncurses.
    LF_PRINT_DEBUG("Initializing ncurses.");
    initscr();
    start_color();     // Allow colors.
    noecho();          // Don't echo input
    cbreak();          // Don't wait for Return or Enter
    wtimeout(stdscr, WGETCHR_TIMEOUT); // Don't wait longer than this for input.
    keypad(stdscr, TRUE);  // Enable keypad input.
    refresh();         // Not documented, but needed?

    _lf_sensor.default_window = stdscr;
    if (_lf_sensor.welcome_message != NULL && _lf_sensor.welcome_message_length > 0) {
        _lf_show_message(_lf_sensor.welcome_message, _lf_sensor.welcome_message_length);
    }
    _lf_sensor.tick_window = stdscr;
    if (_lf_sensor.tick_window_width > 0) {
        _lf_start_tick_window(_lf_sensor.tick_window_width);
    }
    _lf_start_print_window(_lf_sensor.welcome_message_length + 2, _lf_sensor.tick_window_width + 2);

    // ncurses is not thread safe, but since the wtimeout option does not work,
    // there is no way to simultaneously listen for inputs and produce outputs.
    // Here, we create a thread that produces no output and just listens for input.
    // This thread is exclusively responsible for producing output.
    int result = lf_thread_create(&_lf_sensor.input_thread_id, &_lf_sensor_read_input, NULL);
    if (result != 0) {
        lf_print_error("Failed to start sensor simulator input listener!");
    }

    while(_lf_sensor.thread_created != 0) {
    	// Sadly, ncurses is not thread safe, so this thread deals with all messages.
    	while (_lf_sensor.message_q == NULL) {
            lf_cond_wait(&_lf_sensor_simulator_cond_var);
    	}
    	// Show all messages in the queue.
		while (_lf_sensor.message_q != NULL) {
			if (_lf_sensor.message_q->type == _lf_sensor_close_windows) {
			    lf_register_print_function(NULL, -1);
			    endwin();
			    lf_mutex_unlock(&_lf_sensor_mutex);
				return NULL;
			} else if (_lf_sensor.message_q->type == _lf_sensor_tick) {
			    wmove(_lf_sensor.tick_window, _lf_sensor.tick_cursor_y, _lf_sensor.tick_cursor_x);
			    wprintw(_lf_sensor.tick_window, _lf_sensor.message_q->message);
			    int tick_height, tick_width;
			    getmaxyx(_lf_sensor.tick_window, tick_height, tick_width);
			    _lf_sensor.tick_cursor_x += strlen(_lf_sensor.message_q->message);
			    if (_lf_sensor.tick_cursor_x >= tick_width - 1) {
			        _lf_sensor.tick_cursor_x = 1;
			        _lf_sensor.tick_cursor_y++;
			    }
			    if (_lf_sensor.tick_cursor_y >= tick_height - 1) {
			        _lf_sensor.tick_cursor_y = 1;
			    }
			    wmove(_lf_sensor.tick_window, _lf_sensor.tick_cursor_y, _lf_sensor.tick_cursor_x);
			    wrefresh(_lf_sensor.tick_window);
			} else if (_lf_sensor.message_q->type == _lf_sensor_message) {
				wmove(_lf_sensor.print_window, _lf_sensor.print_cursor_y, _lf_sensor.print_cursor_x);
				wclrtoeol(_lf_sensor.print_window);
				wprintw(_lf_sensor.print_window, _lf_sensor.message_q->message);
				_lf_sensor.print_cursor_x = 0;
				_lf_sensor.print_cursor_y += 1;
				if (_lf_sensor.print_cursor_y >= _lf_sensor.print_window_height - 1) {
					_lf_sensor.print_cursor_y = 0;
				}
				wmove(_lf_sensor.print_window, _lf_sensor.print_cursor_y, _lf_sensor.print_cursor_x);
				wclrtoeol(_lf_sensor.print_window);
				wrefresh(_lf_sensor.print_window);

				free(_lf_sensor.message_q->message);
			}
			refresh();
			_lf_sensor_message_t* tmp_recycle = _lf_sensor.message_recycle_q;
			_lf_sensor_message_t* tmp_message = _lf_sensor.message_q;
			_lf_sensor.message_recycle_q = _lf_sensor.message_q;
			_lf_sensor.message_q = tmp_message->next;
			_lf_sensor.message_recycle_q->next = tmp_recycle;
		}
    }
    lf_mutex_unlock(&_lf_sensor_mutex);
    return NULL;
}

void end_sensor_simulator() {
    lf_register_print_function(NULL, -1);
	_lf_sensor_post_message(_lf_sensor_close_windows, NULL);

	void* thread_return;
	lf_thread_join(_lf_sensor.output_thread_id, &thread_return);

    // Timeout mode should result in the input thread exiting on its own.
    // pthread_kill(_lf_sensor.input_thread_id, SIGINT);

    _lf_sensor.thread_created = 0;
	if (_lf_sensor.log_file != NULL) {
		fclose(_lf_sensor.log_file);
	}
}

int start_sensor_simulator(
		const char* message_lines[],
		int number_of_lines,
		int tick_window_width,
		char* log_file,
		int log_level
) {
    int result = 0;
    _lf_sensor.welcome_message = message_lines;
    _lf_sensor.welcome_message_length = number_of_lines;
    _lf_sensor.tick_window_width = tick_window_width;
    _lf_sensor.message_q = NULL;
    _lf_sensor.message_recycle_q = NULL;
    _lf_sensor.thread_created = 0;
    lf_cond_init(&_lf_sensor_simulator_cond_var, &_lf_sensor_mutex);
    if (_lf_sensor.thread_created == 0) {
        // Thread has not been created.
        // Zero out the trigger table.
        for (int i = 0; i < LF_SENSOR_TRIGGER_TABLE_SIZE; i++) {
            _lf_sensor_trigger_table[i] = NULL;
        }
        // For some strange reason, this log file has to be opened before
        // ncurses is initialized, otherwise, ncurses gets disabled (won't
        // accept input).
    	if (log_file != NULL) {
    		_lf_sensor.log_file = fopen(log_file, "w");
    	} else {
    		_lf_sensor.log_file = NULL;
    	}
    	// Register the print function before starting the thread.
    	// Subsequent print messages will go into the queue.
        lf_register_print_function(&_lf_print_message_function, log_level);

        // FIXME: Is this needed? Users should call end_sensor_simulator in
        // a shutdown reaction.
        if (atexit(end_sensor_simulator) != 0) {
            lf_print_warning("sensor_simulator: Failed to register end_sensor_simulator function!");
        }

        // ncurses is not thread safe, so create a one thread that does all
        // the writing to the window and one that does all the reading.
        result = lf_thread_create(&_lf_sensor.output_thread_id, &_lf_sensor_simulator_thread, NULL);
        if (result != 0) {
            lf_print_error("Failed to start sensor simulator!");
        }
    }
    return result;
}

void show_tick(const char* character) {
    if (character != NULL) {
        char* copy;
        asprintf(&copy, "%s", character);
    	_lf_sensor_post_message(_lf_sensor_tick, copy);
    }
}

int register_sensor_key(char key, void* action) {
    if (action == NULL) {
        return 3;
    }
    int index = key - 32;
    if (key != '\n' && key != '\0' && (index < 0 || index >= LF_SENSOR_TRIGGER_TABLE_SIZE)) {
        return 2;
    }
    int result = 0;
    lf_mutex_lock(&_lf_sensor_mutex);
    if (key == '\n') {
        if (_lf_sensor_sensor_newline_trigger != NULL) {
            result = 1;
        } else {
            _lf_sensor_sensor_newline_trigger = action;
        }
    } else if (key == '\0') {
        // Any key trigger.
        if (_lf_sensor_any_key_trigger != NULL) {
            result = 1;
        } else {
            _lf_sensor_any_key_trigger = action;
        }
    } else if (_lf_sensor_trigger_table[index] != NULL) {
        result = 1;
    } else {
        _lf_sensor_trigger_table[index] = action;
    }
    lf_mutex_unlock(&_lf_sensor_mutex);
    return result;
}
