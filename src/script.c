#define lua_State pd_lua_State
#define lua_CFunction pd_lua_CFunction
#include "pd_api.h"
#include "minigb_apu.h"
#undef lua_State
#undef lua_CFunction

#include <lua.h>

#include "app.h"
#include "script.h"
#include "dtcm.h"

#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define REGISTRY_GAME_SCENE_KEY "PGB_GameScene"

void PGB_GameScene_didSelectLibrary(struct PGB_GameScene* scene);

static bool lua_check_args(lua_State* L, int min, int max) {
    int argc = lua_gettop(L);
    return argc >= min && argc <= max;
}

static struct PGB_GameScene* get_game_scene(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, REGISTRY_GAME_SCENE_KEY);
    struct PGB_GameScene* scene = (struct PGB_GameScene*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return scene;
}

static int pgb_close(lua_State* L) {
    if (!lua_check_args(L, 0, 0)) {
        return luaL_error(L, "pgb.close() takes no arguments");
    }
    struct PGB_GameScene* scene = get_game_scene(L);
    if (scene) {
        PGB_GameScene_didSelectLibrary(scene);
    }
    return 0;
}

static void register_pgb_library(lua_State* L) {
    lua_newtable(L);
    {
        lua_pushcfunction(L, pgb_close);
        lua_setfield(L, -2, "close");
    }
    lua_setglobal(L, "pgb");
}

static void open_sandboxed_libs(lua_State* L) {
    luaL_requiref(L, "_G", luaopen_base, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1); lua_pop(L, 1);
}

static void set_package_path_l(lua_State* L) {
    lua_getglobal(L, "package");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, "path");
    const char* path = lua_tostring(L, -1);
    lua_pop(L, 1);

    // Replace all ".lua" with ".l"
    const char* from = ".lua";
    const char* to = ".l";
    char* new_path = malloc(strlen(path) * 2); // generous space
    char* out = new_path;
    const char* p = path;
    while (*p) {
        if (strncmp(p, from, 4) == 0) {
            strcpy(out, to);
            out += 2;
            p += 4;
        } else {
            *out++ = *p++;
        }
    }
    *out = '\0';

    lua_pushstring(L, new_path);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
    free(new_path);
}

typedef struct {
    const char* match_name;
    const char* matched_script;
} MatchContext;

static void decodeError(struct json_decoder* decoder, const char* error, int linenum) {
    // Log or ignore
}

static int shouldDecodeArrayValueAtIndex(struct json_decoder* decoder, int pos) {
    return 1;
}

static void willDecodeSublist(struct json_decoder* decoder, const char* name, json_value_type type) {
    // No-op for now
}

static int shouldDecodeTableValueForKey(struct json_decoder* decoder, const char* key) {
    return 1;
}

static void didDecodeTableValue(struct json_decoder* decoder, const char* key, json_value value) {
    MatchContext* ctx = decoder->userdata;

    if (strcmp(key, "name") == 0 && value.type == kJSONString) {
        if (strcmp(ctx->match_name, value.data.stringval) == 0) {
            // mark match: nothing to do yet
        }
    } else if (strcmp(key, "script") == 0 && value.type == kJSONString) {
        if (ctx->matched_script == NULL) {
            #ifdef TARGET_PLAYDATE
            ctx->matched_script = strdup(value.data.stringval);
            #else
            // prepend Source/
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "Source/%s", value.data.stringval);
            ctx->matched_script = strdup(fullpath);
            #endif
        }
    }
}

static void didDecodeArrayValue(struct json_decoder* decoder, int pos, json_value value) {
}

static void* didDecodeSublist(struct json_decoder* decoder, const char* name, json_value_type type) {
    return NULL;
}

lua_State* script_begin(const char* game_name, struct PGB_GameScene* game_scene) {
    DTCM_VERIFY();
    
    lua_State* L = NULL;
    
    DTCM_VERIFY();
    
    SDFile* file = playdate->file->open("scripts.json", kFileRead);
    if (!file)
    {
        DTCM_VERIFY();
        return NULL;
    };

    MatchContext ctx = {
        .match_name = game_name,
        .matched_script = NULL
    };

    struct json_decoder decoder = {
        .decodeError = decodeError,
        .willDecodeSublist = willDecodeSublist,
        .shouldDecodeTableValueForKey = shouldDecodeTableValueForKey,
        .didDecodeTableValue = didDecodeTableValue,
        .shouldDecodeArrayValueAtIndex = shouldDecodeArrayValueAtIndex,
        .didDecodeArrayValue = didDecodeArrayValue,
        .didDecodeSublist = didDecodeSublist,
        .userdata = &ctx,
        .returnString = 0,
        .path = NULL
    };

    json_reader reader = {
        .read = (int (*)(void*, uint8_t*, int))playdate->file->read,
        .userdata = file
    };

    int ok = playdate->json->decode(&decoder, reader, NULL);
    playdate->file->close(file);

    if (!ok || ctx.matched_script == NULL) {
        return NULL;
    }

    L = luaL_newstate();
    open_sandboxed_libs(L);
    set_package_path_l(L);

    lua_pushlightuserdata(L, (void*)game_scene);
    lua_setfield(L, LUA_REGISTRYINDEX, REGISTRY_GAME_SCENE_KEY);

    register_pgb_library(L);
    
    DTCM_VERIFY();

    if (luaL_dofile(L, ctx.matched_script) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        fprintf(stderr, "Lua error: %s\n", err);
        lua_close(L);
        free((void*)ctx.matched_script);
        
        DTCM_VERIFY();
        return NULL;
    }

    free((void*)ctx.matched_script);
    
    DTCM_VERIFY();
    return L;
}

void script_end(lua_State* L) {
    if (L) {
        lua_close(L);
    }
}

void script_tick(lua_State* L) {
    if (!L) return;
    
    lua_getglobal(L, "pgb");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, "update");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2); // pop update and pgb
        return;
    }

    lua_remove(L, -2); // remove pgb, leave update
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        fprintf(stderr, "script_tick error: %s\n", err);
        lua_pop(L, 1);
    }
}
