//
//  main.c
//  PlayGB
//
//  Created by Matteo D'Ignazio on 14/05/22.
//

#include <stdio.h>

#include "./src/app.h"
#include "app.h"
#include "dtcm.h"
#include "pd_api.h"
#include "revcheck.h"

#ifdef _WINDLL
#define DllExport __declspec(dllexport)
#else
#define DllExport
#endif

static int update(void *userdata);
int eventHandler_pdnewlib(PlaydateAPI *, PDSystemEvent event, uint32_t arg);

__section__(".text.main") DllExport
    int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t arg)
{
    eventHandler_pdnewlib(pd, event, arg);

    if (!dtcm_verify())
        return 0;
    
    if (event != kEventInit)
    {
        PGB_event(event, arg);
    }

    if (event == kEventInit)
    {
        pd_revcheck();
        playdate = pd;

        dtcm_set_mempool(__builtin_frame_address(0) - PLAYDATE_STACK_SIZE);

        PGB_init();

        pd->system->setUpdateCallback(update, pd);
    }
    else if (event == kEventTerminate)
    {
        PGB_quit();
    }

    return 0;
}

__section__(".text.main") int update(void *userdata)
{
    PlaydateAPI *pd = userdata;

    float dt = pd->system->getElapsedTime();
    pd->system->resetElapsedTime();

    PGB_update(dt);

    return 1;
}
