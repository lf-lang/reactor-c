/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @copyright (c) 2020-2023, The University of California at Berkeley and UT Dallas.
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 * 
 * @brief Utility function for playing audio on Linux or MacOS.
 *
 * Audio functions for Linux or MacOS. To start an audio loop, call
 * `lf_start_audio_loop()`, passing it the logical time at which
 * you would like the loop to start.  To play a waveform,
 * call `lf_play_audio_waveform()`.  A waveform may be
 * synthesized or read from a .wav file using
 * `read_wave_file()` (see wave_file_reader.h).
 *
 * To use this, include the following in your target properties:
 * <pre>
    files: [
        "/lib/c/reactor-c/util/audio_loop_mac.c",
        "/lib/c/reactor-c/util/audio_loop.h",
        "/lib/c/reactor-c/util/audio_loop_linux.c",
    ],
    cmake-include: [
        "/lib/c/reactor-c/util/audio_loop.cmake"
    ]
 * </pre>
 *
 * In addition, you need this in your Lingua Franca file:
 * <pre>
 * preamble {=
 *     #include "audio_loop.h"
 * =}
 * </pre>
 */

#ifndef AUDIO_LOOP_H
#define AUDIO_LOOP_H

#include "wave_file_reader.h" // Defines lf_waveform_t.
#include "tag.h"         // Defines instant_t.

// Constants for playback. These are all coupled.
#define SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE  4410  // 1/10 second, 100 msec
#define BUFFER_DURATION_NS 100000000LL
#define NUM_CHANNELS 1 // 2 for stereo

#define MAX_AMPLITUDE 32765

#define NUM_NOTES 8  // Maximum number of notes that can play simultaneously.

/**
 * Start an audio loop thread that becomes ready to receive
 * audio amplitude samples via add_to_sound(). If there is
 * already an audio loop running, then do nothing.
 * @param start_time The logical time that aligns with the
 *  first audio buffer.
 */
void lf_start_audio_loop(instant_t start_time);

/**
 * Stop the audio loop thread.
 */
void lf_stop_audio_loop();

/**
 * Play the specified waveform with the specified emphasis at
 * the specified time. If the waveform is null, play a simple tick
 * (an impulse). If the waveform has length zero or volume 0,
 * play nothing.
 *
 * If the time is too far in the future
 * (beyond the window of the current audio write buffer), then
 * block until the audio output catches up. If the audio playback
 * has already passed the specified point, then play the waveform
 * as soon as possible and return 1.
 * Otherwise, return 0.
 *
 * @param waveform The waveform to play or NULL to just play a tick.
 * @param emphasis The emphasis (0.0 for silence, 1.0 for waveform volume).
 * @param start_time The time to start playing the waveform.
 */
int lf_play_audio_waveform(lf_waveform_t* waveform, float emphasis, instant_t start_time);

#endif // AUDIO_LOOP_H
