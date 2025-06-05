//
//  app.c
//  PlayGB
//
//  Created by Matteo D'Ignazio on 14/05/22.
//

#include "app.h"

#include "../minigb_apu/minigb_apu.h"
#include "dtcm.h"
#include "game_scene.h"
#include "library_scene.h"
#include "preferences.h"
#include "userstack.h"

PGB_Application *PGB_App;

void PGB_init(void)
{
    PGB_App = pgb_malloc(sizeof(PGB_Application));

    PGB_App->scene = NULL;
    PGB_App->pendingScene = NULL;

    playdate->file->mkdir("games");
    playdate->file->mkdir("covers");
    playdate->file->mkdir("saves");

    preferences_init();

    PGB_App->bodyFont =
        playdate->graphics->loadFont("fonts/Roobert-11-Medium", NULL);
    PGB_App->titleFont =
        playdate->graphics->loadFont("fonts/Roobert-20-Medium", NULL);
    PGB_App->subheadFont =
        playdate->graphics->loadFont("fonts/Asheville-Sans-14-Bold", NULL);
    PGB_App->labelFont =
        playdate->graphics->loadFont("fonts/Nontendo-Bold", NULL);

    PGB_App->selectorBitmapTable =
        playdate->graphics->loadBitmapTable("images/selector/selector", NULL);
    PGB_App->startSelectBitmap =
        playdate->graphics->loadBitmap("images/selector-start-select", NULL);

    // add audio callback later
    PGB_App->soundSource = NULL;

    // custom frame rate delimiter
    playdate->display->setRefreshRate(0);

    PGB_LibraryScene *libraryScene = PGB_LibraryScene_new();
    PGB_present(libraryScene->scene);
}

__section__(".rare") static void switchToPendingScene(void)
{
    // present pending scene

    if (PGB_App->scene)
    {
        preferences_save_to_disk();

        void *managedObject = PGB_App->scene->managedObject;
        PGB_App->scene->free(managedObject);
    }

    PGB_App->scene = PGB_App->pendingScene;
    PGB_App->pendingScene = NULL;

    PGB_Scene_refreshMenu(PGB_App->scene);
}

__section__(".text.main") void PGB_update(float dt)
{
    PGB_App->dt = dt;

    PGB_App->crankChange = playdate->system->getCrankChange();

    if (PGB_App->scene)
    {
        void *managedObject = PGB_App->scene->managedObject;
        DTCM_VERIFY_DEBUG();
        if (PGB_App->scene->use_user_stack)
        {
            call_with_user_stack_1(PGB_App->scene->update, managedObject);
        }
        else
        {
            PGB_App->scene->update(managedObject);
        }
        DTCM_VERIFY_DEBUG();
    }

    if (PGB_App->pendingScene)
    {
        DTCM_VERIFY();
        call_with_user_stack(switchToPendingScene);
        DTCM_VERIFY();
    }

#if PGB_DEBUG
    playdate->display->setRefreshRate(60);
#else

    float refreshRate = 30;
    float compensation = 0;

    if (PGB_App->scene)
    {
        refreshRate = PGB_App->scene->preferredRefreshRate;
        compensation = PGB_App->scene->refreshRateCompensation;
    }

    if (refreshRate > 0)
    {
        float refreshInterval = 1.0f / refreshRate + compensation;
        while (playdate->system->getElapsedTime() < refreshInterval)
            ;
    }

#endif
    DTCM_VERIFY_DEBUG();
}

void PGB_present(PGB_Scene *scene)
{
    PGB_App->pendingScene = scene;
}

void PGB_goToLibrary(void)
{
    PGB_LibraryScene *libraryScene = PGB_LibraryScene_new();
    PGB_present(libraryScene->scene);
}

__section__(".rare") void PGB_event(PDSystemEvent event, uint32_t arg)
{
    PGB_ASSERT(PGB_App);
    if (PGB_App->scene)
    {
        PGB_ASSERT(PGB_App->scene->event != NULL);
        PGB_App->scene->event(PGB_App->scene->managedObject, event, arg);
    }
}

void PGB_quit(void)
{
    preferences_save_to_disk();

    if (PGB_App->scene)
    {
        void *managedObject = PGB_App->scene->managedObject;
        PGB_App->scene->free(managedObject);
    }
}
