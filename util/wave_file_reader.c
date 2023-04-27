/**
 * @file
 * @author Edward A. Lee
 * @copyright (c) 2020-2023, The University of California at Berkeley
 * License in [BSD 2-clause](https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md)
 * @brief Utility function for reading WAV audio files.
 * 
 * See wave_file_reader.h for instructions.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "wave_file_reader.h"

#if _WIN32 || WIN32
#define FILE_PATH_SEPARATOR '\\';
#else
#define FILE_PATH_SEPARATOR '/';
#endif

/**
 * The top-level 'chunk' of a .wav file is a 'RIFF'
 * chunk, identified by an ID that is an int formed
 * by the sequence of four chars 'RIFF'.
 */
typedef struct {
    char chunk_id[4];    // "RIFF"
    uint32_t chunk_size; // 36 + subchunk_size
    char format[4];      // "WAVE"
} lf_wav_riff_t;

/**
 * The first subchunk within a .wav file is a 'fmt '
 * chunk, identified by an ID that is an int formed
 * by the sequence of four chars 'fmt '.
 */
typedef struct {
    char subchunk_id[4];    // 'fmt '
    uint32_t subchunk_size; // 16 for linear PCM.
    uint16_t audio_format;  // 1 for linear PCM
    uint16_t num_channels;  // 1 for mono = 1, 2 for stereo, etc.
    uint32_t sample_rate;   // 44100
    uint32_t byte_rate;     // sample_rate * num_channels * bits_per_sample/8
    uint16_t BlockAlign;    /* = num_channels * bits_per_sample/8 */
    uint16_t bits_per_sample; /* 8bits, 16bits, etc. */
} lf_wav_format_t;

/**
 * Header for the subchunk containing the data.
 * This is a 'data' chunk, identified by an ID that
 * is an int formed by the sequenced of four chars 'data'.
 */
typedef struct {
    char subchunk_id[4];    // 'data'
    uint32_t subchunk_size; // data size in bytes
} lf_wav_data_t;

/**
 * Overall wave data.
 */
typedef struct {
   lf_wav_riff_t riff;
   lf_wav_format_t fmt;
   lf_wav_data_t data;
} lf_wav_t;

lf_waveform_t* read_wave_file(const char* path) {
    FILE *fp = NULL;
    
    lf_wav_t wav;
    fp = fopen(path, "rb");
    if (!fp) {
        // Try prefixing the file name with "src-gen".
        // On a remote host, the waveform files will be put in that directory.
        char alt_path[strlen(path) + 9];
        strcpy(alt_path, "src-gen");
        alt_path[7] = FILE_PATH_SEPARATOR;
        strcpy(&(alt_path[8]), path);
        fp = fopen(alt_path, "rb");
        if (!fp) {
            fprintf(stderr, "WARNING: Failed to open waveform sample file: %s\n", path);
            return NULL;
        }
    }
 
    fread(&wav, 1, sizeof(lf_wav_t), fp);
     
    lf_wav_format_t fmt = wav.fmt;
    lf_wav_data_t data = wav.data;
 
    // Wave file format is described here:
    // https://sites.google.com/site/musicgapi/technical-documents/wav-file-format
    uint32_t expected_chunk_id = (uint32_t)'FFIR';     // Little-endian version of RIFF.
    uint32_t expected_format = (uint32_t)'EVAW';       // Little-endian version of WAVE.
    uint32_t expected_subchunk_id = (uint32_t)' tmf';  // Little-endian version of 'fmt '.
    if (*(uint32_t*)wav.riff.chunk_id != expected_chunk_id
        || *(uint32_t*)wav.riff.format != expected_format
        || *(uint32_t*)fmt.subchunk_id != expected_subchunk_id
        || fmt.subchunk_size != 16
        || fmt.audio_format != 1
        || fmt.sample_rate != 44100
        || fmt.bits_per_sample != 16
    ) {
        fprintf(stderr, "WARNING: Waveform sample not a supported format.\n");
        fprintf(stderr, "Chunk ID was expected to be 'RIFF'. Got: '%c%c%c%c'.\n",
                wav.riff.chunk_id[0], wav.riff.chunk_id[1], wav.riff.chunk_id[2], wav.riff.chunk_id[3]);
        fprintf(stderr, "Format was expected to be 'WAVE'. Got: '%c%c%c%c'.\n",
                wav.riff.format[0], wav.riff.format[1], wav.riff.format[2], wav.riff.format[3]);
        fprintf(stderr, "Subchunk ID was expected to be 'fmt '. Got: '%c%c%c%c'.\n",
                fmt.subchunk_id[0], fmt.subchunk_id[1], fmt.subchunk_id[2], fmt.subchunk_id[3]);
        fprintf(stderr, "Subchunk size was expected to be 16. Got: '%d'.\n",
                fmt.subchunk_size);
        fprintf(stderr, "Audio format was expected to be 1 (LPCM, no compression). Got: '%d'.\n",
                fmt.audio_format);
        fprintf(stderr, "Sample rate was expected to be 44100). Got: '%d'.\n",
                fmt.sample_rate);
        fprintf(stderr, "Bits per sample was expected to be 16. Got: '%d'.\n",
                fmt.bits_per_sample);
    }
    // Ignore any intermediate chunks that are not 'data' chunks.
    // Apparently, Apple software sometimes inserts junk here.
    uint32_t expected_data_id = (uint32_t)'atad';      // Little-endian version of 'data'.
    while (*(uint32_t*)data.subchunk_id != expected_data_id) {
        char junk[data.subchunk_size];
        size_t bytes_read = fread(junk, 1, data.subchunk_size , fp);
        if (bytes_read != data.subchunk_size) {
            fprintf(stderr, "Intermediate junk chunk '%c%c%c%c' could not be read. Giving up.\n",
                data.subchunk_id[0], data.subchunk_id[1], data.subchunk_id[2], data.subchunk_id[3]);
            break;
        }
        bytes_read = fread(&data, 1, sizeof(lf_wav_data_t) , fp);
        if (bytes_read != sizeof(lf_wav_data_t)) {
            fprintf(stderr, "Missing 'data' chunk in file %s.\n", path);
            break;
        }
    }

    uint16_t num_channels = fmt.num_channels;     

    // Ignoring the following fields. Should we?
    // printf("byte_rate \t%d\n", fmt.byte_rate);
    // printf("BlockAlign \t%d\n", fmt.BlockAlign);

    // printf("Data subchunk size \t%d\n", data.subchunk_size);

    lf_waveform_t* result = (lf_waveform_t*)malloc(sizeof(lf_waveform_t));
    // printf("Size of lf_waveform_t %d", sizeof(lf_waveform_t));
    result->length = data.subchunk_size/2; // Subchunk size is in bytes, but length is number of samples.
    result->num_channels = num_channels;
    result->waveform = (int16_t*)calloc(data.subchunk_size/2, sizeof(int16_t));

    size_t bytes_read = fread(result->waveform, sizeof(int16_t), data.subchunk_size/2 , fp);
    if (bytes_read != data.subchunk_size/2) {
        fprintf(stderr, "WARNING: Expected %d bytes, but got %zu.\n", data.subchunk_size, bytes_read);
    }

    // printf("duration \t%f\n", (data.subchunk_size * 1.0) / fmt.byte_rate);
    return result;
 }
