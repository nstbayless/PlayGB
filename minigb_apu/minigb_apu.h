/**
 * minigb_apu is released under the terms listed within the LICENSE file.
 *
 * minigb_apu emulates the audio processing unit (APU) of the Game Boy. This
 * project is based on MiniGBS by Alex Baines: https://github.com/baines/MiniGBS
 */

#pragma once

#include <stdint.h>

// increasing AUDIO_SAMPLE_REPLICATION saves processing time,
// but lowers audio quality. (duplicates, triplicates samples, etc.)
// does not need to be a power of 2.
// 2 seems to be a good middle ground between perf and audio quality.
#define AUDIO_SAMPLE_REPLICATION 2
#define AUDIO_SAMPLE_RATE (44100 / AUDIO_SAMPLE_REPLICATION)

#define DMG_CLOCK_FREQ 4194304.0
#define SCREEN_REFRESH_CYCLES 70224.0
#define VERTICAL_SYNC (DMG_CLOCK_FREQ / SCREEN_REFRESH_CYCLES)

#define AUDIO_SAMPLES ((unsigned)(AUDIO_SAMPLE_RATE / VERTICAL_SYNC))

// master audio control
extern int audio_enabled;

/**
 * Read audio register at given address "addr".
 */
uint8_t audio_read(const uint16_t addr);

/**
 * Write "val" to audio register at given address "addr".
 */
void audio_write(const uint16_t addr, const uint8_t val);

/**
 * Initialise audio driver.
 */
void audio_init(uint8_t *audio_mem);

/**
 * Playdate audio callback function.
 */
int audio_callback(void *context, int16_t *left, int16_t *right, int len);
