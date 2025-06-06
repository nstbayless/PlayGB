//
//  utility.c
//  PlayGB
//
//  Created by Matteo D'Ignazio on 14/05/22.
//

#include "utility.h"

#include <stdlib.h>
#include <string.h>

#include "library_scene.h"

PlaydateAPI *playdate;

const char *PGB_savesPath = "saves";
const char *PGB_gamesPath = "games";
const char *PGB_coversPath = "covers";

/* clang-format off */
const clalign uint8_t PGB_patterns[4][4][4] = {
    {
        {1, 1, 1, 1},
        {1, 1, 1, 1},
        {1, 1, 1, 1},
        {1, 1, 1, 1}
    },
    {
        {0, 1, 1, 1},
        {1, 1, 0, 1},
        {0, 1, 1, 1},
        {1, 1, 0, 1}
    },
    {
        {0, 0, 0, 1},
        {0, 1, 0, 0},
        {0, 0, 0, 1},
        {0, 1, 0, 0}
    },
    {
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}
    }
};
/* clang-format on */

char *string_copy(const char *string)
{
    char *copied = pgb_malloc(strlen(string) + 1);
    strcpy(copied, string);
    return copied;
}

size_t pgb_strlen(const char *s)
{
    return strlen(s);
}

char *pgb_strrchr(const char *s, int c)
{
    return strrchr(s, c);
}

int pgb_strcmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

char *pgb_save_filename(const char *path, bool isRecovery)
{

    char *filename;

    char *slash = strrchr(path, '/');
    if (!slash)
    {
        filename = (char *)path;
    }
    else
    {
        filename = slash + 1;
    }

    size_t len;

    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
    {
        len = strlen(filename);
    }
    else
    {
        len = strlen(filename) - strlen(dot);
    }

    char *filenameNoExt = pgb_malloc(len + 1);
    strcpy(filenameNoExt, "");
    strncat(filenameNoExt, filename, len);

    char *suffix = "";
    if (isRecovery)
    {
        suffix = " (recovery)";
    }

    char *buffer;
    playdate->system->formatString(&buffer, "%s/%s%s.sav", PGB_savesPath,
                                   filenameNoExt, suffix);

    pgb_free(filenameNoExt);

    return buffer;
}

char *pgb_extract_fs_error_code(const char *fileError)
{
    char *findStr = "uC-FS error: ";
    char *fsErrorPtr = strstr(fileError, findStr);
    if (fsErrorPtr)
    {
        return fsErrorPtr + strlen(findStr);
    }
    return NULL;
}

float pgb_easeInOutQuad(float x)
{
    return (x < 0.5f) ? 2 * x * x : 1 - powf(-2 * x + 2, 2) * 0.5f;
}

int pgb_compare_games_by_display_name(const void *a, const void *b)
{
    PGB_Game *gameA = *(PGB_Game **)a;
    PGB_Game *gameB = *(PGB_Game **)b;

    return strcmp(gameA->displayName, gameB->displayName);
}

void pgb_sanitize_string_for_filename(char *str)
{
    if (str == NULL)
    {
        return;
    }
    char *p = str;
    while (*p)
    {
        if (*p == ' ' || *p == '(' || *p == ')' || *p == '[' || *p == ']' ||
            *p == '{' || *p == '}' || *p == '!' || *p == '?' || *p == ':' ||
            *p == ';' || *p == ',' || *p == '&' || *p == '\'')
        {
            *p = '_';
        }
        p++;
    }
}

void pgb_sort_games_array(PGB_Array *games_array)
{
    if (games_array != NULL && games_array->length > 1)
    {
        qsort(games_array->items, games_array->length, sizeof(PGB_Game *),
              pgb_compare_games_by_display_name);
    }
}

char *pgb_find_cover_art_path(const char *rom_basename_no_ext,
                              const char *rom_clean_basename_no_ext)
{
    char *found_path = NULL;
    FileStat fileStat;
    char *path_attempt = NULL;

    playdate->system->formatString(&path_attempt, "%s/%s.pdi", PGB_coversPath,
                                   rom_clean_basename_no_ext);
    if (path_attempt && playdate->file->stat(path_attempt, &fileStat) == 0)
    {
        found_path = path_attempt;
    }
    else
    {
        if (path_attempt)
        {
            pgb_free(path_attempt);
            path_attempt = NULL;
        }
        playdate->system->formatString(&path_attempt, "%s/%s.pdi",
                                       PGB_coversPath, rom_basename_no_ext);
        if (path_attempt && playdate->file->stat(path_attempt, &fileStat) == 0)
        {
            found_path = path_attempt;
        }
        else
        {
            if (path_attempt)
            {
                pgb_free(path_attempt);
            }
        }
    }
    return found_path;
}

PGB_LoadedCoverArt pgb_load_and_scale_cover_art_from_path(
    const char *cover_path, int max_target_width, int max_target_height)
{
    PGB_LoadedCoverArt result = {.bitmap = NULL,
                                 .original_width = 0,
                                 .original_height = 0,
                                 .scaled_width = 0,
                                 .scaled_height = 0,
                                 .status = PGB_COVER_ART_FILE_NOT_FOUND};

    if (!cover_path)
    {
        result.status = PGB_COVER_ART_FILE_NOT_FOUND;
        return result;
    }

    FileStat fileStatCheck;
    if (playdate->file->stat(cover_path, &fileStatCheck) != 0)
    {
        result.status = PGB_COVER_ART_FILE_NOT_FOUND;
        return result;
    }

    const char *error_str = NULL;
    LCDBitmap *original_image =
        playdate->graphics->loadBitmap(cover_path, &error_str);

    if (error_str)
    {
        playdate->system->logToConsole(
            "Error string from loadBitmap for %s: %s", cover_path, error_str);
        // FIXME: should we free this?
        //pgb_free(error_str);
    }

    if (original_image == NULL)
    {
        result.status = PGB_COVER_ART_ERROR_LOADING;
        playdate->system->logToConsole("Failed to load bitmap: %s", cover_path);
        return result;
    }

    playdate->graphics->getBitmapData(original_image, &result.original_width,
                                      &result.original_height, NULL, NULL,
                                      NULL);

    if (result.original_width <= 0 || result.original_height <= 0)
    {
        playdate->graphics->freeBitmap(original_image);
        result.status = PGB_COVER_ART_INVALID_IMAGE;
        playdate->system->logToConsole(
            "Invalid image dimensions (%dx%d) for: %s", result.original_width,
            result.original_height, cover_path);
        return result;
    }

    float scale;
    float scaleX = (float)max_target_width / result.original_width;
    float scaleY = (float)max_target_height / result.original_height;
    scale = (scaleX < scaleY) ? scaleX : scaleY;

    result.scaled_width = (int)roundf(result.original_width * scale);
    result.scaled_height = (int)roundf(result.original_height * scale);

    if (result.scaled_width < 1 && result.original_width > 0)
        result.scaled_width = 1;
    if (result.scaled_height < 1 && result.original_height > 0)
        result.scaled_height = 1;

    bool perform_scaling_operation = false;
    if (result.scaled_width != result.original_width ||
        result.scaled_height != result.original_height)
    {
        perform_scaling_operation = true;
    }

    if (perform_scaling_operation)
    {
        if (result.scaled_width <= 0 || result.scaled_height <= 0)
        {
            playdate->system->logToConsole(
                "Error: Calculated scaled dimensions are zero or negative "
                "(%dx%d) for %s. Original: %dx%d, Scale: %f",
                result.scaled_width, result.scaled_height, cover_path,
                result.original_width, result.original_height, (double)scale);
            playdate->graphics->freeBitmap(original_image);
            result.status = PGB_COVER_ART_INVALID_IMAGE;
            return result;
        }

        LCDBitmap *scaled_bitmap = playdate->graphics->newBitmap(
            result.scaled_width, result.scaled_height, kColorClear);
        if (scaled_bitmap == NULL)
        {
            playdate->graphics->freeBitmap(original_image);
            result.status = PGB_COVER_ART_ERROR_LOADING;
            playdate->system->logToConsole(
                "Failed to create new scaled bitmap (%dx%d) for: %s",
                result.scaled_width, result.scaled_height, cover_path);
            return result;
        }

        playdate->graphics->pushContext(scaled_bitmap);
        playdate->graphics->setDrawMode(kDrawModeCopy);
        playdate->graphics->drawScaledBitmap(original_image, 0, 0, scale,
                                             scale);
        playdate->graphics->popContext();

        playdate->graphics->freeBitmap(original_image);
        result.bitmap = scaled_bitmap;
    }
    else
    {
        result.bitmap = original_image;
    }

    result.status = PGB_COVER_ART_SUCCESS;
    return result;
}

void pgb_free_loaded_cover_art_bitmap(PGB_LoadedCoverArt *art_result)
{
    if (art_result && art_result->bitmap)
    {
        playdate->graphics->freeBitmap(art_result->bitmap);
        art_result->bitmap = NULL;
    }
}

void pgb_fillRoundRect(PDRect rect, int radius, LCDColor color)
{
    int r2 = radius * 2;

    playdate->graphics->fillRect(rect.x, rect.y + radius, radius,
                                 rect.height - r2, color);
    playdate->graphics->fillRect(rect.x + radius, rect.y, rect.width - r2,
                                 rect.height, color);
    playdate->graphics->fillRect(rect.x + rect.width - radius, rect.y + radius,
                                 radius, rect.height - r2, color);

    playdate->graphics->fillEllipse(rect.x, rect.y, r2, r2, -90, 0, color);
    playdate->graphics->fillEllipse(rect.x + rect.width - r2, rect.y, r2, r2, 0,
                                    90, color);
    playdate->graphics->fillEllipse(rect.x + rect.width - r2,
                                    rect.y + rect.height - r2, r2, r2, 90, 180,
                                    color);
    playdate->graphics->fillEllipse(rect.x, rect.y + rect.height - r2, r2, r2,
                                    -180, -90, color);
}

void pgb_drawRoundRect(PDRect rect, int radius, int lineWidth, LCDColor color)
{
    int r2 = radius * 2;

    playdate->graphics->fillRect(rect.x, rect.y + radius, lineWidth,
                                 rect.height - r2, color);
    playdate->graphics->fillRect(rect.x + radius, rect.y, rect.width - r2,
                                 lineWidth, color);
    playdate->graphics->fillRect(rect.x + rect.width - lineWidth,
                                 rect.y + radius, lineWidth, rect.height - r2,
                                 color);
    playdate->graphics->fillRect(rect.x + radius,
                                 rect.y + rect.height - lineWidth,
                                 rect.width - r2, lineWidth, color);

    playdate->graphics->drawEllipse(rect.x, rect.y, r2, r2, lineWidth, -90, 0,
                                    color);
    playdate->graphics->drawEllipse(rect.x + rect.width - r2, rect.y, r2, r2,
                                    lineWidth, 0, 90, color);
    playdate->graphics->drawEllipse(rect.x + rect.width - r2,
                                    rect.y + rect.height - r2, r2, r2,
                                    lineWidth, 90, 180, color);
    playdate->graphics->drawEllipse(rect.x, rect.y + rect.height - r2, r2, r2,
                                    lineWidth, -180, -90, color);
}

void *pgb_malloc(size_t size)
{
    return playdate->system->realloc(NULL, size);
}

void *pgb_realloc(void *ptr, size_t size)
{
    return playdate->system->realloc(ptr, size);
}

void *pgb_calloc(size_t count, size_t size)
{
    return memset(pgb_malloc(count * size), 0, count * size);
}

void pgb_free(void *ptr)
{
    if (ptr)
    {
        playdate->system->realloc(ptr, 0);
    }
}
