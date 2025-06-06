#define lua_State pd_lua_State
#define lua_CFunction pd_lua_CFunction
#include "minigb_apu.h"
#include "pd_api.h"
#undef lua_State
#undef lua_CFunction

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "dtcm.h"
#include "game_scene.h"
#include "script.h"
#include "jparse.h"
#include "../peanut_gb/peanut_gb.h"

#ifndef NOLUA

#define REGISTRY_GAME_SCENE_KEY "PGB_GameScene"

void PGB_GameScene_didSelectLibrary(struct PGB_GameScene *scene);

static bool lua_check_args(lua_State *L, int min, int max)
{
    int argc = lua_gettop(L);
    return argc >= min && argc <= max;
}

static struct PGB_GameScene *get_game_scene(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, REGISTRY_GAME_SCENE_KEY);
    struct PGB_GameScene *scene = (struct PGB_GameScene *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return scene;
}

static struct gb_s *get_gb(lua_State *L)
{
    return get_game_scene(L)->context->gb;
}

static int pgb_rom_poke(lua_State *L)
{
    if (!lua_check_args(L, 2, 2))
    {
        return luaL_error(L, "pgb.rom_poke(addr, value) takes two arguments");
    }

    struct gb_s *gb = get_gb(L);

    int addr = luaL_checkinteger(L, 1);
    int value = luaL_checkinteger(L, 2);
    size_t rom_size = 0x4000 * (gb->num_rom_banks_mask + 1);

    if (addr < 0 || addr >= rom_size)
    {
        return luaL_error(L, "pgb.rom_poke: addr out of range (0-%x)",
                          rom_size - 1);
    }

    gb->gb_rom[addr] = value;
    return 0;
}

int set_hw_breakpoint(struct gb_s *gb, uint32_t rom_addr);
static int pgb_rom_set_breakpoint(lua_State *L)
{
    // returns: breakpoint index, or null on failure
    if (!lua_check_args(L, 2, 2))
    {
        return luaL_error(L, "pgb.rom_set_breakpoint(addr, function) takes two arguments");
    }
    
    struct gb_s* gb = get_gb(L);
    int addr = luaL_checkinteger(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    size_t rom_size = 0x4000 * (gb->num_rom_banks_mask + 1);
    
    if (addr < 0 || addr >= rom_size)
    {
        return luaL_error(L, "pgb.rom_set_breakpoint: addr out of range (0-%x)", rom_size - 1);
    }
    int breakpoint_index = set_hw_breakpoint(gb, addr);
    if (breakpoint_index == -1)
    {
        return luaL_error(L, "pgb.rom_set_breakpoint: too many breakpoints set");
    }
    else if (breakpoint_index < 0)
    {
        return luaL_error(L, "pgb.rom_set_breakpoint: failed to set breakpoint at addr %x", addr);
    }
    
    // store the function in a table in the registry
    lua_getfield(L, LUA_REGISTRYINDEX, "pgb_breakpoints");
    if (!lua_istable(L, -1))
    {
        lua_newtable(L);
        lua_setfield(L, LUA_REGISTRYINDEX, "pgb_breakpoints");
        lua_getfield(L, LUA_REGISTRYINDEX, "pgb_breakpoints");
    }
    
    lua_pushinteger(L, breakpoint_index);
    lua_pushvalue(L, 2);  // push the function
    lua_settable(L, -3);  // set the function in the table with the breakpoint index as key
    
    lua_pop(L, 1);  // pop the table
    lua_pushinteger(L, breakpoint_index);  // return the breakpoint index
    return 1;
}

static int pgb_rom_peek(lua_State *L)
{
    if (!lua_check_args(L, 1, 1))
    {
        return luaL_error(L, "pgb.rom_peek(addr) takes one argument");
    }

    struct gb_s *gb = get_gb(L);

    int addr = luaL_checkinteger(L, 1);
    size_t rom_size = 0x4000 * (gb->num_rom_banks_mask + 1);

    if (addr < 0 || addr >= rom_size)
    {
        return luaL_error(L, "pgb.rom_peek: addr out of range (0-%x)",
                          rom_size - 1);
    }

    lua_pushinteger(L, gb->gb_rom[addr]);
    return 1;
}

uint8_t __gb_read_full(struct gb_s *gb, const uint_fast16_t addr);
void __gb_write_full(struct gb_s *gb, const uint_fast16_t addr, uint8_t);

static int pgb_ram_peek(lua_State *L)
{
    if (!lua_check_args(L, 1, 1))
    {
        return luaL_error(L, "pgb.ram_peek(addr) takes one argument");
    }
    
    struct gb_s* gb = get_gb(L);
    
    int addr = luaL_checkinteger(L, 1);
    
    if (addr < 0 || addr >= 0x10000)
    {
        return luaL_error(L, "pgb.ram_peek: addr out of range (0-FFFF)");
    }
    
    lua_pushinteger(L, __gb_read_full(gb, addr));
    return 1;
}

static int pgb_ram_poke(lua_State *L)
{
    if (!lua_check_args(L, 2, 2))
    {
        return luaL_error(L, "pgb.ram_poke(addr, value) takes two arguments");
    }
    
    struct gb_s* gb = get_gb(L);
    
    int addr = luaL_checkinteger(L, 1);
    int val = luaL_checkinteger(L, 2);
    
    if (addr < 0 || addr >= 0x10000)
    {
        return luaL_error(L, "pgb.ram_peek: addr out of range (0-FFFF)");
    }
    
    __gb_write_full(gb, addr, val);
    return 0;
}

static int pgb_get_gb_buttons(lua_State *L)
{
    struct gb_s* gb = get_gb(L);
    lua_pushinteger(L, gb->direct.joypad ^ 0xFF);
    return 1;
}

static int pgb_get_crank(lua_State *L)
{
    if (playdate->system->isCrankDocked())
        return 0;

    float angle = playdate->system->getCrankAngle();
    lua_pushnumber(L, angle);
    return 1;
}

static int pgb_setCrankSoundsDisabled(lua_State *L)
{
    if (playdate->system->isCrankDocked())
        return 0;

    // get boolean value
    int disabled = lua_toboolean(L, 1);
    playdate->system->setCrankSoundsDisabled(disabled);
    return 0;
}

void __gb_step_cpu(struct gb_s *gb);
static int pgb_step_cpu(lua_State *L)
{
    // UNTESTED
    struct gb_s* gb = get_gb(L);
    __gb_step_cpu(gb);
    return 0;
}

static int pgb_close(lua_State *L)
{
    if (!lua_check_args(L, 0, 0))
    {
        return luaL_error(L, "pgb.close() takes no arguments");
    }
    struct PGB_GameScene *scene = get_game_scene(L);
    if (scene)
    {
        PGB_GameScene_didSelectLibrary(scene);
    }
    return 0;
}

__section__(".rare")
static int pgb_regs_index(lua_State *L)
{
    struct gb_s *gb = get_gb(L);
    const char *reg_name = luaL_checkstring(L, 2);

    if (strcmp(reg_name, "af") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.af);
    }
    else if (strcmp(reg_name, "a") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.a);
    }
    else if (strcmp(reg_name, "f") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.f);
    }
    else if (strcmp(reg_name, "bc") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.bc);
    }
    else if (strcmp(reg_name, "b") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.b);
    }
    else if (strcmp(reg_name, "c") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.c);
    }
    else if (strcmp(reg_name, "de") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.de);
    }
    else if (strcmp(reg_name, "d") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.d);
    }
    else if (strcmp(reg_name, "e") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.e);
    }
    else if (strcmp(reg_name, "hl") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.hl);
    }
    else if (strcmp(reg_name, "h") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.h);
    }
    else if (strcmp(reg_name, "l") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.l);
    }
    else if (strcmp(reg_name, "sp") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.sp);
    }
    else if (strcmp(reg_name, "pc") == 0)
    {
        lua_pushinteger(L, gb->cpu_reg.pc);
    }
    else
    {
        return luaL_error(L, "pgb.regs: unknown register '%s'", reg_name);
    }
    
    return 1;
}

__section__(".rare")
static int pgb_regs_newindex(lua_State *L)
{
    struct gb_s *gb = get_gb(L);
    const char *reg_name = luaL_checkstring(L, 2);
    int value = luaL_checkinteger(L, 3);

    if (strcmp(reg_name, "af") == 0)
    {
        gb->cpu_reg.af = value;
    }
    else if (strcmp(reg_name, "a") == 0)
    {
        gb->cpu_reg.a = value;
    }
    else if (strcmp(reg_name, "f") == 0)
    {
        gb->cpu_reg.f = value;
    }
    else if (strcmp(reg_name, "bc") == 0)
    {
        gb->cpu_reg.bc = value;
    }
    else if (strcmp(reg_name, "b") == 0)
    {
        gb->cpu_reg.b = value;
    }
    else if (strcmp(reg_name, "c") == 0)
    {
        gb->cpu_reg.c = value;
    }
    else if (strcmp(reg_name, "de") == 0)
    {
        gb->cpu_reg.de = value;
    }
    else if (strcmp(reg_name, "d") == 0)
    {
        gb->cpu_reg.d = value;
    }
    else if (strcmp(reg_name, "e") == 0)
    {
        gb->cpu_reg.e = value;
    }
    else if (strcmp(reg_name, "hl") == 0)
    {
        gb->cpu_reg.hl = value;
    }
    else if (strcmp(reg_name, "h") == 0)
    {
        gb->cpu_reg.h = value;
    }
    else if (strcmp(reg_name, "l") == 0)
    {
        gb->cpu_reg.l = value;
    }
    else
    {
        return luaL_error(L, "pgb.regs: unknown register '%s'", reg_name);
    }

    return 0;
}

__section__(".rare")
static void register_pgb_library(lua_State *L)
{
    lua_newtable(L);
    {
        lua_pushcfunction(L, pgb_close);
        lua_setfield(L, -2, "close");

        lua_pushcfunction(L, pgb_rom_poke);
        lua_setfield(L, -2, "rom_poke");

        lua_pushcfunction(L, pgb_rom_peek);
        lua_setfield(L, -2, "rom_peek");
        
        lua_pushcfunction(L, pgb_rom_set_breakpoint);
        lua_setfield(L, -2, "rom_set_breakpoint");
        
        lua_pushcfunction(L, pgb_ram_poke);
        lua_setfield(L, -2, "ram_poke");
        
        lua_pushcfunction(L, pgb_ram_peek);
        lua_setfield(L, -2, "ram_peek");
        
        lua_pushcfunction(L, pgb_get_gb_buttons);
        lua_setfield(L, -2, "get_gb_buttons");
        
        lua_pushcfunction(L, pgb_get_crank);
        lua_setfield(L, -2, "get_crank");

        lua_pushcfunction(L, pgb_setCrankSoundsDisabled);
        lua_setfield(L, -2, "setCrankSoundsDisabled");
        
        lua_pushcfunction(L, pgb_step_cpu);
        lua_setfield(L, -2, "step_cpu");
        
        // pgb.regs
        lua_newtable(L);
        {
            lua_newtable(L);
            lua_pushcfunction(L, pgb_regs_index);
            lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, pgb_regs_newindex);
            lua_setfield(L, -2, "__newindex");
            lua_setmetatable(L, -2);
        }
        lua_setfield(L, -2, "regs");
        
    }
    lua_setglobal(L, "pgb");
}

static void open_sandboxed_libs(lua_State *L)
{
    luaL_requiref(L, "_G", luaopen_base, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);
    lua_pop(L, 1);
    luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1);
    lua_pop(L, 1);
}

static void set_package_path_l(lua_State *L)
{
    lua_getglobal(L, "package");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, "path");
    const char *path = lua_tostring(L, -1);
    lua_pop(L, 1);

    // Replace all ".lua" with ".l"
    const char *from = ".lua";
    const char *to = ".l";
    char *new_path = malloc(strlen(path) * 2);  // generous space
    char *out = new_path;
    const char *p = path;
    while (*p)
    {
        if (strncmp(p, from, 4) == 0)
        {
            strcpy(out, to);
            out += 2;
            p += 4;
        }
        else
        {
            *out++ = *p++;
        }
    }
    *out = '\0';

    lua_pushstring(L, new_path);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
    free(new_path);
}

typedef struct ScriptInfo
{
    char* script_path;
} ScriptInfo;

__section__(".rare")
void script_info_free(ScriptInfo* info)
{
    if (!info) return;
    if (info->script_path) free(info->script_path);
    free(info);
}

__section__(".rare")
ScriptInfo* get_script_info(const char *game_name)
{
    json_value v;
    int ok = parse_json("scripts.json", &v);
    
    if (!ok)
    {
        free_json_data(v);
        return NULL;
    }
    
    // confirm that top-level value is an array
    JsonArray* array = v.data.arrayval;
    if (v.type != kJSONArray || array->n == 0)
    {
        free_json_data(v);
        return NULL;
    }
    
    for (size_t i = 0; i < array->n; i++)
    {
        json_value item = array->data[i];
        if (item.type != kJSONTable)
            continue;
        
        const char *name = NULL;
        const char *script_path = NULL;
        
        JsonObject* obj = item.data.tableval;
        for (size_t j = 0; j < obj->n; j++)
        {
            const char *key = obj->data[j].key;
            json_value value = obj->data[j].value;
            
            if (strcmp(key, "name") == 0 && value.type == kJSONString)
            {
                name = value.data.stringval;
            }
            else if (strcmp(key, "script") == 0 && value.type == kJSONString)
            {
            #ifdef TARGET_SIMULATOR
                char fullpath[1024];
                snprintf(fullpath, sizeof(fullpath), "Source/%s", value.data.stringval);
                script_path = strdup(fullpath);
            #else
                script_path = value.data.stringval;
            #endif
            }
        }
        
        if (name && script_path && strcmp(name, game_name) == 0)
        {
            ScriptInfo* info = malloc(sizeof(ScriptInfo));
            info->script_path = strdup(script_path);
            free_json_data(v);
            return info;
        }
    }
    
    free_json_data(v);
    return NULL;
}

// TODO: should take rom data instead
lua_State *script_begin(const char *game_name, struct PGB_GameScene *game_scene)
{
    DTCM_VERIFY();

    lua_State *L = NULL;

    DTCM_VERIFY();
    
    ScriptInfo* info = get_script_info(game_name);

    if (!info)
    {
        return NULL;
    }

    L = luaL_newstate();
    open_sandboxed_libs(L);
    set_package_path_l(L);

    lua_pushlightuserdata(L, (void *)game_scene);
    lua_setfield(L, LUA_REGISTRYINDEX, REGISTRY_GAME_SCENE_KEY);

    register_pgb_library(L);

    DTCM_VERIFY();

    if (luaL_dofile(L, info->script_path) != LUA_OK)
    {
        const char *err = lua_tostring(L, -1);
        fprintf(stderr, "Lua error: %s\n", err);
        lua_close(L);

        DTCM_VERIFY();
        script_info_free(info);
        return NULL;
    }

    script_info_free(info);
    DTCM_VERIFY();
    return L;
}

void script_end(lua_State *L)
{
    if (L)
    {
        lua_close(L);
    }
}

void script_tick(lua_State *L)
{
    if (!L)
        return;

    lua_getglobal(L, "pgb");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, "update");
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2);  // pop update and pgb
        return;
    }

    lua_remove(L, -2);  // remove pgb, leave update
    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        const char *err = lua_tostring(L, -1);
        fprintf(stderr, "script_tick error: %s\n", err);
        lua_pop(L, 1);
    }
}

__section__(".rare")
void script_on_breakpoint(lua_State* L, int index)
{
    if (!L) return;
    
    // get lua top, store so it can be reset to later
    int top = lua_gettop(L);
    
    // Execute function from registry, breakpoint number index
    lua_getfield(L, LUA_REGISTRYINDEX, "pgb_breakpoints");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return;
    }
    lua_pushinteger(L, index);
    lua_gettable(L, -2);  // get function at index
    if (!lua_isfunction(L, -1))
    {
        printf("Unknown breakpoint %d\n", index);
        lua_pop(L, 2);  // pop function and table
        return;
    }
    lua_remove(L, -2);  // remove table, leave function
    // call function, pass index as argument
    lua_pushinteger(L, index);
    if (lua_pcall(L, 1, 0, 0) != LUA_OK)
    {
        const char *err = lua_tostring(L, -1);
        fprintf(stderr, "script breakpoint error error: %s\n", err);
        lua_pop(L, 1);
    }

    lua_settop(L, top);
}

#endif