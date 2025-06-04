//
//  scene.c
//  PlayGB
//
//  Created by Matteo D'Ignazio on 14/05/22.
//

#include "scene.h"

#include "app.h"

static void PGB_Scene_menu_callback(void *object);
static void PGB_Scene_event(void *object, PDSystemEvent event, uint32_t arg);

PGB_Scene *PGB_Scene_new(void)
{
    PGB_Scene *scene = pgb_malloc(sizeof(PGB_Scene));

    scene->update = PGB_Scene_update;
    scene->menu = PGB_Scene_menu_callback;
    scene->free = PGB_Scene_free;
    scene->event = PGB_Scene_event;

    scene->preferredRefreshRate = 30;
    scene->refreshRateCompensation = 0;

    // extra stack space, to avoid stack overflow.
    // (We use most of the normal stack for DTCM.)
    scene->use_user_stack = 1;

    return scene;
}

void PGB_Scene_update(void *object)
{
}

static void PGB_Scene_menu_callback(void *object)
{
}

static void PGB_Scene_event(void *object, PDSystemEvent event, uint32_t arg)
{
}

void PGB_Scene_refreshMenu(PGB_Scene *scene)
{
    playdate->system->removeAllMenuItems();
    scene->menu(PGB_App->scene->managedObject);
}

void PGB_Scene_free(void *object)
{
    PGB_Scene *scene = object;
    pgb_free(scene);
}
