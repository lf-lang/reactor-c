/**
 * @file wave_file_reader.h
 * @author Edward A. Lee
 *
 * @brief Utility function for reading WAV audio files.
 * @ingroup Utilities
 *
 * This defines functions and data types for importing audio files with the
 * wave audio format. The main function is read_wave_file(), which, given
 * a path to a .wav file, reads the file and, if the format of the file is
 * supported, returns an lf_waveform_t struct, which contains the raw
 * audio data in 16-bit linear PCM form.
 *
 * This code has few dependencies, so it should run on just about any platform.
 *
 * To use this, include the following flags in your target properties:
 *
 * ```
 * target C {
 *     files: [
 *         "/lib/c/reactor-c/util/wave_file_reader.c",
 *         "/lib/c/reactor-c/util/wave_file_reader.h"
 *     ],
 *     cmake-include: [
 *         "/lib/c/reactor-c/util/wave_file_reader.cmake"
 *     ]
 * }
 * ```
 *
 * In addition, you need this in your Lingua Franca file or reactor:
 *
 * ```
 * preamble {=
 *     #include "wave_file_reader.h"
 * =}
 * ```
 */

#ifndef WAVE_FILE_READER_H
#define WAVE_FILE_READER_H

/**
 * @brief Waveform in 16-bit linear-PCM format.
 * @ingroup Utilities
 *
 * The waveform element is an array containing audio samples.
 * If there are two channels, then they are interleaved left and right channel.
 * The length is the total number of samples, a multiple of the number of channels.
 */
typedef struct lf_waveform_t {
  uint32_t length;
  uint16_t num_channels;
  int16_t* waveform;
} lf_waveform_t;

/**
 * @brief Open a wave file, check that the format is supported, allocate memory for the sample data,
 * and fill the memory with the sample data.
 * @ingroup Utilities
 *
 * It is up to the caller to free the memory when done with it.
 * That code should first free the waveform element of the returned struct, then the struct itself.
 * This implementation supports only 16-bit linear PCM files.
 * On a Mac, you can convert audio files into this format using the afconvert utility.
 *
 * @param path The path to the file.
 * @return An array of sample data or NULL if the file can't be opened
 *  or has an usupported format.
 */
lf_waveform_t* read_wave_file(const char* path);

#endif // WAVE_FILE_READER_H