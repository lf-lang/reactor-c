/**
 * @file modes.h
 * @author Alexander Schulz-Rosengarten
 * @author Soroush Bateni
 *
 * @brief Header file of the runtime infrastructure for modes in the C target of Lingua Franca.
 * @ingroup Modal
 *
 * This file contains user macros for setting new modes, forward declarations for
 * functions and types used in the generated code, and types definitions for the modal
 * representation.
 *
 * Any mode related code will only work in the presence of the MODAL_REACTORS
 * definition.
 * However, this header should be included regardless to provide definitions
 * for mode-unaware pre-compilation.
 *
 * This file is intended for direct include in reactor.h augmenting certain type
 * definitions.
 */
#ifndef MODES_H
#define MODES_H

#ifdef MODAL_REACTORS

#include <stddef.h>
#include <stdbool.h>

#include "lf_types.h"
#include "tag.h"

typedef struct event_t event_t;
typedef struct reaction_t reaction_t;
typedef struct trigger_t trigger_t;

////////////////////////////////////////////////////////////
//// Macros for setting modes.

/**
 * @brief Set the next mode of a modal reactor with an explicit change type
 * (reset or history, from the enum `lf_mode_change_type_t`).
 * @ingroup Modal
 *
 * This macro is not meant to be used by LF programmers.
 * It is used in Python.
 *
 * @param mode The target mode to set for activation.
 * @param change_type The change type of the transition.
 */
#define _LF_SET_MODE_WITH_TYPE(mode, change_type)                                                                      \
  do {                                                                                                                 \
    ((self_base_t*)self)->_lf__mode_state.next_mode = mode;                                                            \
    ((self_base_t*)self)->_lf__mode_state.mode_change = change_type;                                                   \
  } while (0)

////////////////////////////////////////////////////////////
//// Type definitions for modal infrastructure.

/* Typedef for reactor_mode_t struct, used for representing a mode. */
typedef struct reactor_mode_t reactor_mode_t;
/* Typedef for reactor_mode_state_t struct, used for storing modal state of reactor and/or its relation to enclosing
 * modes. */
typedef struct reactor_mode_state_t reactor_mode_state_t;
/* Typedef for mode_state_variable_reset_data_t struct, used for storing data for resetting state variables nested in
 * modes. */
typedef struct mode_state_variable_reset_data_t mode_state_variable_reset_data_t;

/**
 * @brief Type of the mode change.
 * @ingroup Modal
 */
typedef enum { no_transition, reset_transition, history_transition } lf_mode_change_type_t;

/**
 * @brief A struct to represent a single mode instace in a reactor instance.
 * @ingroup Modal
 */
struct reactor_mode_t {
  /**
   * @brief Pointer to a struct with the reactor's mode state for this mode instance.
   *
   * This links to the state information associated with this mode.
   */
  reactor_mode_state_t* state;
  /**
   * @brief Name of this mode (null-terminated string).
   *
   * Useful for debugging and tracing.
   */
  char* name;
  /**
   * @brief The logical time when the mode was deactivated (left).
   *
   * Used to track when the mode was last exited.
   */
  instant_t deactivation_time;
  /**
   * @brief Bit vector for several internal flags related to the mode.
   *
   * Used for internal bookkeeping (e.g., active, scheduled for activation, etc.).
   */
  uint8_t flags;
};

/**
 * @brief A struct to store state of the modes in a reactor instance and/or its relation to enclosing modes.
 * @ingroup Modal
 */
struct reactor_mode_state_t {
  /**
   * @brief Pointer to the next enclosing mode (if exists).
   *
   * Used to represent the parent mode in a hierarchy of modes.
   */
  reactor_mode_t* parent_mode;
  /**
   * @brief Pointer to the initial mode for this state.
   *
   * Indicates which mode is considered the initial mode.
   */
  reactor_mode_t* initial_mode;
  /**
   * @brief Pointer to the currently active mode (only locally active).
   *
   * Tracks which mode is currently active in this state.
   */
  reactor_mode_t* current_mode;
  /**
   * @brief Pointer to the next mode to activate at the end of this step (if set).
   *
   * Used to schedule a mode transition.
   */
  reactor_mode_t* next_mode;
  /**
   * @brief A mode change type flag.
   *
   * Indicates the type of mode change (none, reset, or history).
   */
  lf_mode_change_type_t mode_change;
};

/**
 * @brief A struct to store data for resetting state variables nested in modes.
 * @ingroup Modal
 */
struct mode_state_variable_reset_data_t {
  /**
   * @brief Pointer to the enclosing mode for this reset data.
   *
   * Indicates which mode this reset data is associated with.
   */
  reactor_mode_t* mode;
  /**
   * @brief Pointer to the target variable to be reset.
   *
   * The variable that will be reset when the mode is entered or exited.
   */
  void* target;
  /**
   * @brief Pointer to the data source for resetting the variable.
   *
   * The source data used to reset the target variable.
   */
  void* source;
  /**
   * @brief The size of the variable to be reset (in bytes).
   *
   * Used to determine how much data to copy from source to target.
   */
  size_t size;
};

////////////////////////////////////////////////////////////
//// Forward declaration
typedef struct environment_t environment_t;

////////////////////////////////////////////////////////////
//// Modes API

/**
 * @brief Initialize the modes in the environment.
 * @ingroup Modal
 *
 * @param env The environment to initialize the modes in.
 */
void _lf_initialize_modes(environment_t* env);

/**
 * @brief Handle the mode changes in the environment.
 * @ingroup Modal
 *
 * @param env The environment to handle the mode changes in.
 */
void _lf_handle_mode_changes(environment_t* env);

/**
 * @brief Handle the mode triggered reactions in the environment.
 * @ingroup Modal
 *
 * @param env The environment to handle the mode triggered reactions in.
 */
void _lf_handle_mode_triggered_reactions(environment_t* env);

/**
 * @brief Check whether a mode is active.
 * @ingroup Modal
 *
 * @param mode The mode to check.
 * @return True if the mode is active, false otherwise.
 */
bool _lf_mode_is_active(reactor_mode_t* mode);

/**
 * @brief Initialize the mode states in the environment.
 * @ingroup Modal
 *
 * @param env The environment to initialize the mode states in.
 * @param states The array of mode states to initialize.
 * @param states_size The size of the states array.
 */
void _lf_initialize_mode_states(environment_t* env, reactor_mode_state_t* states[], int states_size);

/**
 * @brief Process the mode changes in the environment.
 * @ingroup Modal
 *
 * @param env The environment to process the mode changes in.
 * @param states The array of mode states to process.
 * @param states_size The size of the states array.
 * @param reset_data The array of reset data to process.
 * @param reset_data_size The size of the reset data array.
 * @param timer_triggers The array of timer triggers to process.
 * @param timer_triggers_size The size of the timer triggers array.
 */
void _lf_process_mode_changes(environment_t* env, reactor_mode_state_t* states[], int states_size,
                              mode_state_variable_reset_data_t reset_data[], int reset_data_size,
                              trigger_t* timer_triggers[], int timer_triggers_size);

/**
 * @brief Add a suspended event to the list of suspended events.
 * @ingroup Modal
 *
 * @param event The event to add.
 */
void _lf_add_suspended_event(event_t* event);

/**
 * @brief Handle the mode startup reset reactions in the environment.
 * @ingroup Modal
 *
 * @param env The environment to handle the mode startup reset reactions in.
 * @param startup_reactions The array of startup reactions to handle.
 * @param startup_reactions_size The size of the startup reactions array.
 * @param reset_reactions The array of reset reactions to handle.
 * @param reset_reactions_size The size of the reset reactions array.
 * @param states The array of mode states to handle.
 * @param states_size The size of the states array.
 */
void _lf_handle_mode_startup_reset_reactions(environment_t* env, reaction_t** startup_reactions,
                                             int startup_reactions_size, reaction_t** reset_reactions,
                                             int reset_reactions_size, reactor_mode_state_t* states[], int states_size);

/**
 * @brief Handle the mode shutdown reactions in the environment.
 * @ingroup Modal
 *
 * @param env The environment to handle the mode shutdown reactions in.
 * @param shutdown_reactions The array of shutdown reactions to handle.
 * @param shutdown_reactions_size The size of the shutdown reactions array.
 */
void _lf_handle_mode_shutdown_reactions(environment_t* env, reaction_t** shutdown_reactions,
                                        int shutdown_reactions_size);

/**
 * @brief Terminate the modal reactors in the environment.
 * @ingroup Modal
 *
 * @param env The environment to terminate the modal reactors in.
 */
void _lf_terminate_modal_reactors(environment_t* env);

#else  /* IF NOT MODAL_REACTORS */
/*
 * Reactions and triggers must have a mode pointer to set up connection to enclosing modes,
 * also when they are precompiled without modal reactors in order to later work in modal reactors.
 * Hence define mode type as void in the absence of modes to treat mode pointer as void pointers for that time being.
 */
typedef void reactor_mode_t;
#endif /* MODAL_REACTORS */
#endif /* MODES_H */
