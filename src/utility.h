//
//  utility.h
//  PlayGB
//
//  Created by Matteo D'Ignazio on 14/05/22.
//

#ifndef utility_h
#define utility_h

#include <stdbool.h>
#include <stdio.h>

#include "array.h"
#include "pd_api.h"

extern PlaydateAPI *playdate;

#define PGB_DEBUG false
#define PGB_DEBUG_UPDATED_ROWS false

#define PGB_LCD_WIDTH 320
#define PGB_LCD_HEIGHT 240
#define PGB_LCD_ROWSIZE 40

#define PGB_LCD_X 32  // multiple of 8
#define PGB_LCD_Y 0

#define PGB_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define PGB_MIN(x, y) (((x) < (y)) ? (x) : (y))

extern const uint8_t PGB_patterns[4][4][4];

extern const char *PGB_savesPath;
extern const char *PGB_gamesPath;
extern const char *PGB_coversPath;

typedef enum
{
    PGB_COVER_ART_SUCCESS,
    PGB_COVER_ART_ERROR_LOADING,
    PGB_COVER_ART_INVALID_IMAGE,
    PGB_COVER_ART_FILE_NOT_FOUND
} PGB_CoverArtStatus;

typedef struct
{
    LCDBitmap *bitmap;
    int original_width;
    int original_height;
    int scaled_width;
    int scaled_height;
    PGB_CoverArtStatus status;
} PGB_LoadedCoverArt;

char *string_copy(const char *string);

char *pgb_save_filename(const char *filename, bool isRecovery);
char *pgb_extract_fs_error_code(const char *filename);

float pgb_easeInOutQuad(float x);

int pgb_compare_games_by_display_name(const void *a, const void *b);

void pgb_sanitize_string_for_filename(char *str);
void pgb_sort_games_array(PGB_Array *games_array);
void pgb_fillRoundRect(PDRect rect, int radius, LCDColor color);
void pgb_drawRoundRect(PDRect rect, int radius, int lineWidth, LCDColor color);

void *pgb_malloc(size_t size);
void *pgb_realloc(void *ptr, size_t size);
void *pgb_calloc(size_t count, size_t size);
void pgb_free(void *ptr);

size_t pgb_strlen(const char *s);
char *pgb_strrchr(const char *s, int c);
int pgb_strcmp(const char *s1, const char *s2);

char *pgb_find_cover_art_path(const char *rom_basename_no_ext,
                              const char *rom_clean_basename_no_ext);

PGB_LoadedCoverArt pgb_load_and_scale_cover_art_from_path(
    const char *cover_path, int max_target_width, int max_target_height);

void pgb_free_loaded_cover_art_bitmap(PGB_LoadedCoverArt *art_result);

#ifdef TARGET_PLAYDATE
#define __section__(x) __attribute__((section(x)))
#else
#define __section__(x)
#endif

#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

#ifdef TARGET_SIMULATOR
#define clalign
#else
#define clalign __attribute__((aligned(32)))
#endif

#ifdef TARGET_SIMULATOR
#define CPU_VALIDATE 1
#define PGB_ASSERT(x) \
    if (!(x))         \
        playdate->system->error("ASSERTION FAILED: %s", #x);
#else
#define PGB_ASSERT(x)
#endif

#endif /* utility_h */
