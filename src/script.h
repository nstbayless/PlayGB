#pragma once

struct PGB_GameScene;

lua_State *script_begin(const char *game_name,
                        struct PGB_GameScene *game_scene);
void script_end(lua_State *L);
void script_tick(lua_State *L);
void script_on_breakpoint(lua_State *, int index);