//
//  array.c
//  CrankBoy
//
//  Created by Matteo D'Ignazio on 23/08/21.
//  Maintained and developed by the CrankBoy dev team.
//

#ifndef array_h
#define array_h

#include <stdbool.h>
#include <stdio.h>

typedef struct
{
    unsigned int length;
    void **items;
} PGB_Array;

PGB_Array *array_new(void);

void array_push(PGB_Array *array, void *item);
void array_clear(PGB_Array *array);
void array_free(PGB_Array *array);

#endif /* array_h */
