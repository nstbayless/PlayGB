//
//  game_scene.h
//  CrankBoy
//
//  Created by Matteo D'Ignazio on 14/05/22.
//  Maintained and developed by the CrankBoy dev team.
//

#ifndef game_scene_h
#define game_scene_h

#include <math.h>
#include <stdio.h>

#include "peanut_gb.h"
#include "scene.h"

typedef struct PGB_GameSceneContext PGB_GameSceneContext;
typedef struct PGB_GameScene PGB_GameScene;

extern PGB_GameScene *audioGameScene;

typedef enum
{
    PGB_GameSceneStateLoaded,
    PGB_GameSceneStateError
} PGB_GameSceneState;

typedef enum
{
    PGB_GameSceneErrorUndefined,
    PGB_GameSceneErrorLoadingRom,
    PGB_GameSceneErrorWrongLocation,
    PGB_GameSceneErrorFatal
} PGB_GameSceneError;

typedef struct
{
    PGB_GameSceneState state;
    PGB_GameSceneError error;
    int selectorIndex;
    bool empty;
} PGB_GameSceneModel;

typedef struct
{
    int width;
    int height;
    int containerWidth;
    int containerHeight;
    int containerX;
    int containerY;
    int x;
    int y;
    int startButtonX;
    int startButtonY;
    int selectButtonX;
    int selectButtonY;
    int numberOfFrames;
    float triggerAngle;
    float deadAngle;
    float index;
    bool startPressed;
    bool selectPressed;
} PGB_CrankSelector;

struct gb_s;

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

    int buttons_held_since_start;  // buttons that have been down since the
                                   // start of the game
} PGB_GameSceneContext;

typedef struct PGB_GameScene
{
    PGB_Scene *scene;
    char *save_filename;
    char *rom_filename;

    bool audioEnabled;
    bool audioLocked;
    bool cartridge_has_rtc;
    bool staticSelectorUIDrawn;
    bool save_data_loaded_successfully;

    unsigned int rtc_time;
    uint16_t rtc_seconds_to_catch_up;

    float rtc_fractional_second_accumulator;

    PGB_GameSceneState state;
    PGB_GameSceneContext *context;
    PGB_GameSceneModel model;
    PGB_GameSceneError error;

    PGB_CrankSelector selector;

#if PGB_DEBUG && PGB_DEBUG_UPDATED_ROWS
    PDRect debug_highlightFrame;
    bool debug_updatedRows[LCD_ROWS];
#endif

    lua_State *script;
} PGB_GameScene;

PGB_GameScene *PGB_GameScene_new(const char *rom_filename);

#endif /* game_scene_h */
