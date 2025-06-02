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

char *string_copy(const char *string);

char *pgb_save_filename(const char *filename, bool isRecovery);
char *pgb_extract_fs_error_code(const char *filename);

float pgb_easeInOutQuad(float x);

int pgb_compare_games_by_display_name(const void *a, const void *b);

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

#endif /* utility_h */
