//
//  game_scene.c
//  CrankBoy
//
//  Created by Matteo D'Ignazio on 14/05/22.
//  Maintained and developed by the CrankBoy dev team.
//

#define PGB_IMPL

#include "../minigb_apu/minigb_apu.h"
#include "../peanut_gb/peanut_gb.h"
#include "app.h"
#include "dtcm.h"
#include "preferences.h"
#include "revcheck.h"
#include "script.h"
#include "userstack.h"
#include "utility.h"

// clang-format off
#include "game_scene.h"
// clang-format on

// attempt to stay on top of frames per second
#define DYNAMIC_RATE_ADJUSTMENT 1

// TODO: double-check these

// approximately how long it takes to render one gameboy line
#define LINE_RENDER_TIME_S 0.000032f

// expected extra time outside of logic + rendering
#define LINE_RENDER_MARGIN_S 0.0005f

// let's try to render a frame at least this fast
#define TARGET_RENDER_TIME_S 0.0167f

PGB_GameScene *audioGameScene = NULL;

static void PGB_GameScene_selector_init(PGB_GameScene *gameScene);
static void PGB_GameScene_update(void *object);
static void PGB_GameScene_menu(void *object);
static void PGB_GameScene_generateBitmask(void);
static void PGB_GameScene_free(void *object);
static void PGB_GameScene_event(void *object, PDSystemEvent event,
                                uint32_t arg);

static uint8_t *read_rom_to_ram(const char *filename,
                                PGB_GameSceneError *sceneError);

static void read_cart_ram_file(const char *save_filename, uint8_t **dest,
                               const size_t len);
static void write_cart_ram_file(const char *save_filename, uint8_t *src,
                                const size_t len);

static void gb_error(struct gb_s *gb, const enum gb_error_e gb_err,
                     const uint16_t val);
static void gb_save_to_disk(struct gb_s *gb);

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
    playdate->system->logToConsole("ROM: %s", rom_filename);
    playdate->system->setCrankSoundsDisabled(true);

    if (!DTCM_VERIFY_DEBUG())
        return NULL;

    PGB_Scene *scene = PGB_Scene_new();

    PGB_GameScene *gameScene = pgb_malloc(sizeof(PGB_GameScene));
    gameScene->scene = scene;
    scene->managedObject = gameScene;

    scene->update = PGB_GameScene_update;
    scene->menu = PGB_GameScene_menu;
    scene->free = PGB_GameScene_free;
    scene->event = PGB_GameScene_event;
    scene->use_user_stack = 0;  // user stack is slower

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

    gameScene->save_data_loaded_successfully = false;

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
    memset(gb, 0, sizeof(struct gb_s));
    itcm_core_init();
    DTCM_VERIFY_DEBUG();

    if (PGB_App->soundSource == NULL)
    {
        PGB_App->soundSource =
            playdate->sound->addSource(audio_callback, &audioGameScene, 1);
    }
    audio_enabled = 1;
    context->gb = gb;
    context->scene = gameScene;
    context->rom = NULL;
    context->cart_ram = NULL;

    PDButtons current_pd_buttons;
    playdate->system->getButtonState(&current_pd_buttons, NULL, NULL);
    context->buttons_held_since_start = current_pd_buttons;

    gameScene->context = context;

    PGB_GameSceneError romError;
    uint8_t *rom = read_rom_to_ram(rom_filename, &romError);
    if (rom)
    {
        context->rom = rom;

        static uint8_t lcd[LCD_HEIGHT * LCD_WIDTH_PACKED * 2];
        memset(lcd, 0, sizeof(lcd));

        enum gb_init_error_e gb_ret =
            gb_init(context->gb, context->wram, context->vram, lcd, rom,
                    gb_error, context);

        if (gb_ret == GB_INIT_NO_ERROR)
        {
            char *save_filename = pgb_save_filename(rom_filename, false);
            gameScene->save_filename = save_filename;

            read_cart_ram_file(save_filename, &context->cart_ram,
                               gb_get_save_size(context->gb));

            gameScene->save_data_loaded_successfully = true;

            context->gb->gb_cart_ram = context->cart_ram;
            context->gb->gb_cart_ram_size = gb_get_save_size(context->gb);

            uint8_t actual_cartridge_type = context->gb->gb_rom[0x0147];
            if (actual_cartridge_type == 0x0F || actual_cartridge_type == 0x10)
            {
                gameScene->cartridge_has_rtc = true;
                playdate->system->logToConsole(
                    "Cartridge Type 0x%02X: RTC Enabled.",
                    actual_cartridge_type);

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
                playdate->system->logToConsole(
                    "Cartridge Type 0x%02X (MBC: %d): RTC Disabled.",
                    actual_cartridge_type, context->gb->mbc);
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

#ifndef NOLUA
    char name[17];
    gb_get_rom_name(context->gb, name);
    playdate->system->logToConsole("ROM name: \"%s\"", name);
    gameScene->script = script_begin(name, gameScene);
    if (!gameScene->script)
    {
        playdate->system->logToConsole(
            "Associated script failed to load or not found.");
    }
#endif
    DTCM_VERIFY();

    PGB_ASSERT(gameScene->context == context);
    PGB_ASSERT(gameScene->context->scene == gameScene);
    PGB_ASSERT(gameScene->context->gb->direct.priv == context);

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
    gameScene->selector.triggerAngle = 45;
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

static void write_cart_ram_file(const char *save_filename, uint8_t *src,
                                const size_t len)
{
    if (len == 0 || src == NULL)
    {
        return;
    }

    playdate->system->logToConsole("Saving SRAM to file %s", save_filename);

    SDFile *f = playdate->file->open(save_filename, kFileWrite);

    if (f == NULL)
    {
        playdate->system->logToConsole("%s:%i: Can't write save file", __FILE__,
                                       __LINE__, save_filename);
        return;
    }

    /* Record save file. */
    playdate->file->write(f, src, (unsigned int)(len * sizeof(uint8_t)));
    playdate->file->close(f);
}

static void gb_save_to_disk(struct gb_s *gb)
{
    DTCM_VERIFY_DEBUG();

    PGB_GameSceneContext *context = gb->direct.priv;
    PGB_GameScene *gameScene = context->scene;

    if (!context->gb->direct.sram_dirty)
        return;

    if (gameScene->save_filename)
    {
        write_cart_ram_file(gameScene->save_filename, context->gb->gb_cart_ram,
                            gb_get_save_size(context->gb));
    }
    else
    {
        playdate->system->logToConsole(
            "No save file name specified; can't save.");
    }

    context->gb->direct.sram_dirty = false;

    DTCM_VERIFY_DEBUG();
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
        playdate->system->logToConsole("Invalid read: addr %04x", val);
    }
    else if (gb_err == GB_INVALID_WRITE)
    {
        playdate->system->logToConsole("Invalid write: addr %04x", val);
    }
    else
    {
        is_fatal = true;
        playdate->system->logToConsole("%s:%i: Unknown error occurred",
                                       __FILE__, __LINE__);
    }

    if (is_fatal)
    {
        // save a recovery file
        if (context->scene->save_data_loaded_successfully)
        {
            char *recovery_filename =
                pgb_save_filename(context->scene->rom_filename, true);
            write_cart_ram_file(recovery_filename, context->gb->gb_cart_ram,
                                gb_get_save_size(context->gb));
            pgb_free(recovery_filename);
        }

        // TODO: write recovery savestate

        context->scene->state = PGB_GameSceneStateError;
        context->scene->error = PGB_GameSceneErrorFatal;

        PGB_Scene_refreshMenu(context->scene->scene);
    }

    return;
}

typedef typeof(playdate->graphics->markUpdatedRows) markUpdateRows_t;

__core_section("fb") void update_fb_dirty_lines(
    uint8_t *restrict framebuffer, uint8_t *restrict lcd,
    const uint16_t *restrict line_changed_flags,
    markUpdateRows_t markUpdateRows)
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

        if (((line_changed_flags[y_gb / 16] >> (y_gb % 16)) & 1) == 0)
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

static void save_check(struct gb_s *gb);

__section__(".text.tick") __space static void PGB_GameScene_update(void *object)
{
    PGB_GameScene *gameScene = object;
    PGB_GameSceneContext *context = gameScene->context;

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

        PDButtons current_pd_buttons;
        playdate->system->getButtonState(&current_pd_buttons, NULL, NULL);

        // mask out buttons that have been held down since the game started
        context->buttons_held_since_start &= current_pd_buttons;

#if 0
        current_pd_buttons &= ~context->buttons_held_since_start;
#endif

        bool gb_joypad_start_is_active_low =
            !(gameScene->selector.startPressed);
        bool gb_joypad_select_is_active_low =
            !(gameScene->selector.selectPressed);

        context->gb->direct.joypad_bits.start = gb_joypad_start_is_active_low;
        context->gb->direct.joypad_bits.select = gb_joypad_select_is_active_low;

        context->gb->direct.joypad_bits.a = !(current_pd_buttons & kButtonA);
        context->gb->direct.joypad_bits.b = !(current_pd_buttons & kButtonB);
        context->gb->direct.joypad_bits.left =
            !(current_pd_buttons & kButtonLeft);
        context->gb->direct.joypad_bits.up = !(current_pd_buttons & kButtonUp);
        context->gb->direct.joypad_bits.right =
            !(current_pd_buttons & kButtonRight);
        context->gb->direct.joypad_bits.down =
            !(current_pd_buttons & kButtonDown);

        if (gbScreenRequiresFullRefresh)
        {
            playdate->graphics->clear(kColorBlack);
        }

#if PGB_DEBUG && PGB_DEBUG_UPDATED_ROWS
        memset(gameScene->debug_updatedRows, 0, LCD_ROWS);
#endif

        context->gb->direct.sram_updated = 0;

#ifndef NOLUA
        if (context->scene->script)
        {
            script_tick(context->scene->script);
        }
#endif

        PGB_ASSERT(context == context->gb->direct.priv);

#ifdef DTCM_ALLOC
        DTCM_VERIFY_DEBUG();
        ITCM_CORE_FN(gb_run_frame)(context->gb);
        DTCM_VERIFY_DEBUG();
#else
        // copy gb to stack (DTCM) temporarily
        struct gb_s gb;
        struct gb_s *tmp_gb = context->gb;
        context->gb = &gb;
        memcpy(&gb, tmp_gb, sizeof(struct gb_s));

        gb_run_frame(&gb);

        memcpy(tmp_gb, &gb, sizeof(struct gb_s));
        context->gb = tmp_gb;
#endif

        if (context->gb->cart_battery)
        {
            save_check(context->gb);
        }

#if DYNAMIC_RATE_ADJUSTMENT
        float logic_time = playdate->system->getElapsedTime();
#endif

        // --- Conditional Screen Update (Drawing) Logic ---
        uint8_t *current_lcd = context->gb->lcd;
        int line_changed_count = 0;
        uint16_t line_has_changed[LCD_HEIGHT / 16];
        for (int y = 0; y < LCD_HEIGHT / 16; y++)
        {
            uint16_t changed = 0;
            for (int y2 = 0; y2 < 16; ++y2)
            {
                changed >>= 1;
                uint8_t sy = (y * 16) | y2;
                if (memcmp(&current_lcd[sy * LCD_WIDTH_PACKED],
                           &context->previous_lcd[sy * LCD_WIDTH_PACKED],
                           LCD_WIDTH_PACKED) != 0)
                {
                    changed |= 0x8000;
                }
            }

            line_has_changed[y] = changed;
        }

#if DYNAMIC_RATE_ADJUSTMENT
        uint16_t interlace_mask = 0xFFFF;
        static int interlace_i = 0;
        float time_for_rendering =
            TARGET_RENDER_TIME_S - LINE_RENDER_MARGIN_S - logic_time;
        static int frame_i = 0;
        if (++frame_i % 256 == 16)
        {
            printf("logic: %f, time for rendering: %f; %f\n",
                   1000 * (double)logic_time, 1000 * (double)time_for_rendering,
                   1000 * (double)(line_changed_count * LINE_RENDER_TIME_S));
        }
        if (time_for_rendering < line_changed_count * LINE_RENDER_TIME_S)
        {
            ++interlace_i;

            if (time_for_rendering >=
                line_changed_count * LINE_RENDER_TIME_S * 0.75f)
            {
                // render 3 out of 4 lines
                interlace_mask = 0b11101110111011101110 >> (interlace_i % 4);
            }
            else
            {
                // render 1 out of 2 lines
                interlace_mask =
                    (interlace_i % 2) ? 0b1010101010101010 : 0b0101010101010101;
            }
        }

        static volatile int k = 0;
        if (k == 1)
            interlace_mask = 0xFFFF;
#endif

        // Determine if drawing is actually needed based on changes or
        // forced display
        bool actual_gb_draw_needed = true;
        // gbScreenRequiresFullRefresh;

        if (actual_gb_draw_needed)
        {
            if (gbScreenRequiresFullRefresh)
            {
                for (int i = 0; i < LCD_HEIGHT / 16; i++)
                {
                    line_has_changed[i] = 0xFFFF;
                }
            }
#if DYNAMIC_RATE_ADJUSTMENT
            else
            {
                for (int i = 0; i < LCD_HEIGHT / 16; i++)
                {
                    line_has_changed[i] &= interlace_mask;
                }
            }
#endif

            ITCM_CORE_FN(update_fb_dirty_lines)(
                playdate->graphics->getFrame(), current_lcd, line_has_changed,
                playdate->graphics->markUpdatedRows);

            for (int i = 0; i < LCD_HEIGHT; i++)
            {
                if ((line_has_changed[i / 16] >> (i % 16)) & 1)
                {
                    ITCM_CORE_FN(gb_fast_memcpy_32)(
                        context->previous_lcd + LCD_WIDTH_PACKED * i,
                        current_lcd + LCD_WIDTH_PACKED * i, LCD_WIDTH_PACKED);
                }
            }
        }

        // Always request the update loop to run at 60 FPS.
        // This ensures gb_run_frame() is called at a consistent rate.
        gameScene->scene->preferredRefreshRate = 60;
        gameScene->scene->refreshRateCompensation =
            (1.0f / 60.0f - PGB_App->dt);

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
                errorMessages[1] = "/Data/*.crankboy/games/";
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

__section__(".text.tick") __space static void save_check(struct gb_s *gb)
{
    static uint32_t frames_since_last_save, frames_since_sram_update;

    // save SRAM under some conditions
    // TODO: also save if menu opens, playdate goes to sleep, app closes, or
    // powers down
    ++frames_since_last_save;
    gb->direct.sram_dirty |= gb->direct.sram_updated;
    if (gb->cart_battery && gb->direct.sram_dirty)
    {
        if (frames_since_last_save >= PGB_MAX_FRAMES_SAVE)
        {
            playdate->system->logToConsole("Saving (periodic)");
            gb_save_to_disk(gb);
            frames_since_last_save = 0;
            frames_since_sram_update = 0;
        }
        else if (!gb->direct.sram_updated)
        {
            if (frames_since_sram_update >= PGB_MIN_FRAMES_SAVE)
            {
                playdate->system->logToConsole(
                    "Saving (gap since last ram edit)");
                gb_save_to_disk(gb);
                frames_since_last_save = 0;
            }
            frames_since_sram_update = 0;
        }
    }
    else
    {
        frames_since_sram_update++;
    }
}

__section__(".rare") void PGB_GameScene_didSelectLibrary(void *userdata)
{
    DTCM_VERIFY();

    PGB_GameScene *gameScene = userdata;

    gameScene->audioLocked = true;

    call_with_user_stack(PGB_goToLibrary);

    DTCM_VERIFY();
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

__section__(".rare") static void PGB_GameScene_event(void *object,
                                                     PDSystemEvent event,
                                                     uint32_t arg)
{
    PGB_GameScene *gameScene = object;
    PGB_GameSceneContext *context = gameScene->context;

    switch (event)
    {
    case kEventLock:
    case kEventPause:
    case kEventTerminate:
        if (context->gb->direct.sram_dirty)
        {
            playdate->system->logToConsole("saving (system event)");
            gb_save_to_disk(context->gb);
        }
        break;
    case kEventLowPower:
        if (context->gb->direct.sram_dirty &&
            gameScene->save_data_loaded_successfully)
        {
            // save a recovery file
            char *recovery_filename =
                pgb_save_filename(context->scene->rom_filename, true);
            write_cart_ram_file(recovery_filename, context->gb->gb_cart_ram,
                                gb_get_save_size(context->gb));
            pgb_free(recovery_filename);
        }
        break;
    default:
        break;
    }
}

static void PGB_GameScene_free(void *object)
{
    audio_enabled = 0;

    DTCM_VERIFY_DEBUG();
    PGB_GameScene *gameScene = object;
    PGB_GameSceneContext *context = gameScene->context;

    audioGameScene = NULL;

    playdate->system->setMenuImage(NULL, 0);

    PGB_Scene_free(gameScene->scene);

    gb_save_to_disk(context->gb);

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
    DTCM_VERIFY_DEBUG();
}

__section__(".rare") void __gb_on_breakpoint(struct gb_s *gb,
                                             int breakpoint_number)
{
    PGB_GameSceneContext *context = gb->direct.priv;
    PGB_GameScene *gameScene = context->scene;

    PGB_ASSERT(gameScene->context == context);
    PGB_ASSERT(gameScene->context->scene == gameScene);
    PGB_ASSERT(gameScene->context->gb->direct.priv == context);
    PGB_ASSERT(gameScene->context->gb == gb);

    call_with_user_stack_2(script_on_breakpoint, gameScene->script,
                           breakpoint_number);
}
