/**
 * @file audio_loop.h
 * @author Edward A. Lee
 * @author Soroush Bateni
 *
 * @brief Utility function for playing audio on Linux or MacOS.
 * @ingroup Utilities
 *
 * Audio functions for Linux or MacOS. To start an audio loop, call
 * @ref lf_start_audio_loop(), passing it the logical time at which
 * you would like the loop to start.  To play a waveform,
 * call @ref lf_play_audio_waveform().  A waveform may be
 * synthesized or read from a .wav file using
 * @ref read_wave_file() (see @ref wave_file_reader.h).
 *
 * To use this, include the following in your target properties:
 *
 * ```
    files: [
        "/lib/c/reactor-c/util/audio_loop_mac.c",
        "/lib/c/reactor-c/util/audio_loop.h",
        "/lib/c/reactor-c/util/audio_loop_linux.c",
    ],
    cmake-include: [
        "/lib/c/reactor-c/util/audio_loop.cmake"
    ]
 * ```
 *
 * In addition, you need this in your Lingua Franca file:
 *
 * ```
 * preamble {=
 *     #include "audio_loop.h"
 * =}
 * ```
 */

#ifndef AUDIO_LOOP_H
#define AUDIO_LOOP_H

#include "wave_file_reader.h" // Defines lf_waveform_t.
#include "tag.h"              // Defines instant_t.

#ifndef SAMPLE_RATE
/**
 * @brief Sample rate for audio playback.
 * @ingroup Utilities
 */
#define SAMPLE_RATE 44100
#endif

#ifndef AUDIO_BUFFER_SIZE
/**
 * @brief Size of the audio buffer.
 * @ingroup Utilities
 */
#define AUDIO_BUFFER_SIZE 4410 // 1/10 second, 100 msec
#endif

#ifndef BUFFER_DURATION_NS
/**
 * @brief Duration of the audio buffer.
 * @ingroup Utilities
 */
#define BUFFER_DURATION_NS 100000000LL
#endif

#ifndef NUM_CHANNELS
/**
 * @brief Number of channels for audio playback.
 * @ingroup Utilities
 */
#define NUM_CHANNELS 1 // 2 for stereo
#endif

#ifndef MAX_AMPLITUDE
/**
 * @brief Maximum amplitude for audio playback.
 * @ingroup Utilities
 */
#define MAX_AMPLITUDE 32765
#endif

#ifndef NUM_NOTES
/**
 * @brief Maximum number of notes that can play simultaneously.
 * @ingroup Utilities
 */
#define NUM_NOTES 8 // Maximum number of notes that can play simultaneously.
#endif

/**
 * @brief Start an audio loop thread that becomes ready to receive
 * audio amplitude samples via @ref add_to_sound().
 * @ingroup Utilities
 *
 * If there is already an audio loop running, then do nothing.
 *
 * @param start_time The logical time that aligns with the first audio buffer.
 */
void lf_start_audio_loop(instant_t start_time);

/**
 * @brief Stop the audio loop thread.
 * @ingroup Utilities
 */
void lf_stop_audio_loop();

/**
 * @brief Play the specified waveform with the specified emphasis at the specified time.
 * @ingroup Utilities
 *
 * If the waveform is null, play a simple tick (an impulse).
 * If the waveform has length zero or volume 0, play nothing.
 *
 * If the time is too far in the future (beyond the window of the current audio write buffer),
 * then block until the audio output catches up. If the audio playback has already passed the
 * specified point, then play the waveform as soon as possible and return 1.
 * Otherwise, return 0.
 *
 * @param waveform The waveform to play or NULL to just play a tick.
 * @param emphasis The emphasis (0.0 for silence, 1.0 for waveform volume).
 * @param start_time The time to start playing the waveform.
 */
int lf_play_audio_waveform(lf_waveform_t* waveform, float emphasis, instant_t start_time);

/**
 * @brief Add the given value to the current write buffer at the specified index.
 * @ingroup Utilities
 *
 * If the resulting value is larger than what can be represented in the 16-bit short, truncate it.
 *
 * @param index_offset Where in the buffer to add the amplitude.
 * @param value The amplitude to add to whatever amplitude is already there.
 */
void add_to_sound(int index_offset, double value);

#endif // AUDIO_LOOP_H
