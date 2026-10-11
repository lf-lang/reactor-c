/**
 * @file reaction_macros.h
 * @author Edward A. Lee
 * @brief Macros providing an API for use in inline reaction bodies.
 * @ingroup API
 *
 * This set of macros is defined prior to each reaction body and undefined after the reaction body
 * using the set_undef.h header file.  If you wish to use these macros in external code, such as
 * that implementing a bodiless reaction, then you can include this header file (and at least
 * reactor.h, plus possibly a few other header files) in your code.
 *
 * The purpose for these macros is to provide a semblance of polymorphism even though C does not support
 * polymorphism. For example, `lf_set(port, value)` is a macro where the first argument is a specific
 * port struct and the second type is a value with a type corresponding to the port's type. It is not
 * possible in C to provide a function that can be called with a port struct and a value of any type.
 *
 * Some of the macros are provided for convenience. For example, the macro can automatically provide
 * common arguments such as the environment and can cast arguments to required base types to suppress
 * warning.
 *
 * Note for target language developers. This is one way of developing a target language where
 * the C core runtime is adopted. This file is a translation layer that implements Lingua Franca
 * APIs which interact with the internal APIs.
 */

// Prevent inclusion twice in a row without an intervening inclusion of reaction_macros_undef.h.
#ifndef REACTION_MACROS_H
#define REACTION_MACROS_H

// NOTE: According to the "Swallowing the Semicolon" section on this page:
//    https://gcc.gnu.org/onlinedocs/gcc-3.0.1/cpp_3.html
// some of the following macros should use an odd do-while construct to avoid
// problems with if ... else statements that do not use braces around the
// two branches. Specifically, if the macro expands to more than one statement,
// then the odd construct is needed.

/**
 * @brief Mark a port present.
 * @ingroup API
 *
 * This sets the is_present field of the specified output to true.
 *
 * This macro is a thin wrapper around the lf_set_present() function.
 * It simply casts the argument to `lf_port_base_t*` to suppress warnings.
 *
 * @param out The output port (by name).
 */
#define lf_set_present(out) lf_set_present((lf_port_base_t*)out)

/**
 * @brief Set the specified output (or input of a contained reactor) to the specified value.
 * @ingroup API
 *
 * If the value argument is a primitive type such as int,
 * double, etc. as well as the built-in types bool and string,
 * the value is copied and therefore the variable carrying the
 * value can be subsequently modified without changing the output.
 * This also applies to structs with a type defined by a typedef
 * so that the type designating string does not end in '*'.
 *
 * If the value argument is a pointer
 * to memory that the calling reaction has dynamically allocated,
 * the memory will be automatically freed once all downstream
 * reactions no longer need the value.
 * If 'lf_set_destructor' is called on 'out', then that destructor
 * will be used to free 'value'.
 * Otherwise, the default void free(void*) function is used.
 *
 * @param out The output port (by name) or input of a contained
 *  reactor in form input_name.port_name.
 * @param val The value to insert into the self struct.
 */
#define lf_set(out, val)                                                                                               \
  do {                                                                                                                 \
    out->value = val;                                                                                                  \
    lf_set_present(out);                                                                                               \
    if (((token_template_t*)out)->token != NULL) {                                                                     \
      /* The cast "*((void**) &out->value)" is a hack to make the code */                                              \
      /* compile with non-token types where value is not a pointer. */                                                 \
      lf_token_t* token = _lf_initialize_token_with_value((token_template_t*)out, *((void**)&out->value), 1);          \
      out->token = token;                                                                                              \
    }                                                                                                                  \
  } while (0)

/**
 * @brief Set the specified output (or input of a contained reactor)
 * to the specified array with the given length.
 * @ingroup API
 *
 * The array is assumed to be in dynamically allocated memory.
 * The deallocation is delegated to downstream reactors, which
 * automatically deallocate when the reference count drops to zero.
 *
 * @param out The output port (by name).
 * @param val The array to send (a pointer to the first element).
 * @param len The length of the array to send.
 */
#ifndef __cplusplus
#define lf_set_array(out, val, len)                                                                                    \
  do {                                                                                                                 \
    lf_set_present(out);                                                                                               \
    lf_token_t* token = _lf_initialize_token_with_value((token_template_t*)out, val, len);                             \
    out->token = token;                                                                                                \
    out->value = token->value;                                                                                         \
    out->length = len;                                                                                                 \
  } while (0)
#else
#define lf_set_array(out, val, len)                                                                                    \
  do {                                                                                                                 \
    lf_set_present(out);                                                                                               \
    lf_token_t* token = _lf_initialize_token_with_value((token_template_t*)out, val, len);                             \
    out->token = token;                                                                                                \
    out->value = static_cast<decltype(out->value)>(token->value);                                                      \
    out->length = len;                                                                                                 \
  } while (0)
#endif

/**
 * @brief Set the specified output (or input of a contained reactor)
 * to the specified token value.
 * @ingroup API
 *
 * Tokens in the C runtime wrap messages that are in dynamically allocated memory and
 * perform reference counting to ensure that memory is not freed prematurely.
 *
 * @param out The output port (by name).
 * @param newtoken A pointer to token obtained from an input, an action, or from `lf_new_token()`.
 */
#ifndef __cplusplus
#define lf_set_token(out, newtoken)                                                                                    \
  do {                                                                                                                 \
    lf_set_present(out);                                                                                               \
    _lf_replace_template_token((token_template_t*)out, newtoken);                                                      \
    out->value = newtoken->value;                                                                                      \
    out->length = newtoken->length;                                                                                    \
  } while (0)
#else
#define lf_set_token(out, newtoken)                                                                                    \
  do {                                                                                                                 \
    lf_set_present(out);                                                                                               \
    _lf_replace_template_token((token_template_t*)out, newtoken);                                                      \
    out->value = static_cast<decltype(out->value)>(newtoken->value);                                                   \
    out->length = newtoken->length;                                                                                    \
  } while (0)
#endif

/**
 * @brief Set the destructor associated with the specified port.
 * @ingroup API
 *
 * The destructor will be used to free any value sent through the specified port when all
 * downstream users of the value are finished with it.
 *
 * @param out The output port (by name) or input of a contained reactor in form reactor.port_name.
 * @param dtor A pointer to a void function that takes a pointer argument
 * (or NULL to use the default void free(void*) function.
 */
#define lf_set_destructor(out, dtor) ((token_type_t*)out)->destructor = dtor

/**
 * @brief Set the copy constructor associated with the specified port.
 * @ingroup API
 *
 * The copy constructor will be used to copy any value sent through the specified port whenever
 * a downstream user of the value declares a mutable input port or calls `lf_writable_copy()`.
 *
 * @param out The output port (by name) or input of a contained reactor in form reactor.port_name.
 * @param cpy_ctor A pointer to a void function that takes a pointer argument
 * (or NULL to use the default void `memcpy()` function.
 */
#define lf_set_copy_constructor(out, cpy_ctor) ((token_type_t*)out)->copy_constructor = cpy_ctor

#ifdef MODAL_REACTORS

/**
 * @brief Set the next mode of a modal reactor.
 * @ingroup API
 *
 * As with `lf_set` for outputs, only
 * the last value will have effect if invoked multiple times at any given tag.
 * This works only in reactions with the target mode declared as effect.
 *
 * @param mode The target mode to set for activation.
 */
#define lf_set_mode(mode) _LF_SET_MODE_WITH_TYPE(mode, _lf_##mode##_change_type)

#endif // MODAL_REACTORS

/////////// Convenience macros.
// For simplicity and backward compatability, don't require the environment-pointer when calling the timing API.
// As long as this is done from the context of a reaction, `self` is in scope and is a pointer to the self-struct
// of the current reactor.

/**
 * @brief Return the current tag of the environment invoking this reaction.
 * @ingroup API
 */
#define lf_tag() lf_tag(self->base.environment)

/**
 * @brief Return the current logical time in nanoseconds of the environment invoking this reaction.
 * @ingroup API
 */
#define lf_time_logical() lf_time_logical(self->base.environment)

/**
 * @brief Return the current logical time of the environment invoking this reaction relative to the
 * start time in nanoseconds.
 * @ingroup API
 */
#define lf_time_logical_elapsed() lf_time_logical_elapsed(self->base.environment)

/**
 * @brief Add a worker thread to the pool of worker threads of the environment invoking this reaction.
 * @ingroup API
 *
 * See @ref lf_add_worker_thread(environment_t*) for details.
 * @return 0 on success, or -1 if the thread could not be created or the runtime does not support it.
 */
#define lf_add_worker_thread() lf_add_worker_thread(self->base.environment)

/**
 * @brief Return the number of worker threads that have been added to the environment invoking this reaction.
 * @ingroup API
 *
 * See @ref lf_added_worker_thread_count(environment_t*) for details.
 */
#define lf_added_worker_thread_count() lf_added_worker_thread_count(self->base.environment)

/**
 * @brief Invoke a function that may block for a long time without blocking the rest of the program.
 * @ingroup API
 *
 * This macro invokes `func` with the given arguments and evaluates to the value returned by `func`,
 * but it allows logical time to advance and other reactions to execute while `func` is running.
 * It must be used within a reaction body of a reactor that declares a physical action named `awake`
 * and a reaction triggered by `awake` whose body calls @ref lf_async_resume(). The `awake` reaction
 * must be declared before any reaction that calls `lf_async()`, so that its level is lower:
 * ```
 * physical action awake
 * reaction(awake) {= lf_async_resume(awake); =}
 * reaction(startup) {= int result = lf_async(slow, 50); ... =}
 * ```
 * Specifically:
 *
 * 1. Any outputs set so far with `lf_set`, `lf_set_array`, or `lf_set_token` are propagated at the
 *    current tag, in place, before logical time can advance. A single downstream reaction is executed
 *    in this thread; any others are queued at a later level of the same tag. Reactions triggered by
 *    those outputs therefore observe them at the tag at which they were set.
 * 2. If needed (see the caveats below), a worker thread is added to the pool of the environment (see
 *    @ref lf_add_worker_thread()). Then the worker thread executing the reaction that calls
 *    `lf_async()` temporarily leaves the pool. The remaining workers may therefore advance logical
 *    time and execute other reactions as if the calling reaction had completed.
 * 3. `func` is invoked with the given arguments.
 * 4. When `func` returns, the physical action `awake` is scheduled, and the calling thread blocks until
 *    the reaction triggered by `awake` calls `lf_async_resume(awake)`.
 * 5. The calling thread rejoins the pool, and, once the body of the `awake` reaction has returned, this
 *    macro yields the value returned by `func`. The remainder of the calling reaction therefore
 *    executes at the tag of the `awake` event; in particular, `lf_time_logical()` returns the new
 *    logical time. The propagation of any outputs of the `awake` reaction may proceed concurrently.
 *
 * Caveats:
 *
 * - `func` must return a value; functions returning `void` are not supported.
 * - The value of state variables is not preserved across `lf_async()` calls. Other reactions in this
 *   reactor may have modified them before `lf_async()` returns.
 * - Outputs set before `lf_async()` are propagated at the tag at which they were set, before the call
 *   allows logical time to advance. Outputs set after it returns are propagated when the calling
 *   reaction completes, at the new tag. That later propagation relies on the `awake` reaction having
 *   a lower level than the calling reaction, which the declaration order above ensures: every reaction
 *   downstream of the calling reaction then has a level greater than that of the `awake` reaction, so
 *   it is queued for later in the tag rather than inserted at a level that is already executing. If
 *   the `awake` reaction were declared after the calling reaction, a downstream reaction could have
 *   the same level as the `awake` reaction, which is not supported.
 * - The calling reaction counts as executing while it is suspended. If it is triggered again at an
 *   intervening tag, that triggering is dropped. Other reactions of the same reactor can execute at
 *   intervening tags, so state shared with them should be handled with care.
 * - The reactions of the reactor are mutually exclusive. The first call creates a reactor mutex if
 *   the reactor does not have one. This is similar to reactors with watchdogs.
 * - A worker thread is added to the pool, and persists until the program terminates, only when
 *   every thread previously added by `lf_async()` is compensating for another call that is
 *   currently suspended. Hence, the number of added threads is the maximum number of concurrently
 *   suspended calls; sequential calls share a single added thread.
 * - If the program stops before the tag of the `awake` event, the call returns at the stop tag after
 *   the shutdown reactions have executed. Any further `lf_async()` call made by the resumed reaction
 *   invokes `func` directly without suspending.
 * - This requires the threaded runtime with the NP (default) or GEDF_NP scheduler, and a compiler that
 *   supports statement expressions and `__typeof__` (GCC and Clang do).
 *
 * @param func The function to invoke.
 * @param ... The arguments to pass to `func`.
 * @return The value returned by `func`.
 */
#define lf_async(func, ...)                                                                                            \
  ({                                                                                                                   \
    lf_async_state_t _lf_async_state;                                                                                  \
    _lf_async_begin(&_lf_async_state, self, &self->_lf_awake);                                                         \
    __typeof__(func(__VA_ARGS__)) _lf_async_result = func(__VA_ARGS__);                                                \
    _lf_async_end(&_lf_async_state);                                                                                   \
    _lf_async_result;                                                                                                  \
  })

/**
 * @brief Return the instance name of the reactor.
 * @ingroup API
 *
 * The instance name is the name of given to the instance created by the `new` operator in LF.
 * If the instance is in a bank, then the name will have a suffix of the form `[bank_index]`.
 *
 * @param reactor The reactor to get the name of.
 */
#define lf_reactor_name(reactor) lf_reactor_name(&reactor->base)

/**
 * @brief Return the fully qualified name of the reactor.
 * @ingroup API
 *
 * The fully qualified name of a reactor is the instance name of the reactor concatenated with the names of all
 * of its parents, separated by dots. If the reactor or any of its parents is a bank, then the name
 * will have a suffix of the form `[bank_index]`.
 *
 * @param reactor The reactor to get the name of.
 */
#define lf_reactor_full_name(reactor) lf_reactor_full_name(&reactor->base)

#endif // REACTION_MACROS_H
