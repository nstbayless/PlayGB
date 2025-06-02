//
//  game_scene.c
//  PlayGB
//
//  Created by Matteo D'Ignazio on 14/05/22.
//

#include "game_scene.h"

#include "../minigb_apu/minigb_apu.h"
#include "../peanut_gb/peanut_gb.h"
#include "app.h"
#include "dtcm.h"
#include "preferences.h"
#include "revcheck.h"
#include "utility.h"

static const float TARGET_TIME_PER_GB_FRAME_MS = 1000.0f / 59.73f;
static const float PERFORMANCE_SKIP_OVERRIDE_FACTOR = 0.95f;
static uint8_t MAX_CONSECUTIVE_DRAW_SKIPS = 0;
static const uint8_t ADJUSTMENT_PERIOD_FRAMES = 60;

PGB_GameScene *audioGameScene = NULL;

typedef struct PGB_GameSceneContext
{
    PGB_GameScene *scene;
    struct gb_s *gb;
    uint8_t wram[WRAM_SIZE];
    uint8_t vram[VRAM_SIZE];
    uint8_t *rom;
    uint8_t *cart_ram;
    uint8_t
        previous_lcd[LCD_HEIGHT *
                     LCD_WIDTH_PACKED];  // Buffer for the previous frame's LCD
    bool line_has_changed[LCD_HEIGHT];   // Flags for changed lines

    uint8_t frames_to_skip_drawing;
    uint8_t frames_actually_skipped_drawing;
    uint8_t frames_emulated_since_adjustment;
    float total_time_for_adjustment_period_ms;
} PGB_GameSceneContext;

static void PGB_GameScene_selector_init(PGB_GameScene *gameScene);
static void PGB_GameScene_update(void *object);
static void PGB_GameScene_menu(void *object);
static void PGB_GameScene_saveGame(PGB_GameScene *gameScene);
static void PGB_GameScene_generateBitmask(void);
static void PGB_GameScene_free(void *object);

static uint8_t *read_rom_to_ram(const char *filename,
                                PGB_GameSceneError *sceneError);

static void read_cart_ram_file(const char *save_filename, uint8_t **dest,
                               const size_t len);
static void write_cart_ram_file(const char *save_filename, uint8_t **dest,
                                const size_t len);

static void gb_error(struct gb_s *gb, const enum gb_error_e gb_err,
                     const uint16_t val);

static const char *startButtonText = "start";
static const char *selectButtonText = "select";

static const uint16_t PGB_dither_lut_c0 =
    0 | (0b1111 << 0) | (0b0111 << 4) | (0b0001 << 8) | (0b0000 << 12);

static const uint16_t PGB_dither_lut_c1 =
    0 | (0b1111 << 0) | (0b1101 << 4) | (0b0100 << 8) | (0b0000 << 12);

static uint8_t PGB_bitmask[4][4][4];
static bool PGB_GameScene_bitmask_done = false;

#if ITCM_CORE
void *core_itcm_reloc = NULL;

__section__(".rare") void itcm_core_init(void)
{
    if (core_itcm_reloc == (void *)&__itcm_start)
        core_itcm_reloc = NULL;

    if (core_itcm_reloc != NULL)
        return;

    if (!dtcm_enabled())
    {
        // just use original non-relocated code
        core_itcm_reloc = (void *)&__itcm_start;
        playdate->system->logToConsole("itcm_core_init but dtcm not enabled");
        return;
    }

    // make region to copy instructions to; ensure it has same cache alignment
    core_itcm_reloc =
        dtcm_alloc_aligned(itcm_core_size, (uintptr_t)&__itcm_start);
    memcpy(core_itcm_reloc, __itcm_start, itcm_core_size);
    playdate->system->logToConsole("itcm start: %x, end %x: run_frame: %x",
                                   &__itcm_start, &__itcm_end, &gb_run_frame);
    playdate->system->logToConsole("core is 0x%X bytes, relocated at 0x%X",
                                   itcm_core_size, core_itcm_reloc);
    playdate->system->clearICache();
}
#else
void itcm_core_init(void)
{
}
#endif

PGB_GameScene *PGB_GameScene_new(const char *rom_filename)
{
    playdate->system->setCrankSoundsDisabled(true);

    PGB_Scene *scene = PGB_Scene_new();

    PGB_GameScene *gameScene = pgb_malloc(sizeof(PGB_GameScene));
    gameScene->scene = scene;
    scene->managedObject = gameScene;

    scene->update = PGB_GameScene_update;
    scene->menu = PGB_GameScene_menu;
    scene->free = PGB_GameScene_free;

    scene->preferredRefreshRate = 30;

    gameScene->rtc_time = playdate->system->getSecondsSinceEpoch(NULL);
    gameScene->rtc_seconds_to_catch_up = 0;
    gameScene->rtc_fractional_second_accumulator = 0.0f;

    gameScene->rom_filename = string_copy(rom_filename);
    gameScene->save_filename = NULL;

    gameScene->state = PGB_GameSceneStateError;
    gameScene->error = PGB_GameSceneErrorUndefined;

    gameScene->model =
        (PGB_GameSceneModel){.state = PGB_GameSceneStateError,
                             .error = PGB_GameSceneErrorUndefined,
                             .selectorIndex = 0,
                             .empty = true};

    gameScene->audioEnabled = preferences_sound_enabled;
    gameScene->audioLocked = false;

    gameScene->staticSelectorUIDrawn = false;

    PGB_GameScene_generateBitmask();

    PGB_GameScene_selector_init(gameScene);

#if PGB_DEBUG && PGB_DEBUG_UPDATED_ROWS
    int highlightWidth = 10;
    gameScene->debug_highlightFrame =
        PDRectMake(PGB_LCD_X - 1 - highlightWidth, 0, highlightWidth,
                   playdate->display->getHeight());
#endif

    // we don't use the DTCM trick on REV_B, because it's untested
    // FIXME
    if (pd_rev == PD_REV_A)
        dtcm_init();

    PGB_GameSceneContext *context = pgb_malloc(sizeof(PGB_GameSceneContext));
    static struct gb_s *gb = NULL;
    static struct gb_s
        gb_fallback;  // use this gb struct if dtcm alloc not available
    if (dtcm_enabled())
    {
        if (gb == NULL || gb == &gb_fallback)
            gb = dtcm_alloc(sizeof(struct gb_s));
    }
    else
    {
        gb = &gb_fallback;
    }
    itcm_core_init();

    if (PGB_App->soundSource == NULL)
    {
        PGB_App->soundSource =
            playdate->sound->addSource(audio_callback, &audioGameScene, 1);
    }
    context->gb = gb;
    context->scene = gameScene;
    context->rom = NULL;
    context->cart_ram = NULL;

    context->frames_to_skip_drawing = 0;
    context->frames_actually_skipped_drawing = 0;
    context->frames_emulated_since_adjustment = 0;
    context->total_time_for_adjustment_period_ms = 0.0f;

    gameScene->context = context;

    PGB_GameSceneError romError;
    uint8_t *rom = read_rom_to_ram(rom_filename, &romError);
    if (rom)
    {
        context->rom = rom;

        static uint8_t lcd[LCD_HEIGHT * LCD_WIDTH_PACKED * 2];

        enum gb_init_error_e gb_ret =
            gb_init(context->gb, context->wram, context->vram, lcd, rom,
                    gb_error, context);

        if (gb_ret == GB_INIT_NO_ERROR)
        {
            playdate->system->logToConsole(
                    "Cartridge Type 0x%02X (MBC %d)",
                    gb->cartridge_type, (int)gb->mbc);
            char *save_filename = pgb_save_filename(rom_filename, false);
            gameScene->save_filename = save_filename;

            read_cart_ram_file(save_filename, &context->cart_ram,
                               gb_get_save_size(context->gb));

            context->gb->gb_cart_ram = context->cart_ram;
            context->gb->gb_cart_ram_size = gb_get_save_size(context->gb);

            if (context->gb->mbc_flags.rtc)
            {
                gameScene->cartridge_has_rtc = true;
                playdate->system->logToConsole("RTC enabled");

                // Initialize Playdate-side RTC tracking variables
                gameScene->rtc_time =
                    playdate->system->getSecondsSinceEpoch(NULL);
                gameScene->rtc_seconds_to_catch_up = 0;
                gameScene->rtc_fractional_second_accumulator = 0.0f;

                // Set the GB core's RTC
                time_t time_for_core =
                    gameScene->rtc_time + 946684800;  // Unix epoch
                struct tm *timeinfo = localtime(&time_for_core);
                if (timeinfo != NULL)
                {
                    gb_set_rtc(context->gb, timeinfo);
                }
                else
                {
                    playdate->system->logToConsole(
                        "Error: localtime() failed during RTC setup.");
                }
            }
            else
            {
                gameScene->cartridge_has_rtc = false;
            }

            audio_init(gb->hram + 0x10);
            if (gameScene->audioEnabled)
            {
                // init audio
                playdate->sound->channel->setVolume(
                    playdate->sound->getDefaultChannel(), 0.2f);

                context->gb->direct.sound = 1;
                audioGameScene = gameScene;
            }

            // init lcd
            gb_init_lcd(context->gb);

            // Initialize previous_lcd, for simplicity, let's zero it.
            // This means the first frame will draw everything.
            memset(context->previous_lcd, 0, sizeof(context->previous_lcd));

            // Mark all lines as changed for the first frame render
            for (int i = 0; i < LCD_HEIGHT; i++)
            {
                context->line_has_changed[i] = true;
            }

            context->gb->direct.frame_skip = preferences_frame_skip ? 1 : 0;

            // set game state to loaded
            gameScene->state = PGB_GameSceneStateLoaded;
        }
        else
        {
            gameScene->state = PGB_GameSceneStateError;
            gameScene->error = PGB_GameSceneErrorFatal;

            playdate->system->logToConsole(
                "%s:%i: Error initializing gb context", __FILE__, __LINE__);
        }
    }
    else
    {
        gameScene->state = PGB_GameSceneStateError;
        gameScene->error = romError;
    }

    return gameScene;
}

static void PGB_GameScene_selector_init(PGB_GameScene *gameScene)
{
    int startButtonWidth = playdate->graphics->getTextWidth(
        PGB_App->labelFont, startButtonText, strlen(startButtonText),
        kUTF8Encoding, 0);
    int selectButtonWidth = playdate->graphics->getTextWidth(
        PGB_App->labelFont, selectButtonText, strlen(selectButtonText),
        kUTF8Encoding, 0);

    int width = 18;
    int height = 46;

    int startSpacing = 3;
    int selectSpacing = 6;

    int labelHeight = playdate->graphics->getFontHeight(PGB_App->labelFont);

    int containerHeight =
        labelHeight + startSpacing + height + selectSpacing + labelHeight;
    int containerWidth = width;

    containerWidth = PGB_MAX(containerWidth, startButtonWidth);
    containerWidth = PGB_MAX(containerWidth, selectButtonWidth);

    int containerX = playdate->display->getWidth() - 6 - containerWidth;
    int containerY = 8;

    int x = containerX + (float)(containerWidth - width) / 2;
    int y = containerY + labelHeight + startSpacing;

    int startButtonX =
        containerX + (float)(containerWidth - startButtonWidth) / 2;
    int startButtonY = containerY;

    int selectButtonX =
        containerX + (float)(containerWidth - selectButtonWidth) / 2;
    int selectButtonY = containerY + containerHeight - labelHeight;

    gameScene->selector.x = x;
    gameScene->selector.y = y;
    gameScene->selector.width = width;
    gameScene->selector.height = height;
    gameScene->selector.containerX = containerX;
    gameScene->selector.containerY = containerY;
    gameScene->selector.containerWidth = containerWidth;
    gameScene->selector.containerHeight = containerHeight;
    gameScene->selector.startButtonX = startButtonX;
    gameScene->selector.startButtonY = startButtonY;
    gameScene->selector.selectButtonX = selectButtonX;
    gameScene->selector.selectButtonY = selectButtonY;
    gameScene->selector.numberOfFrames = 27;
    gameScene->selector.triggerAngle = 15;
    gameScene->selector.deadAngle = 20;
    gameScene->selector.index = 0;
    gameScene->selector.startPressed = false;
    gameScene->selector.selectPressed = false;
}

/**
 * Returns a pointer to the allocated space containing the ROM. Must be freed.
 */
static uint8_t *read_rom_to_ram(const char *filename,
                                PGB_GameSceneError *sceneError)
{
    *sceneError = PGB_GameSceneErrorUndefined;

    SDFile *rom_file = playdate->file->open(filename, kFileReadData);

    if (rom_file == NULL)
    {
        const char *fileError = playdate->file->geterr();
        playdate->system->logToConsole("%s:%i: Can't open rom file %s",
                                       __FILE__, __LINE__, filename);
        playdate->system->logToConsole("%s:%i: File error %s", __FILE__,
                                       __LINE__, fileError);

        *sceneError = PGB_GameSceneErrorLoadingRom;

        if (fileError)
        {
            char *fsErrorCode = pgb_extract_fs_error_code(fileError);
            if (fsErrorCode)
            {
                if (strcmp(fsErrorCode, "0709") == 0)
                {
                    *sceneError = PGB_GameSceneErrorWrongLocation;
                }
            }
        }
        return NULL;
    }

    playdate->file->seek(rom_file, 0, SEEK_END);
    int rom_size = playdate->file->tell(rom_file);
    playdate->file->seek(rom_file, 0, SEEK_SET);

    uint8_t *rom = pgb_malloc(rom_size);

    if (playdate->file->read(rom_file, rom, rom_size) != rom_size)
    {
        playdate->system->logToConsole("%s:%i: Can't read rom file %s",
                                       __FILE__, __LINE__, filename);

        pgb_free(rom);
        playdate->file->close(rom_file);
        *sceneError = PGB_GameSceneErrorLoadingRom;
        return NULL;
    }

    playdate->file->close(rom_file);
    return rom;
}

static void read_cart_ram_file(const char *save_filename, uint8_t **dest,
                               const size_t len)
{

    /* If save file not required. */
    if (len == 0)
    {
        *dest = NULL;
        return;
    }

    /* Allocate enough memory to hold save file. */
    if ((*dest = pgb_malloc(len)) == NULL)
    {
        playdate->system->logToConsole("%s:%i: Error allocating save file %s",
                                       __FILE__, __LINE__, save_filename);
    }

    SDFile *f = playdate->file->open(save_filename, kFileReadData);

    /* It doesn't matter if the save file doesn't exist. We initialise the
     * save memory allocated above. The save file will be created on exit. */
    if (f == NULL)
    {
        memset(*dest, 0, len);
        return;
    }

    /* Read save file to allocated memory. */
    playdate->file->read(f, *dest, (unsigned int)len);
    playdate->file->close(f);
}

static void write_cart_ram_file(const char *save_filename, uint8_t **dest,
                                const size_t len)
{
    if (len == 0 || *dest == NULL)
    {
        return;
    }

    SDFile *f = playdate->file->open(save_filename, kFileWrite);

    if (f == NULL)
    {
        playdate->system->logToConsole("%s:%i: Can't write save file", __FILE__,
                                       __LINE__, save_filename);
        return;
    }

    /* Record save file. */
    playdate->file->write(f, *dest, (unsigned int)(len * sizeof(uint8_t)));
    playdate->file->close(f);
}

/**
 * Handles an error reported by the emulator. The emulator context may be used
 * to better understand why the error given in gb_err was reported.
 */
static void gb_error(struct gb_s *gb, const enum gb_error_e gb_err,
                     const uint16_t val)
{
    PGB_GameSceneContext *context = gb->direct.priv;

    bool is_fatal = false;

    if (gb_err == GB_INVALID_OPCODE)
    {
        is_fatal = true;

        playdate->system->logToConsole(
            "%s:%i: Invalid opcode %#04x at PC: %#06x, SP: %#06x", __FILE__,
            __LINE__, val, gb->cpu_reg.pc - 1, gb->cpu_reg.sp);
    }
    else if (gb_err == GB_INVALID_READ)
    {
        playdate->system->logToConsole("ERROR: Invalid read, addr %04x", val);
    }
    else if (gb_err == GB_INVALID_WRITE)
    {
        playdate->system->logToConsole("ERROR: Invalid write, addr %04x", val);
    }
    else
    {
        is_fatal = true;
        playdate->system->logToConsole("%s:%i: Unknown error occurred",
                                       __FILE__, __LINE__);
    }

    if (is_fatal)
    {
        // write recovery .sav
        char *recovery_filename =
            pgb_save_filename(context->scene->rom_filename, true);
        write_cart_ram_file(recovery_filename, &context->gb->gb_cart_ram,
                            gb_get_save_size(gb));

        pgb_free(recovery_filename);

        context->scene->state = PGB_GameSceneStateError;
        context->scene->error = PGB_GameSceneErrorFatal;

        PGB_Scene_refreshMenu(context->scene->scene);
    }

    return;
}

typedef typeof(playdate->graphics->markUpdatedRows) markUpdateRows_t;

__core_section("fb") void update_fb_dirty_lines(
    uint8_t *restrict framebuffer, uint8_t *restrict lcd,
    const bool *restrict line_changed_flags, markUpdateRows_t markUpdateRows)
{
    framebuffer += (PGB_LCD_X / 8);
    // const u32 dither = 0b00011111 | (0b00001011 << 8);
    int scale_index = 0;
    unsigned fb_y_playdate_current_bottom =
        PGB_LCD_Y + PGB_LCD_HEIGHT;  // Bottom of drawable area on Playdate

    uint32_t dither_lut =
        PGB_dither_lut_c0 | ((uint32_t)PGB_dither_lut_c1 << 16);

    for (int y_gb = LCD_HEIGHT;
         y_gb-- > 0;)  // y_gb is Game Boy line index from top, 143 down to 0
    {
        int row_height_on_playdate = 2;
        if (scale_index++ == 2)
        {
            scale_index = 0;
            row_height_on_playdate = 1;

            // swap dither pattern on each half-row;
            // yields smoother results
            dither_lut = (dither_lut >> 16) | (dither_lut << 16);
        }

        // Calculate the Playdate Y position for the *top* of the current GB
        // line's representation
        unsigned int current_line_pd_top_y =
            fb_y_playdate_current_bottom - row_height_on_playdate;

        if (!line_changed_flags[y_gb])
        {
            // If line not changed, just update the bottom for the next line
            fb_y_playdate_current_bottom -= row_height_on_playdate;
            continue;  // Skip drawing
        }

        // Line has changed, draw it
        fb_y_playdate_current_bottom -=
            row_height_on_playdate;  // Update bottom for this drawn line

        uint8_t *restrict gb_line_data = &lcd[y_gb * LCD_WIDTH_PACKED];
        uint8_t *restrict pd_fb_line_top_ptr =
            &framebuffer[current_line_pd_top_y * PLAYDATE_ROW_STRIDE];

        for (int x_packed_gb = LCD_WIDTH_PACKED; x_packed_gb-- > 0;)
        {
            uint8_t orgpixels = gb_line_data[x_packed_gb];
            uint8_t pixels_temp_c0 = orgpixels;
            unsigned p = 0;

#pragma GCC unroll 4
            for (int i = 0; i < 4; ++i)
            {  // Unpack 4 GB pixels from the byte
                p <<= 2;
                unsigned c0h = dither_lut >> ((pixels_temp_c0 & 3) * 4);
                unsigned c0 = (c0h >> ((i * 2) % 4)) & 3;
                p |= c0;
                pixels_temp_c0 >>= 2;
            }

            u8 *restrict pd_fb_target_byte0 = pd_fb_line_top_ptr + x_packed_gb;
            *pd_fb_target_byte0 = p & 0xFF;

            if (row_height_on_playdate == 2)
            {
                uint8_t pixels_temp_c1 =
                    orgpixels;  // Reset for second dither pattern
                u8 *restrict pd_fb_target_byte1 =
                    pd_fb_target_byte0 +
                    PLAYDATE_ROW_STRIDE;  // Next Playdate row
                p = 0;  // Reset p for the second row calculation

// FIXME: why does this pragma cause a crash if unroll 4??
#pragma GCC unroll 2
                for (int i = 0; i < 4; ++i)
                {
                    p <<= 2;
                    unsigned c1h =
                        dither_lut >> ((pixels_temp_c1 & 3) * 4 + 16);
                    unsigned c1 = (c1h >> ((i * 2) % 4)) & 3;
                    p |= c1;
                    pixels_temp_c1 >>= 2;
                }
                *pd_fb_target_byte1 = p & 0xFF;
            }
        }
        markUpdateRows(current_line_pd_top_y,
                       current_line_pd_top_y + row_height_on_playdate - 1);
    }
}

__section__(".text.tick") __space static void PGB_GameScene_update(void *object)
{
    PGB_GameScene *gameScene = object;
    PGB_GameSceneContext *context = gameScene->context;

    float actual_delta_time_ms = PGB_App->dt * 1000.0f;
    if (gameScene->state == PGB_GameSceneStateLoaded)
    {  // Only accumulate if game is running
        context->total_time_for_adjustment_period_ms += actual_delta_time_ms;
        context->frames_emulated_since_adjustment++;
    }

    PGB_Scene_update(gameScene->scene);

    float progress = 0.5f;

    gameScene->selector.startPressed = false;
    gameScene->selector.selectPressed = false;

    if (!playdate->system->isCrankDocked())
    {
        float angle = fmaxf(0, fminf(360, playdate->system->getCrankAngle()));

        if (angle <= (180 - gameScene->selector.deadAngle))
        {
            if (angle >= gameScene->selector.triggerAngle)
            {
                gameScene->selector.startPressed = true;
            }

            float adjustedAngle =
                fminf(angle, gameScene->selector.triggerAngle);
            progress =
                0.5f - adjustedAngle / gameScene->selector.triggerAngle * 0.5f;
        }
        else if (angle >= (180 + gameScene->selector.deadAngle))
        {
            if (angle <= (360 - gameScene->selector.triggerAngle))
            {
                gameScene->selector.selectPressed = true;
            }

            float adjustedAngle =
                fminf(360 - angle, gameScene->selector.triggerAngle);
            progress =
                0.5f + adjustedAngle / gameScene->selector.triggerAngle * 0.5f;
        }
        else
        {
            gameScene->selector.startPressed = true;
            gameScene->selector.selectPressed = true;
        }
    }

    int selectorIndex;

    if (gameScene->selector.startPressed && gameScene->selector.selectPressed)
    {
        selectorIndex = -1;
    }
    else
    {
        selectorIndex =
            1 + roundf(progress * (gameScene->selector.numberOfFrames - 2));

        if (progress == 0)
        {
            selectorIndex = 0;
        }
        else if (progress == 1)
        {
            selectorIndex = gameScene->selector.numberOfFrames - 1;
        }
    }

    gameScene->selector.index = selectorIndex;

    bool gbScreenRequiresFullRefresh = false;
    if (gameScene->model.empty || gameScene->model.state != gameScene->state ||
        gameScene->model.error != gameScene->error)
    {
        gbScreenRequiresFullRefresh = true;
    }

    bool animatedSelectorBitmapNeedsRedraw = false;
    if (gbScreenRequiresFullRefresh || !gameScene->staticSelectorUIDrawn ||
        gameScene->model.selectorIndex != gameScene->selector.index)
    {
        animatedSelectorBitmapNeedsRedraw = true;
    }

    gameScene->model.empty = false;
    gameScene->model.state = gameScene->state;
    gameScene->model.error = gameScene->error;
    gameScene->model.selectorIndex = gameScene->selector.index;

    if (gameScene->state == PGB_GameSceneStateLoaded)
    {
        PGB_GameSceneContext *context = gameScene->context;

        PDButtons current;
        playdate->system->getButtonState(&current, NULL, NULL);

        context->gb->direct.joypad_bits.start =
            !(gameScene->selector.startPressed);
        context->gb->direct.joypad_bits.select =
            !(gameScene->selector.selectPressed);

        context->gb->direct.joypad_bits.a = !(current & kButtonA);
        context->gb->direct.joypad_bits.b = !(current & kButtonB);
        context->gb->direct.joypad_bits.left = !(current & kButtonLeft);
        context->gb->direct.joypad_bits.up = !(current & kButtonUp);
        context->gb->direct.joypad_bits.right = !(current & kButtonRight);
        context->gb->direct.joypad_bits.down = !(current & kButtonDown);

        if (gbScreenRequiresFullRefresh)
        {
            playdate->graphics->clear(kColorBlack);
        }

#if PGB_DEBUG && PGB_DEBUG_UPDATED_ROWS
        memset(gameScene->debug_updatedRows, 0, LCD_ROWS);
#endif

#ifdef DTCM_ALLOC
        ITCM_CORE_FN(gb_run_frame)(context->gb);
#else
        // copy gb to stack (DTCM) temporarily
        struct gb_s gb;
        memcpy(&gb, context->gb, sizeof(struct gb_s));

        gb_run_frame(&gb);

        memcpy(context->gb, &gb, sizeof(struct gb_s));
#endif

        // --- 1. Dynamic Frame Skipping Decision ---
        bool should_draw_this_frame = true;

        if (context->frames_to_skip_drawing > 0)
        {  // A skip strategy is active
            bool planned_skip_for_this_frame_in_burst =
                (context->frames_actually_skipped_drawing <
                     context->frames_to_skip_drawing &&
                 context->frames_actually_skipped_drawing <
                     MAX_CONSECUTIVE_DRAW_SKIPS);

            if (planned_skip_for_this_frame_in_burst)
            {
                // A skip is planned. Check if previous frame's performance
                // allows an override.
                if (actual_delta_time_ms < (TARGET_TIME_PER_GB_FRAME_MS *
                                            PERFORMANCE_SKIP_OVERRIDE_FACTOR))
                {
                    // Previous frame was fast, override skip and draw this
                    // frame.
                    should_draw_this_frame = true;
                    context->frames_actually_skipped_drawing = 0;
                }
                else
                {
                    // Previous frame wasn't fast enough, proceed with planned
                    // skip.
                    should_draw_this_frame = false;
                    context->frames_actually_skipped_drawing++;
                }
            }
            else
            {
                // Max skips for burst reached or planned skips for burst done.
                should_draw_this_frame = true;
                context->frames_actually_skipped_drawing = 0;
            }
        }
        else
        {
            should_draw_this_frame = true;
            context->frames_actually_skipped_drawing = 0;
        }

        // --- 2. Conditional Screen Update (Drawing) Logic ---
        if (should_draw_this_frame)
        {
            uint8_t *current_lcd = context->gb->lcd;
            bool any_line_changed_this_frame = false;
            for (int y = 0; y < LCD_HEIGHT; y++)
            {
                if (memcmp(&current_lcd[y * LCD_WIDTH_PACKED],
                           &context->previous_lcd[y * LCD_WIDTH_PACKED],
                           LCD_WIDTH_PACKED) != 0)
                {
                    context->line_has_changed[y] = true;
                    any_line_changed_this_frame = true;
                }
                else
                {
                    context->line_has_changed[y] = false;
                }
            }

            // Determine if drawing is actually needed based on changes or
            // forced display
            bool actual_gb_draw_needed =
                context->gb->lcd_master_enable &&
                (any_line_changed_this_frame || gbScreenRequiresFullRefresh);

            if (actual_gb_draw_needed)
            {
                if (gbScreenRequiresFullRefresh)
                {
                    for (int i = 0; i < LCD_HEIGHT; i++)
                    {
                        context->line_has_changed[i] = true;
                    }
                }

                ITCM_CORE_FN(update_fb_dirty_lines)(
                    playdate->graphics->getFrame(), current_lcd,
                    context->line_has_changed,
                    playdate->graphics->markUpdatedRows);

                memcpy(context->previous_lcd, current_lcd,
                       LCD_HEIGHT * LCD_WIDTH_PACKED);
            }
        }

        // --- 3. Adjust Skip Level Periodically ---
        if (context->frames_emulated_since_adjustment >=
            ADJUSTMENT_PERIOD_FRAMES)
        {
            float average_time_per_emulated_frame_ms =
                context->total_time_for_adjustment_period_ms /
                context->frames_emulated_since_adjustment;

            if (average_time_per_emulated_frame_ms >
                TARGET_TIME_PER_GB_FRAME_MS * 1.1f)
            {
                if (context->frames_to_skip_drawing <
                    MAX_CONSECUTIVE_DRAW_SKIPS)
                {
                    context->frames_to_skip_drawing++;
                }
            }
            else if (average_time_per_emulated_frame_ms <
                     TARGET_TIME_PER_GB_FRAME_MS * 0.9f)
            {
                if (context->frames_to_skip_drawing > 0)
                {
                    context->frames_to_skip_drawing--;
                }
            }

            // Reset for next adjustment period
            context->total_time_for_adjustment_period_ms = 0.0f;
            context->frames_emulated_since_adjustment = 0;
        }

        // Always request the update loop to run at 60 FPS.
        // This ensures gb_run_frame() is called at a consistent rate.
        gameScene->scene->preferredRefreshRate = 60;
        gameScene->scene->refreshRateCompensation =
            (1.0f / 60.0f - PGB_App->dt);

#if 0
            uint8_t *framebuffer = ;

            int skip_counter = 0;
            bool single_line = false;

            int y2 = 0;
            int lcd_rows = PGB_LCD_Y * LCD_ROWSIZE + PGB_LCD_X / 8;

            int row_offset = LCD_ROWSIZE;
            int row_offset2 = LCD_ROWSIZE * 2;

            int y_offset;
            int next_y_offset = 2;

            for(int y = 0; y < (LCD_HEIGHT - 1); y++)
            {
                y_offset = next_y_offset;

                if(skip_counter == 5)
                {
                    y_offset = 1;
                    next_y_offset = 1;
                    skip_counter = 0;
                    single_line = true;
                }
                else if(single_line)
                {
                    next_y_offset = 2;
                    single_line = false;
                }

                uint8_t *pixels;
                uint8_t *old_pixels;

                if(context->gb->display.back_fb_enabled)
                {
                    pixels = gb_front_fb[y];
                    old_pixels = gb_back_fb[y];
                }
                else
                {
                    pixels = gb_back_fb[y];
                    old_pixels = gb_front_fb[y];
                }

                if(memcmp(pixels, old_pixels, LCD_WIDTH) != 0)
                {
                    int d_row1 = y2 & 3;
                    int d_row2 = (y2 + 1) & 3;

                    int fb_index1 = lcd_rows;
                    int fb_index2 = lcd_rows + row_offset;

                    memset(&framebuffer[fb_index1], 0x00, PGB_LCD_ROWSIZE);
                    if(y_offset == 2)
                    {
                        memset(&framebuffer[fb_index2], 0x00, PGB_LCD_ROWSIZE);
                    }

                    uint8_t bit = 0;

                    for(int x = 0; x < LCD_WIDTH; x++)
                    {
                        uint8_t pixel = __gb_get_pixel(pixels, x) & 3;

                        framebuffer[fb_index1] |= PGB_bitmask[pixel][bit][d_row1];
                        if(y_offset == 2)
                        {
                            framebuffer[fb_index2] |= PGB_bitmask[pixel][bit][d_row2];
                        }

                        bit++;

                        if(bit == 4)
                        {
                            bit = 0;
                            fb_index1++;
                            fb_index2++;
                        }
                    }

                    playdate->graphics->markUpdatedRows(y2, y2 + y_offset - 1);

#if PGB_DEBUG && PGB_DEBUG_UPDATED_ROWS
                    for(int i = 0; i < y_offset; i++){
                        context->scene->debug_updatedRows[y2 + i] = true;
                    }
#endif
                }

                y2 += y_offset;
                lcd_rows += (y_offset == 1) ? row_offset : row_offset2;

                if(!single_line)
                {
                    skip_counter++;
                }
            }
        }
#endif

        if (gameScene->cartridge_has_rtc)
        {
            const uint8_t MAX_RTC_TICKS_PER_FRAME = 1;
            const uint8_t MAX_CATCH_UP_INCREMENT = 255;  // 4 minutes 15 seconds

            gameScene->rtc_fractional_second_accumulator += PGB_App->dt;

            if (gameScene->rtc_fractional_second_accumulator >= 1.0f)
            {
                unsigned int total_whole_seconds_accumulated =
                    (unsigned int)gameScene->rtc_fractional_second_accumulator;

                uint8_t seconds_to_process_now;

                if (total_whole_seconds_accumulated > MAX_CATCH_UP_INCREMENT)
                {
                    seconds_to_process_now = MAX_CATCH_UP_INCREMENT;
                }
                else
                {
                    seconds_to_process_now =
                        (uint8_t)total_whole_seconds_accumulated;
                }

                gameScene->rtc_seconds_to_catch_up += seconds_to_process_now;
                gameScene->rtc_fractional_second_accumulator -=
                    (float)seconds_to_process_now;
                gameScene->rtc_time += seconds_to_process_now;
            }

            uint8_t ticks_this_frame = 0;
            while (gameScene->rtc_seconds_to_catch_up > 0 &&
                   ticks_this_frame < MAX_RTC_TICKS_PER_FRAME)
            {
                gb_tick_rtc(context->gb);
                gameScene->rtc_seconds_to_catch_up--;
                ticks_this_frame++;
            }
        }

        if (!gameScene->staticSelectorUIDrawn || gbScreenRequiresFullRefresh)
        {
            playdate->graphics->setFont(PGB_App->labelFont);
            playdate->graphics->setDrawMode(kDrawModeFillWhite);
            playdate->graphics->drawText(
                startButtonText, pgb_strlen(startButtonText), kUTF8Encoding,
                gameScene->selector.startButtonX,
                gameScene->selector.startButtonY);
            playdate->graphics->drawText(
                selectButtonText, pgb_strlen(selectButtonText), kUTF8Encoding,
                gameScene->selector.selectButtonX,
                gameScene->selector.selectButtonY);
            playdate->graphics->setDrawMode(kDrawModeCopy);
        }

        if (animatedSelectorBitmapNeedsRedraw)
        {
            LCDBitmap *bitmap;
            // Use gameScene->selector.index, which is the most current
            // calculated frame
            if (gameScene->selector.index < 0)
            {
                bitmap = PGB_App->startSelectBitmap;
            }
            else
            {
                bitmap = playdate->graphics->getTableBitmap(
                    PGB_App->selectorBitmapTable, gameScene->selector.index);
            }
            playdate->graphics->drawBitmap(bitmap, gameScene->selector.x,
                                           gameScene->selector.y,
                                           kBitmapUnflipped);
        }

        if (!gameScene->staticSelectorUIDrawn || gbScreenRequiresFullRefresh)
        {
            gameScene->staticSelectorUIDrawn = true;
        }

#if PGB_DEBUG && PGB_DEBUG_UPDATED_ROWS
        PDRect highlightFrame = gameScene->debug_highlightFrame;
        playdate->graphics->fillRect(highlightFrame.x, highlightFrame.y,
                                     highlightFrame.width,
                                     highlightFrame.height, kColorBlack);

        for (int y = 0; y < PGB_LCD_HEIGHT; y++)
        {
            int absoluteY = PGB_LCD_Y + y;

            if (gameScene->debug_updatedRows[absoluteY])
            {
                playdate->graphics->fillRect(highlightFrame.x, absoluteY,
                                             highlightFrame.width, 1,
                                             kColorWhite);
            }
        }
#endif

        if (preferences_display_fps)
        {
            playdate->system->drawFPS(0, 0);
        }
    }
    else if (gameScene->state == PGB_GameSceneStateError)
    {
        gameScene->scene->preferredRefreshRate = 30;
        gameScene->scene->refreshRateCompensation = 0;

        if (gbScreenRequiresFullRefresh)
        {
            char *errorTitle = "Oh no!";

            int errorMessagesCount = 1;
            char *errorMessages[4];

            errorMessages[0] = "A generic error occurred";

            if (gameScene->error == PGB_GameSceneErrorLoadingRom)
            {
                errorMessages[0] = "Can't load the selected ROM";
            }
            else if (gameScene->error == PGB_GameSceneErrorWrongLocation)
            {
                errorTitle = "Wrong location";
                errorMessagesCount = 2;
                errorMessages[0] = "Please move the ROM to";
                errorMessages[1] = "/Data/*.playgb/games/";
            }
            else if (gameScene->error == PGB_GameSceneErrorFatal)
            {
                errorMessages[0] = "A fatal error occurred";
            }

            playdate->graphics->clear(kColorWhite);

            int titleToMessageSpacing = 6;

            int titleHeight =
                playdate->graphics->getFontHeight(PGB_App->titleFont);
            int lineSpacing = 2;
            int messageHeight =
                playdate->graphics->getFontHeight(PGB_App->bodyFont);
            int messagesHeight = messageHeight * errorMessagesCount +
                                 lineSpacing * (errorMessagesCount - 1);

            int containerHeight =
                titleHeight + titleToMessageSpacing + messagesHeight;

            int titleX = (float)(playdate->display->getWidth() -
                                 playdate->graphics->getTextWidth(
                                     PGB_App->titleFont, errorTitle,
                                     strlen(errorTitle), kUTF8Encoding, 0)) /
                         2;
            int titleY =
                (float)(playdate->display->getHeight() - containerHeight) / 2;

            playdate->graphics->setFont(PGB_App->titleFont);
            playdate->graphics->drawText(errorTitle, strlen(errorTitle),
                                         kUTF8Encoding, titleX, titleY);

            int messageY = titleY + titleHeight + titleToMessageSpacing;

            for (int i = 0; i < errorMessagesCount; i++)
            {
                char *errorMessage = errorMessages[i];
                int messageX =
                    (float)(playdate->display->getWidth() -
                            playdate->graphics->getTextWidth(
                                PGB_App->bodyFont, errorMessage,
                                strlen(errorMessage), kUTF8Encoding, 0)) /
                    2;

                playdate->graphics->setFont(PGB_App->bodyFont);
                playdate->graphics->drawText(errorMessage, strlen(errorMessage),
                                             kUTF8Encoding, messageX, messageY);

                messageY += messageHeight + lineSpacing;
            }

            gameScene->staticSelectorUIDrawn = false;
        }
    }
}

static void PGB_GameScene_didSelectSave(void *userdata)
{
    PGB_GameScene *gameScene = userdata;

    gameScene->audioLocked = true;

    PGB_GameScene_saveGame(gameScene);

    gameScene->audioLocked = false;
}

static void PGB_GameScene_didSelectLibrary(void *userdata)
{
    PGB_GameScene *gameScene = userdata;

    gameScene->audioLocked = true;

    PGB_goToLibrary();
}

static void PGB_GameScene_didToggleLCD(void *userdata)
{
    PGB_GameScene *gameScene = userdata;

    gameScene->context->gb->lcd_master_enable ^= 1;
}

static void PGB_GameScene_menu(void *object)
{
    PGB_GameScene *gameScene = object;
    playdate->system->removeAllMenuItems();

    if (gameScene->rom_filename != NULL)
    {
        char *rom_basename_full = string_copy(gameScene->rom_filename);
        char *filename_part = rom_basename_full;
        char *last_slash = strrchr(rom_basename_full, '/');
        if (last_slash != NULL)
        {
            filename_part = last_slash + 1;
        }

        char *rom_basename_ext = string_copy(filename_part);

        char *basename_no_ext = string_copy(rom_basename_ext);
        char *ext = strrchr(basename_no_ext, '.');
        if (ext != NULL)
        {
            *ext = '\0';
        }

        char *cleanName_no_ext = string_copy(basename_no_ext);
        pgb_sanitize_string_for_filename(cleanName_no_ext);

        char *actual_cover_path =
            pgb_find_cover_art_path(basename_no_ext, cleanName_no_ext);

        if (actual_cover_path != NULL)
        {
            PGB_LoadedCoverArt cover_art =
                pgb_load_and_scale_cover_art_from_path(actual_cover_path, 200,
                                                       200);

            if (cover_art.status == PGB_COVER_ART_SUCCESS &&
                cover_art.bitmap != NULL)
            {
                LCDBitmap *menuImage =
                    playdate->graphics->newBitmap(400, 240, kColorClear);
                if (menuImage != NULL)
                {
                    int final_scaled_width = cover_art.scaled_width;
                    int final_scaled_height = cover_art.scaled_height;

                    int drawX = (200 - final_scaled_width) / 2;
                    if (drawX < 0)
                    {
                        drawX = 0;
                    }

                    int drawY = 40 + (160 - final_scaled_height) / 2;

                    playdate->graphics->pushContext(menuImage);
                    playdate->graphics->setDrawMode(kDrawModeCopy);

                    playdate->graphics->fillRect(0, 0, 400, 40, kColorBlack);
                    playdate->graphics->fillRect(0, 200, 400, 40, kColorBlack);

                    playdate->graphics->drawBitmap(cover_art.bitmap, drawX,
                                                   drawY, kBitmapUnflipped);

                    playdate->graphics->popContext();
                    playdate->system->setMenuImage(menuImage, 0);
                }
                pgb_free_loaded_cover_art_bitmap(&cover_art);
            }
            pgb_free(actual_cover_path);
        }

        pgb_free(cleanName_no_ext);
        pgb_free(basename_no_ext);
        pgb_free(rom_basename_ext);
        pgb_free(rom_basename_full);
    }

    playdate->system->addMenuItem("Library", PGB_GameScene_didSelectLibrary,
                                  gameScene);

    if (gameScene->state == PGB_GameSceneStateLoaded)
    {
        playdate->system->addMenuItem("Save", PGB_GameScene_didSelectSave,
                                      gameScene);

        playdate->system->addCheckmarkMenuItem(
            "LCD", 1, PGB_GameScene_didToggleLCD, gameScene);
    }
}

static void PGB_GameScene_saveGame(PGB_GameScene *gameScene)
{
    if (gameScene->state == PGB_GameSceneStateLoaded)
    {
        PGB_GameSceneContext *context = gameScene->context;

        if (gameScene->save_filename)
        {
            write_cart_ram_file(gameScene->save_filename,
                                &context->gb->gb_cart_ram,
                                gb_get_save_size(context->gb));
        }
    }
}

static void PGB_GameScene_generateBitmask(void)
{
    if (PGB_GameScene_bitmask_done)
    {
        return;
    }

    PGB_GameScene_bitmask_done = true;

    for (int colour = 0; colour < 4; colour++)
    {
        for (int y = 0; y < 4; y++)
        {
            int x_offset = 0;

            for (int i = 0; i < 4; i++)
            {
                int mask = 0x00;

                for (int x = 0; x < 2; x++)
                {
                    if (PGB_patterns[colour][y][x_offset + x] == 1)
                    {
                        int n = i * 2 + x;
                        mask |= (1 << (7 - n));
                    }
                }

                PGB_bitmask[colour][i][y] = mask;

                x_offset ^= 2;
            }
        }
    }
}

static void PGB_GameScene_free(void *object)
{
    PGB_GameScene *gameScene = object;
    PGB_GameSceneContext *context = gameScene->context;

    audioGameScene = NULL;

    playdate->system->setMenuImage(NULL, 0);

    PGB_Scene_free(gameScene->scene);

    PGB_GameScene_saveGame(gameScene);

    gb_reset(context->gb);

    pgb_free(gameScene->rom_filename);

    if (gameScene->save_filename)
    {
        pgb_free(gameScene->save_filename);
    }

    if (context->rom)
    {
        pgb_free(context->rom);
    }

    if (context->cart_ram)
    {
        pgb_free(context->cart_ram);
    }

    pgb_free(context);
    pgb_free(gameScene);
}
