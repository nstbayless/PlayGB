//
//  scene.h
//  CrankBoy
//
//  Created by Matteo D'Ignazio on 14/05/22.
//  Maintained and developed by the CrankBoy dev team.
//

#ifndef scene_h
#define scene_h

#include <stdio.h>

#include "pd_api.h"
#include "utility.h"

typedef struct PGB_Scene
{
    void *managedObject;

    float preferredRefreshRate;
    float refreshRateCompensation;

    bool use_user_stack;

    void (*update)(void *object);
    void (*menu)(void *object);
    void (*free)(void *object);
    void (*event)(void *object, PDSystemEvent event, uint32_t arg);
} PGB_Scene;

PGB_Scene *PGB_Scene_new(void);

void PGB_Scene_refreshMenu(PGB_Scene *scene);

void PGB_Scene_update(void *scene);
void PGB_Scene_free(void *scene);

#endif /* scene_h */
