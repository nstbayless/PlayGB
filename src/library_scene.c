//
//  library_scene.c
//  CrankBoy
//
//  Created by Matteo D'Ignazio on 15/05/22.
//  Maintained and developed by the CrankBoy dev team.
//

#include "library_scene.h"

#include "app.h"
#include "dtcm.h"
#include "game_scene.h"
#include "minigb_apu.h"
#include "preferences.h"

static void PGB_LibraryScene_update(void *object);
static void PGB_LibraryScene_free(void *object);
static void PGB_LibraryScene_reloadList(PGB_LibraryScene *libraryScene);
static void PGB_LibraryScene_menu(void *object);

static PDMenuItem *audioMenuItem;
static PDMenuItem *fpsMenuItem;
static PDMenuItem *frameSkipMenuItem;

PGB_LibraryScene *PGB_LibraryScene_new(void)
{
    playdate->system->setCrankSoundsDisabled(false);

    PGB_Scene *scene = PGB_Scene_new();

    PGB_LibraryScene *libraryScene = pgb_malloc(sizeof(PGB_LibraryScene));
    libraryScene->scene = scene;

    DTCM_VERIFY_DEBUG();

    scene->managedObject = libraryScene;

    scene->update = PGB_LibraryScene_update;
    scene->free = PGB_LibraryScene_free;
    scene->menu = PGB_LibraryScene_menu;

    libraryScene->model =
        (PGB_LibrarySceneModel){.empty = true, .tab = PGB_LibrarySceneTabList};

    libraryScene->games = array_new();
    libraryScene->listView = PGB_ListView_new();
    libraryScene->tab = PGB_LibrarySceneTabList;
    libraryScene->lastSelectedItem = -1;

    DTCM_VERIFY_DEBUG();

    PGB_LibraryScene_reloadList(libraryScene);

    return libraryScene;
}

static void PGB_LibraryScene_listFiles(const char *filename, void *userdata)
{
    PGB_LibraryScene *libraryScene = userdata;

    char *extension;
    char *dot = pgb_strrchr(filename, '.');

    if (!dot || dot == filename)
    {
        extension = "";
    }
    else
    {
        extension = dot + 1;
    }

    if ((pgb_strcmp(extension, "gb") == 0 || pgb_strcmp(extension, "gbc") == 0))
    {
        PGB_Game *game = PGB_Game_new(filename);
        array_push(libraryScene->games, game);
    }
}

static void PGB_LibraryScene_reloadList(PGB_LibraryScene *libraryScene)
{
    for (int i = 0; i < libraryScene->games->length; i++)
    {
        PGB_Game *game = libraryScene->games->items[i];
        PGB_Game_free(game);
    }

    array_clear(libraryScene->games);

    DTCM_VERIFY();

    playdate->file->listfiles(PGB_gamesPath, PGB_LibraryScene_listFiles,
                              libraryScene, 0);

    DTCM_VERIFY();
    pgb_sort_games_array(libraryScene->games);
    DTCM_VERIFY_DEBUG();

    PGB_Array *items = libraryScene->listView->items;

    for (int i = 0; i < items->length; i++)
    {
        PGB_ListItem *item = items->items[i];
        PGB_ListItem_free(item);
    }

    array_clear(items);

    for (int i = 0; i < libraryScene->games->length; i++)
    {
        PGB_Game *game = libraryScene->games->items[i];

        PGB_ListItemButton *itemButton =
            PGB_ListItemButton_new(game->displayName);
        array_push(items, itemButton->item);
    }

    if (items->length > 0)
    {
        libraryScene->tab = PGB_LibrarySceneTabList;
    }
    else
    {
        libraryScene->tab = PGB_LibrarySceneTabEmpty;
    }

    DTCM_VERIFY_DEBUG();

    PGB_ListView_reload(libraryScene->listView);
}

static void PGB_LibraryScene_update(void *object)
{
    PGB_LibraryScene *libraryScene = object;

    PGB_Scene_update(libraryScene->scene);

    PDButtons released;
    playdate->system->getButtonState(NULL, NULL, &released);

    if (released & kButtonA)
    {
        int selectedItem = libraryScene->listView->selectedItem;
        if (selectedItem >= 0 &&
            selectedItem < libraryScene->listView->items->length)
        {

            PGB_Game *game = libraryScene->games->items[selectedItem];

            PGB_GameScene *gameScene = PGB_GameScene_new(game->fullpath);
            if (gameScene)
            {
                PGB_present(gameScene->scene);
            }
        }
    }

    bool needsDisplay = false;

    if (libraryScene->model.empty ||
        libraryScene->model.tab != libraryScene->tab)
    {
        needsDisplay = true;
    }

    libraryScene->model.empty = false;
    libraryScene->model.tab = libraryScene->tab;

    if (needsDisplay)
    {
        playdate->graphics->clear(kColorWhite);
    }

    if (libraryScene->tab == PGB_LibrarySceneTabList)
    {
        int screenWidth = playdate->display->getWidth();
        int screenHeight = playdate->display->getHeight();

        int rightPanelWidth = 241;
        int leftPanelWidth = screenWidth - rightPanelWidth;

        libraryScene->listView->needsDisplay = needsDisplay;
        libraryScene->listView->frame =
            PDRectMake(0, 0, leftPanelWidth, screenHeight);

        PGB_ListView_update(libraryScene->listView);
        PGB_ListView_draw(libraryScene->listView);

        int selectedIndex = libraryScene->listView->selectedItem;

        bool selectionChanged =
            (selectedIndex != libraryScene->lastSelectedItem);

        if (needsDisplay || libraryScene->listView->needsDisplay ||
            selectionChanged)
        {
            libraryScene->lastSelectedItem = selectedIndex;

            playdate->graphics->fillRect(leftPanelWidth + 1, 0,
                                         rightPanelWidth - 1, screenHeight,
                                         kColorWhite);

            if (selectedIndex >= 0 &&
                selectedIndex < libraryScene->games->length)
            {
                PGB_Game *selectedGame =
                    libraryScene->games->items[selectedIndex];

                if (selectedGame->coverPath != NULL)
                {
                    PGB_LoadedCoverArt cover_art =
                        pgb_load_and_scale_cover_art_from_path(
                            selectedGame->coverPath, 240, 240);

                    if (cover_art.status == PGB_COVER_ART_SUCCESS &&
                        cover_art.bitmap != NULL)
                    {
                        int panel_content_width = rightPanelWidth - 1;
                        int coverX =
                            leftPanelWidth + 1 +
                            (panel_content_width - cover_art.scaled_width) / 2;
                        int coverY =
                            (screenHeight - cover_art.scaled_height) / 2;

                        playdate->graphics->setDrawMode(kDrawModeCopy);
                        playdate->graphics->drawBitmap(
                            cover_art.bitmap, coverX, coverY, kBitmapUnflipped);
                    }
                    else
                    {
                        const char *message = "Error";
                        if (cover_art.status == PGB_COVER_ART_FILE_NOT_FOUND)
                        {
                            message = "Cover not found";
                            playdate->system->logToConsole(
                                "Cover %s not found by load func.",
                                selectedGame->coverPath);
                        }
                        else if (cover_art.status ==
                                 PGB_COVER_ART_ERROR_LOADING)
                        {
                            message = "Error loading image";
                        }
                        else if (cover_art.status ==
                                 PGB_COVER_ART_INVALID_IMAGE)
                        {
                            message = "Invalid image";
                        }

                        playdate->graphics->setFont(PGB_App->bodyFont);
                        int textWidth = playdate->graphics->getTextWidth(
                            PGB_App->bodyFont, message, pgb_strlen(message),
                            kUTF8Encoding, 0);
                        int panel_content_width = rightPanelWidth - 1;
                        int textX = leftPanelWidth + 1 +
                                    (panel_content_width - textWidth) / 2;
                        int textY =
                            (screenHeight - playdate->graphics->getFontHeight(
                                                PGB_App->bodyFont)) /
                            2;

                        playdate->graphics->setDrawMode(kDrawModeFillBlack);
                        playdate->graphics->drawText(
                            message, pgb_strlen(message), kUTF8Encoding, textX,
                            textY);
                    }
                    pgb_free_loaded_cover_art_bitmap(&cover_art);
                }
                else
                {
                    const char *message = "Missing cover";
                    playdate->graphics->setFont(PGB_App->bodyFont);
                    int textWidth = playdate->graphics->getTextWidth(
                        PGB_App->bodyFont, message, pgb_strlen(message),
                        kUTF8Encoding, 0);
                    int panel_content_width = rightPanelWidth - 1;
                    int textX = leftPanelWidth + 1 +
                                (panel_content_width - textWidth) / 2;
                    int textY =
                        (screenHeight -
                         playdate->graphics->getFontHeight(PGB_App->bodyFont)) /
                        2;

                    playdate->graphics->setDrawMode(kDrawModeFillBlack);
                    playdate->graphics->drawText(message, pgb_strlen(message),
                                                 kUTF8Encoding, textX, textY);
                }
            }
            playdate->graphics->drawLine(leftPanelWidth, 0, leftPanelWidth,
                                         screenHeight, 1, kColorBlack);
        }
    }
    else if (libraryScene->tab == PGB_LibrarySceneTabEmpty)
    {
        if (needsDisplay)
        {
            static const char *title = "CrankBoy";
            static const char *message1 = "Connect to a computer and";
            static const char *message2 = "copy games to Data/*.crankboy/games";

            playdate->graphics->clear(kColorWhite);

            int titleToMessageSpacing = 6;

            int titleHeight =
                playdate->graphics->getFontHeight(PGB_App->titleFont);
            int messageHeight =
                playdate->graphics->getFontHeight(PGB_App->bodyFont);
            int messageLineSpacing = 2;

            int containerHeight = titleHeight + titleToMessageSpacing +
                                  messageHeight * 2 + messageLineSpacing;
            int titleY =
                (float)(playdate->display->getHeight() - containerHeight) / 2;

            int titleX = (float)(playdate->display->getWidth() -
                                 playdate->graphics->getTextWidth(
                                     PGB_App->titleFont, title,
                                     pgb_strlen(title), kUTF8Encoding, 0)) /
                         2;
            int message1_X =
                (float)(playdate->display->getWidth() -
                        playdate->graphics->getTextWidth(
                            PGB_App->bodyFont, message1, pgb_strlen(message1),
                            kUTF8Encoding, 0)) /
                2;
            int message2_X =
                (float)(playdate->display->getWidth() -
                        playdate->graphics->getTextWidth(
                            PGB_App->bodyFont, message2, pgb_strlen(message2),
                            kUTF8Encoding, 0)) /
                2;

            int message1_Y = titleY + titleHeight + titleToMessageSpacing;
            int message2_Y = message1_Y + messageHeight + messageLineSpacing;

            playdate->graphics->setFont(PGB_App->titleFont);
            playdate->graphics->drawText(title, pgb_strlen(title),
                                         kUTF8Encoding, titleX, titleY);

            playdate->graphics->setFont(PGB_App->bodyFont);
            playdate->graphics->drawText(message1, pgb_strlen(message1),
                                         kUTF8Encoding, message1_X, message1_Y);
            playdate->graphics->drawText(message2, pgb_strlen(message2),
                                         kUTF8Encoding, message2_X, message2_Y);
        }
    }
}

static void PGB_LibraryScene_didChangeSound(void *userdata)
{
    preferences_sound_enabled =
        playdate->system->getMenuItemValue(audioMenuItem);
}

static void PGB_LibraryScene_didChangeFPS(void *userdata)
{
    preferences_display_fps = playdate->system->getMenuItemValue(fpsMenuItem);
}

static void PGB_LibraryScene_didChangeFrameSkip(void *userdata)
{
    preferences_frame_skip =
        playdate->system->getMenuItemValue(frameSkipMenuItem);
}

static void PGB_LibraryScene_menu(void *object)
{
    PGB_LibraryScene *libraryScene = object;

    audioMenuItem = playdate->system->addCheckmarkMenuItem(
        "Sound", preferences_sound_enabled, PGB_LibraryScene_didChangeSound,
        libraryScene);
    frameSkipMenuItem = playdate->system->addCheckmarkMenuItem(
        "Frame skip", preferences_frame_skip,
        PGB_LibraryScene_didChangeFrameSkip, libraryScene);
    fpsMenuItem = playdate->system->addCheckmarkMenuItem(
        "Show FPS", preferences_display_fps, PGB_LibraryScene_didChangeFPS,
        libraryScene);
}

static void PGB_LibraryScene_free(void *object)
{
    PGB_LibraryScene *libraryScene = object;

    PGB_Scene_free(libraryScene->scene);

    for (int i = 0; i < libraryScene->games->length; i++)
    {
        PGB_Game *game = libraryScene->games->items[i];
        PGB_Game_free(game);
    }

    PGB_Array *items = libraryScene->listView->items;

    for (int i = 0; i < items->length; i++)
    {
        PGB_ListItem *item = items->items[i];
        PGB_ListItem_free(item);
    }

    PGB_ListView_free(libraryScene->listView);

    array_free(libraryScene->games);

    pgb_free(libraryScene);
}

PGB_Game *PGB_Game_new(const char *filename)
{
    PGB_Game *game = pgb_malloc(sizeof(PGB_Game));
    game->filename = string_copy(filename);

    char *fullpath_str;
    playdate->system->formatString(&fullpath_str, "%s/%s", PGB_gamesPath,
                                   filename);
    game->fullpath = fullpath_str;

    char *basename_no_ext = string_copy(filename);
    char *ext = pgb_strrchr(basename_no_ext, '.');
    if (ext != NULL)
    {
        *ext = '\0';
    }

    game->displayName = string_copy(basename_no_ext);

    char *cleanName_no_ext = string_copy(basename_no_ext);
    pgb_sanitize_string_for_filename(cleanName_no_ext);

    game->coverPath =
        pgb_find_cover_art_path(basename_no_ext, cleanName_no_ext);

    if (game->coverPath)
    {
        playdate->system->logToConsole("Cover for '%s': '%s'",
                                       game->displayName, game->coverPath);
    }
    else
    {
        playdate->system->logToConsole(
            "No cover found for '%s' (basename: '%s', clean: '%s')",
            game->displayName, basename_no_ext, cleanName_no_ext);
    }

    pgb_free(basename_no_ext);
    pgb_free(cleanName_no_ext);

    return game;
}

void PGB_Game_free(PGB_Game *game)
{
    pgb_free(game->filename);
    pgb_free(game->fullpath);
    pgb_free(game->coverPath);
    pgb_free(game->displayName);

    pgb_free(game);
}
