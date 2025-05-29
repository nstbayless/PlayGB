//
//  game_scene.c
//  PlayGB
//
//  Created by Matteo D'Ignazio on 14/05/22.
//

#define PEANUT_GB_IMPL

#include "game_scene.h"
#include "minigb_apu.h"
#include "app.h"
#include "peanut_gb.h"
#include "library_scene.h"
#include "preferences.h"
#include "rom.h"

PGB_GameScene *audioGameScene = NULL;

typedef struct PGB_GameSceneContext {
    PGB_GameScene *scene;
    struct gb_s* gb;
    uint8_t wram[WRAM_SIZE];
    uint8_t vram[VRAM_SIZE];
    uint8_t *rom;
    uint8_t *cart_ram;
} PGB_GameSceneContext;

static void PGB_GameScene_selector_init(PGB_GameScene *gameScene);
static void PGB_GameScene_update(void *object);
static void PGB_GameScene_menu(void *object);
static void PGB_GameScene_saveGame(PGB_GameScene *gameScene);
static void PGB_GameScene_generateBitmask(void);
static void PGB_GameScene_free(void *object);

static uint8_t *read_rom_to_ram(const char *filename, PGB_GameSceneError *sceneError);

static void read_cart_ram_file(const char *save_filename, uint8_t **dest, const size_t len);
static void write_cart_ram_file(const char *save_filename, uint8_t **dest, const size_t len);

static void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t val);

static const char *startButtonText = "start";
static const char *selectButtonText = "select";

static uint8_t PGB_bitmask[4][4][4];
static bool PGB_GameScene_bitmask_done = false;

#if ITCM_CORE
    void* core_itcm_reloc = NULL;

    void itcm_core_init(void)
    {
        if (core_itcm_reloc != NULL) return;
        
        // allocate in tcm, aligned
        uintptr_t core_itcm_reloc_u = dtcm_alloc(itcm_core_size + CACHE_LINE);
        core_itcm_reloc_u += CACHE_LINE - 1 - 0x4;
        core_itcm_reloc_u /= CACHE_LINE;
        core_itcm_reloc_u *= CACHE_LINE;
        core_itcm_reloc_u += 0x4;
        core_itcm_reloc = (void*)core_itcm_reloc_u;
        
        memcpy(core_itcm_reloc, __itcm_start, itcm_core_size);
        playdate->system->logToConsole("itcm start: %x, end %x: run_frame: %x", &__itcm_start, &__itcm_end, &gb_run_frame);
        playdate->system->logToConsole("core is 0x%X bytes, relocated at 0x%X", itcm_core_size, core_itcm_reloc);
        playdate->system->clearICache();
    }
#else
    void itcm_core_init(void) {}
#endif

static uint8_t* lcd;

PGB_GameScene* PGB_GameScene_new(const char *rom_filename)
{
    PGB_Scene *scene = PGB_Scene_new();
    
    PGB_GameScene *gameScene = pgb_malloc(sizeof(PGB_GameScene));
    gameScene->scene = scene;
    scene->managedObject = gameScene;
    
    scene->update = PGB_GameScene_update;
    scene->menu = PGB_GameScene_menu;
    scene->free = PGB_GameScene_free;

    scene->preferredRefreshRate = 30;

    gameScene->rtc_timer = 0;
    
    gameScene->rom_filename = string_copy(rom_filename);
    gameScene->save_filename = NULL;

    gameScene->state = PGB_GameSceneStateError;
    gameScene->error = PGB_GameSceneErrorUndefined;

    gameScene->model = (PGB_GameSceneModel){
        .state = PGB_GameSceneStateError,
        .error = PGB_GameSceneErrorUndefined,
        .selectorIndex = 0,
        .empty = true
    };
    
    gameScene->needsDisplay = false;
    
    gameScene->audioEnabled = preferences_sound_enabled;
    gameScene->audioLocked = false;

    PGB_GameScene_generateBitmask();
    
    PGB_GameScene_selector_init(gameScene);
    
    #if PGB_DEBUG && PGB_DEBUG_UPDATED_ROWS
    int highlightWidth = 10;
    gameScene->debug_highlightFrame = PDRectMake(PGB_LCD_X - 1 - highlightWidth, 0, highlightWidth, playdate->display->getHeight());
    #endif
    
    PGB_GameSceneContext *context = pgb_malloc_aligned(sizeof(PGB_GameSceneContext));
    static struct gb_s* gb = NULL;
    if (gb == NULL) gb = dtcm_alloc(sizeof(struct gb_s));
    itcm_core_init();
    context->gb = gb;
    context->scene = gameScene;
    context->rom = NULL;
    context->cart_ram = NULL;
    
    gameScene->context = context;
    
    PGB_GameSceneError romError;
    uint8_t *rom = read_rom_to_ram(rom_filename, &romError);
    if(rom)
    {
        context->rom = rom;
        
        if (!lcd) lcd = pgb_malloc_aligned(LCD_HEIGHT * LCD_WIDTH_PACKED * 2);
        
        enum gb_init_error_e gb_ret = gb_init(context->gb, context->wram, context->vram, lcd, rom, gb_error, context);
        
        if(gb_ret == GB_INIT_NO_ERROR)
        {
            char *save_filename = pgb_save_filename(rom_filename, false);
            gameScene->save_filename = save_filename;
            
            read_cart_ram_file(save_filename, &context->cart_ram, gb_get_save_size(context->gb));
            
            context->gb->gb_cart_ram = context->cart_ram;
            
            audio_init(gb->hram + 0x10);
            if(gameScene->audioEnabled)
            {
                // init audio
                playdate->sound->channel->setVolume(playdate->sound->getDefaultChannel(), 0.2f);
                
                context->gb->direct.sound = 1;
                audioGameScene = gameScene;
            }
            
            // init lcd
            gb_init_lcd(context->gb);
            
            context->gb->direct.frame_skip = 1;

            // set game state to loaded
            gameScene->state = PGB_GameSceneStateLoaded;
        }
        else
        {
            gameScene->state = PGB_GameSceneStateError;
            gameScene->error = PGB_GameSceneErrorFatal;
            
            playdate->system->logToConsole("%s:%i: Error initializing gb context", __FILE__, __LINE__);
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
    int startButtonWidth = playdate->graphics->getTextWidth(PGB_App->labelFont, startButtonText, strlen(startButtonText), kUTF8Encoding, 0);
    int selectButtonWidth = playdate->graphics->getTextWidth(PGB_App->labelFont, selectButtonText, strlen(selectButtonText), kUTF8Encoding, 0);
    
    int width = 18;
    int height = 46;
    
    int startSpacing = 3;
    int selectSpacing = 6;

    int labelHeight = playdate->graphics->getFontHeight(PGB_App->labelFont);

    int containerHeight = labelHeight + startSpacing + height + selectSpacing + labelHeight;
    int containerWidth = width;
    
    containerWidth = PGB_MAX(containerWidth, startButtonWidth);
    containerWidth = PGB_MAX(containerWidth, selectButtonWidth);
    
    int containerX = playdate->display->getWidth() - 6 - containerWidth;
    int containerY = 8;
    
    int x = containerX + (float)(containerWidth - width) / 2;
    int y = containerY + labelHeight + startSpacing;
    
    int startButtonX = containerX + (float)(containerWidth - startButtonWidth) / 2;
    int startButtonY = containerY;
    
    int selectButtonX = containerX + (float)(containerWidth - selectButtonWidth) / 2;
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
static uint8_t *read_rom_to_ram(const char *filename, PGB_GameSceneError *sceneError)
{
    *sceneError = PGB_GameSceneErrorUndefined;
    
    SDFile *rom_file = playdate->file->open(filename, kFileReadData);
    
    if(rom_file == NULL)
    {
        const char *fileError = playdate->file->geterr();
        playdate->system->logToConsole("%s:%i: Can't open rom file %s", __FILE__, __LINE__, filename);
        playdate->system->logToConsole("%s:%i: File error %s", __FILE__, __LINE__, fileError);
        
        *sceneError = PGB_GameSceneErrorLoadingRom;
        
        if(fileError)
        {
            char *fsErrorCode = pgb_extract_fs_error_code(fileError);
            if(fsErrorCode)
            {
                if(strcmp(fsErrorCode, "0709") == 0)
                {
                    *sceneError = PGB_GameSceneErrorWrongLocation;
                }
            }
        }
        return NULL;
    }
    
    playdate->file->seek(rom_file, 0, SEEK_END);
    int rom_size = playdate->file->tell(rom_file);
    
    if (rom_size < 0x150)
    {
        playdate->file->close(rom_file);
        playdate->system->logToConsole("File \"%s\" too small: %d bytes", filename, rom_size);
        return NULL;
    }
    
    char header_raw[0x50];
    playdate->file->seek(rom_file, 0x100, SEEK_SET);
    playdate->file->read(rom_file, header_raw, 0x50);
    playdate->file->seek(rom_file, 0, SEEK_SET);
    
    struct gb_header header;
    gb_read_header(&header, header_raw);
    
    playdate->lua->pushString(header.title);
    
    const char* err;
    playdate->lua->callFunction("on_load_rom", 1, &err);
    
    uint8_t *rom = pgb_malloc(rom_size);
    
    if(playdate->file->read(rom_file, rom, rom_size) != rom_size)
    {
        playdate->system->logToConsole("%s:%i: Can't read rom file %s", __FILE__, __LINE__, filename);
        
        pgb_free(rom);
        playdate->file->close(rom_file);
        *sceneError = PGB_GameSceneErrorLoadingRom;
        return NULL;
    }

    playdate->file->close(rom_file);
    return rom;
}

static void read_cart_ram_file(const char *save_filename, uint8_t **dest, const size_t len){

    /* If save file not required. */
    if(len == 0)
    {
        *dest = NULL;
        return;
    }

    /* Allocate enough memory to hold save file. */
    if((*dest = pgb_malloc(len)) == NULL)
    {
        playdate->system->logToConsole("%s:%i: Error allocating save file %s", __FILE__, __LINE__, save_filename);
    }

    SDFile *f = playdate->file->open(save_filename, kFileReadData);

    /* It doesn't matter if the save file doesn't exist. We initialise the
     * save memory allocated above. The save file will be created on exit. */
    if(f == NULL)
    {
        memset(*dest, 0, len);
        return;
    }

    /* Read save file to allocated memory. */
    playdate->file->read(f, *dest, (unsigned int)len);
    playdate->file->close(f);
}

static void write_cart_ram_file(const char *save_filename, uint8_t **dest, const size_t len)
{
    if(len == 0 || *dest == NULL)
    {
        return;
    }
    
    SDFile *f = playdate->file->open(save_filename, kFileWrite);
    
    if(f == NULL)
    {
        playdate->system->logToConsole("%s:%i: Can't write save file", __FILE__, __LINE__, save_filename);
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
static void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t val)
{
    PGB_GameSceneContext *context = gb->direct.priv;
    
    bool is_fatal = false;
    
    if(gb_err == GB_INVALID_OPCODE)
    {
        is_fatal = true;
        
        playdate->system->logToConsole("%s:%i: Invalid opcode %#04x at PC: %#06x, SP: %#06x", __FILE__, __LINE__, val, gb->cpu_reg.pc - 1, gb->cpu_reg.sp);
    }
    else if(gb_err == GB_INVALID_READ || gb_err == GB_INVALID_WRITE)
    {
        playdate->system->logToConsole("%s:%i: Invalid read / write", __FILE__, __LINE__);
    }
    else
    {
        is_fatal = true;
        playdate->system->logToConsole("%s:%i: Unknown error occurred", __FILE__, __LINE__);
    }
    
    if(is_fatal)
    {
        // write recovery .sav
        char *recovery_filename = pgb_save_filename(context->scene->rom_filename, true);
        write_cart_ram_file(recovery_filename, &context->gb->gb_cart_ram, gb_get_save_size(gb));
        
        pgb_free(recovery_filename);
        
        context->scene->state = PGB_GameSceneStateError;
        context->scene->error = PGB_GameSceneErrorFatal;
        
        PGB_Scene_refreshMenu(context->scene->scene);
    }

    return;
}

typedef typeof(playdate->graphics->markUpdatedRows) markUpdateRows_t;

__core
void update_fb(uint8_t* restrict framebuffer, uint8_t* restrict lcd, int interlace, markUpdateRows_t markUpdateRows)
{
    framebuffer += (PGB_LCD_X/8);
    const u32 dither = 0b00011111 | (0b00001011 << 8);
    int scale_index = 0;
    unsigned fb_y = PGB_LCD_HEIGHT + PGB_LCD_Y;
    for (int y = LCD_HEIGHT; y --> 0;)
    {
        // some scanlines are 2 px in thickness, some are 1.
        int row_height = 2;
        if (scale_index++ == 2)
        {
            scale_index = 0;
            row_height = 1;
        }
        
        uint8_t* restrict line = &lcd[y*LCD_WIDTH_PACKED];
        fb_y -= row_height;
        uint8_t* restrict fbline = &framebuffer[fb_y*PLAYDATE_ROW_STRIDE];
        
        if (interlace++ % 2) continue;
        
        for (int x = LCD_WIDTH_PACKED; x --> 0;)
        {
            uint8_t orgpixels = line[x];
            uint8_t pixels = orgpixels;
            
            // output pixel
            unsigned p = 0;
            
            for (int i = 0; i < 4; ++i)
            {
                p <<= 2;
                unsigned c0 = (dither >> (2*(pixels&3))) & 3;
                p |= c0;
                
                pixels >>= 2;
            }
            
            u8* restrict fbpix0 = fbline + x;
            
            *fbpix0 = p & 0xFF;
            
            if (row_height == 2)
            {
                pixels = orgpixels;
                fbpix0 += PLAYDATE_ROW_STRIDE;
                for (int i = 0; i < 4; ++i)
                {
                    p <<= 2;
                    unsigned c1 = (dither >> (2*(pixels&3) + 8)) & 3;
                    p |= c1;
                    
                    pixels >>= 2;
                }
                *fbpix0 = p & 0xFF;
            }
        }
        
        markUpdateRows(fb_y, fb_y + row_height - 1);
    }
}

__space
static void PGB_GameScene_update(void *object)
{
    PGB_GameScene *gameScene = object;
    
    PGB_Scene_update(gameScene->scene);
            
    float progress = 0.5f;
    
    gameScene->selector.startPressed = false;
    gameScene->selector.selectPressed = false;
    
    if(!playdate->system->isCrankDocked())
    {
        float angle = fmaxf(0, fminf(360, playdate->system->getCrankAngle()));
        
        if(angle <= (180 - gameScene->selector.deadAngle))
        {
            if(angle >= gameScene->selector.triggerAngle)
            {
                gameScene->selector.startPressed = true;
            }
            
            float adjustedAngle = fminf(angle, gameScene->selector.triggerAngle);
            progress = 0.5f - adjustedAngle / gameScene->selector.triggerAngle * 0.5f;
        }
        else if(angle >= (180 + gameScene->selector.deadAngle))
        {
            if(angle <= (360 - gameScene->selector.triggerAngle))
            {
                gameScene->selector.selectPressed = true;
            }
            
            float adjustedAngle = fminf(360 - angle, gameScene->selector.triggerAngle);
            progress = 0.5f + adjustedAngle / gameScene->selector.triggerAngle * 0.5f;
        }
        else {
            gameScene->selector.startPressed = true;
            gameScene->selector.selectPressed = true;
        }
    }
    
    int selectorIndex;
    
    if(gameScene->selector.startPressed && gameScene->selector.selectPressed)
    {
        selectorIndex = -1;
    }
    else {
        selectorIndex = 1 + roundf(progress * (gameScene->selector.numberOfFrames - 2));
        
        if(progress == 0)
        {
            selectorIndex = 0;
        }
        else if(progress == 1)
        {
            selectorIndex = gameScene->selector.numberOfFrames - 1;
        }
    }
    
    gameScene->selector.index = selectorIndex;
    
    bool needsDisplay = false;
    
    if(gameScene->model.empty || gameScene->needsDisplay || gameScene->model.state != gameScene->state || gameScene->model.error != gameScene->error)
    {
        needsDisplay = true;
    }
    
    bool needsDisplaySelector = false;
    if(needsDisplay || gameScene->model.selectorIndex != gameScene->selector.index)
    {
        needsDisplaySelector = true;
    }

    gameScene->model.empty = false;
    gameScene->model.state = gameScene->state;
    gameScene->model.error = gameScene->error;
    gameScene->model.selectorIndex = gameScene->selector.index;
    gameScene->needsDisplay = false;
    
    if(gameScene->state == PGB_GameSceneStateLoaded)
    {
        PGB_GameSceneContext *context = gameScene->context;
                
        PDButtons current;
        playdate->system->getButtonState(&current, NULL, NULL);
        
        context->gb->direct.joypad_bits.start = !(gameScene->selector.startPressed);
        context->gb->direct.joypad_bits.select = !(gameScene->selector.selectPressed);
           
        context->gb->direct.joypad_bits.a = !(current & kButtonA);
        context->gb->direct.joypad_bits.b = !(current & kButtonB);
        context->gb->direct.joypad_bits.left = !(current & kButtonLeft);
        context->gb->direct.joypad_bits.up = !(current & kButtonUp);
        context->gb->direct.joypad_bits.right = !(current & kButtonRight);
        context->gb->direct.joypad_bits.down = !(current & kButtonDown);
        
        if(needsDisplay)
        {
            playdate->graphics->clear(kColorBlack);
        }
        
        #if PGB_DEBUG && PGB_DEBUG_UPDATED_ROWS
        memset(gameScene->debug_updatedRows, 0, LCD_ROWS);
        #endif
        
        #ifdef DTCM_ALLOC
        ITCM_CORE_FN(gb_run_frame)(context->gb);
        #else
        // copy gb to DTCM temporarily
        struct gb_s gb;
        memcpy(&gb, context->gb, sizeof(struct gb_s));
        
        gb_run_frame(&gb);
        
        memcpy(context->gb, &gb, sizeof(struct gb_s));
        #endif
        
        bool gb_draw = context->gb->lcd_master_enable && (!context->gb->direct.frame_skip || !context->gb->display.frame_skip_count || needsDisplay);
        
        gameScene->scene->preferredRefreshRate = gb_draw ? 60 : 0;
        gameScene->scene->refreshRateCompensation = gb_draw ? (1.0f / 60 - PGB_App->dt) : 0;
        
        if(gb_draw)
        {
            uint8_t* lcd = context->gb->lcd;
            static int interlace;
            
            interlace = !interlace;
            
            ITCM_CORE_FN(update_fb)(
                playdate->graphics->getFrame(),
                lcd,
                interlace,
                playdate->graphics->markUpdatedRows
            );
        }
        # if 0
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
        
        gameScene->rtc_timer += PGB_App->dt;
        
        if(gameScene->rtc_timer >= 1)
        {
            gameScene->rtc_timer = gameScene->rtc_timer - 1;
            gb_tick_rtc(context->gb);
        }

        if(needsDisplay)
        {
            playdate->graphics->setFont(PGB_App->labelFont);
            playdate->graphics->setDrawMode(kDrawModeFillWhite);
            
            playdate->graphics->drawText(startButtonText, strlen(startButtonText), kUTF8Encoding, gameScene->selector.startButtonX, gameScene->selector.startButtonY);
            playdate->graphics->drawText(selectButtonText, strlen(selectButtonText), kUTF8Encoding, gameScene->selector.selectButtonX, gameScene->selector.selectButtonY);
            
            playdate->graphics->setDrawMode(kDrawModeCopy);
        }
        
        if(needsDisplaySelector)
        {
            LCDBitmap *bitmap;
            
            if(selectorIndex < 0)
            {
                bitmap = PGB_App->startSelectBitmap;
            }
            else
            {
                bitmap = playdate->graphics->getTableBitmap(PGB_App->selectorBitmapTable, selectorIndex);
            }
            
            playdate->graphics->drawBitmap(bitmap, gameScene->selector.x, gameScene->selector.y, kBitmapUnflipped);
        }
        
        #if PGB_DEBUG && PGB_DEBUG_UPDATED_ROWS
        PDRect highlightFrame = gameScene->debug_highlightFrame;
        playdate->graphics->fillRect(highlightFrame.x, highlightFrame.y, highlightFrame.width, highlightFrame.height, kColorBlack);
        
        for(int y = 0; y < PGB_LCD_HEIGHT; y++)
        {
            int absoluteY = PGB_LCD_Y + y;
            
            if(gameScene->debug_updatedRows[absoluteY])
            {
                playdate->graphics->fillRect(highlightFrame.x, absoluteY, highlightFrame.width, 1, kColorWhite);
            }
        }
        #endif
        
        if(preferences_display_fps)
        {
            playdate->system->drawFPS(0, 0);
        }
    }
    else if(gameScene->state == PGB_GameSceneStateError)
    {
        gameScene->scene->preferredRefreshRate = 30;
        gameScene->scene->refreshRateCompensation = 0;
        
        if(needsDisplay)
        {
            char *errorTitle = "Oh no!";
            
            int errorMessagesCount = 1;
            char *errorMessages[4];
            
            errorMessages[0] = "A generic error occurred";
            
            if(gameScene->error == PGB_GameSceneErrorLoadingRom)
            {
                errorMessages[0] = "Can't load the selected ROM";
            }
            else if(gameScene->error == PGB_GameSceneErrorWrongLocation)
            {
                errorTitle = "Wrong location";
                errorMessagesCount = 2;
                errorMessages[0] = "Please move the ROM to";
                errorMessages[1] = "/Data/*.playgb/games/";
            }
            else if(gameScene->error == PGB_GameSceneErrorFatal)
            {
                errorMessages[0] = "A fatal error occurred";
            }
            
            playdate->graphics->clear(kColorWhite);
            
            int titleToMessageSpacing = 6;
            
            int titleHeight = playdate->graphics->getFontHeight(PGB_App->titleFont);
            int lineSpacing = 2;
            int messageHeight = playdate->graphics->getFontHeight(PGB_App->bodyFont);
            int messagesHeight = messageHeight * errorMessagesCount + lineSpacing * (errorMessagesCount - 1);
            
            int containerHeight = titleHeight + titleToMessageSpacing + messagesHeight;
            
            int titleX = (float)(playdate->display->getWidth() - playdate->graphics->getTextWidth(PGB_App->titleFont, errorTitle, strlen(errorTitle), kUTF8Encoding, 0)) / 2;
            int titleY = (float)(playdate->display->getHeight() - containerHeight) / 2;
            
            playdate->graphics->setFont(PGB_App->titleFont);
            playdate->graphics->drawText(errorTitle, strlen(errorTitle), kUTF8Encoding, titleX, titleY);
            
            int messageY = titleY + titleHeight + titleToMessageSpacing;
            
            for(int i = 0; i < errorMessagesCount; i++)
            {
                char *errorMessage = errorMessages[i];
                int messageX = (float)(playdate->display->getWidth() - playdate->graphics->getTextWidth(PGB_App->bodyFont, errorMessage, strlen(errorMessage), kUTF8Encoding, 0)) / 2;
                
                playdate->graphics->setFont(PGB_App->bodyFont);
                playdate->graphics->drawText(errorMessage, strlen(errorMessage), kUTF8Encoding, messageX, messageY);
                
                messageY += messageHeight + lineSpacing;
            }
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
    
    PGB_LibraryScene *libraryScene = PGB_LibraryScene_new();
    PGB_present(libraryScene->scene);
}

static void PGB_GameScene_didToggleLCD(void *userdata)
{
    PGB_GameScene *gameScene = userdata;
    
    gameScene->context->gb->lcd_master_enable ^= 1;
}

static void PGB_GameScene_menu(void *object)
{
    PGB_GameScene *gameScene = object;
    
    playdate->system->addMenuItem("Library", PGB_GameScene_didSelectLibrary, gameScene);
    
    if(gameScene->state == PGB_GameSceneStateLoaded)
    {
        playdate->system->addMenuItem("Save", PGB_GameScene_didSelectSave, gameScene);
        
        playdate->system->addCheckmarkMenuItem("LCD", 1, PGB_GameScene_didToggleLCD, gameScene);
    }
}

static void PGB_GameScene_saveGame(PGB_GameScene *gameScene)
{
    if(gameScene->state == PGB_GameSceneStateLoaded)
    {
        PGB_GameSceneContext *context = gameScene->context;
        
        if(gameScene->save_filename)
        {
            write_cart_ram_file(gameScene->save_filename, &context->gb->gb_cart_ram, gb_get_save_size(context->gb));
        }
    }
}

static void PGB_GameScene_generateBitmask(void)
{
    if(PGB_GameScene_bitmask_done)
    {
        return;
    }
    
    PGB_GameScene_bitmask_done = true;
    
    for(int palette = 0; palette < 4; palette++)
    {
        for(int y = 0; y < 4; y++)
        {
            int x_offset = 0;
            
            for(int i = 0; i < 4; i++)
            {
                int mask = 0x00;
                
                for(int x = 0; x < 2; x++){
                    if(PGB_patterns[palette][y][x_offset + x] == 1)
                    {
                        int n = i * 2 + x;
                        mask |= (1 << (7 - n));
                    }
                }
                
                PGB_bitmask[palette][i][y] = mask;
                
                x_offset += 2;
                
                if(x_offset == 4)
                {
                    x_offset = 0;
                }
            }
        }
    }
}

static void PGB_GameScene_free(void *object)
{
    PGB_GameScene *gameScene = object;
    PGB_GameSceneContext *context = gameScene->context;
    
    audioGameScene = NULL;
    
    PGB_Scene_free(gameScene->scene);
    
    PGB_GameScene_saveGame(gameScene);
        
    gb_reset(context->gb);
    
    pgb_free(gameScene->rom_filename);
    
    if(gameScene->save_filename)
    {
        pgb_free(gameScene->save_filename);
    }
    
    if(context->rom)
    {
        pgb_free(context->rom);
    }
    
    if(context->cart_ram)
    {
        pgb_free(context->cart_ram);
    }
    
    pgb_free(context);
    pgb_free(gameScene);
}

void user_breakpoint(struct gb_s* gb, void* ud)
{
}