/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua sax functions @file */

#include <lua.h>
#include <lauxlib.h>
#include <math.h>
#include <symtseries.h>

static const char* mozsvc_sax_window = "mozsvc.sax.window";
static const char* mozsvc_sax_word = "mozsvc.sax.word";
static const char* mozsvc_sax_table = "sax";

static void check_nwc(lua_State* lua, int n, int w, int c, int offset, int allow_n_zero) 
{
  luaL_argcheck(lua, (allow_n_zero ? n >= 0 : n > 1) && n <= 4096, 
      offset, "n is out of range");
  luaL_argcheck(lua, w > 1 && w <= 2048, offset, "w is out of range");
  luaL_argcheck(lua, n % w == 0, offset, 
                "n must be evenly divisible by w");
  luaL_argcheck(lua, 1 < c && c <= STS_MAX_CARDINALITY, offset,
                "cardinality is out of range");
}

static sts_word check_sax_word(lua_State* lua, int ind) {
  /* word was previously successfully constructed -> No need to check for NULLs */
  sts_word *ud = luaL_checkudata(lua, ind, mozsvc_sax_word);
  return *ud;
}

static const struct sts_word *check_word_or_window(lua_State* lua, int ind)
{
  void *ud = lua_touserdata(lua, ind);
  if (ud != NULL) {  
    if (lua_getmetatable(lua, ind)) { 
      lua_getfield(lua, LUA_REGISTRYINDEX, mozsvc_sax_word);
      if (lua_rawequal(lua, -1, -2)) {
        lua_pop(lua, 2);  /* remove both metatables */
        return *((sts_word *) ud);
      } else {
        lua_pop(lua, 1);  /* remove word metatable */
        lua_getfield(lua, LUA_REGISTRYINDEX, mozsvc_sax_window);
        if (lua_rawequal(lua, -1, -2)) {
          sts_window window = *((struct sts_window **) ud);
          if (window->values->cnt < window->current_word.n_values) {
            luaL_argerror(lua, ind, 
                "sax.window detected but it has not enough values to construct a word");
          }
          lua_pop(lua, 2);  /* remove both metatables */
          return &window->current_word;
        }
      }
    }
  }
  luaL_typerror(lua, ind, "sax.window or sax.word");
  return NULL;
}

static sts_window check_sax_window(lua_State* lua, int ind)
{
  sts_window* ud = luaL_checkudata(lua, ind, mozsvc_sax_window);
  luaL_argcheck(lua, ud != NULL, ind, "invalid userdata type");
  luaL_argcheck(lua, *ud != NULL, ind, "invalid sax_window address");
  sts_window win = *ud;
  return win;
}

static int sax_new_window(lua_State* lua)
{
  luaL_argcheck(lua, lua_gettop(lua) == 3, 0, "incorrect number of args");
  int n = luaL_checkint(lua, 1);
  int w = luaL_checkint(lua, 2);
  int c = luaL_checkint(lua, 3);
  check_nwc(lua, n, w, c, 1, 0);

  sts_window win = sts_new_window(n, w, c);
  if (!win) luaL_error(lua, "memory allocation failed");

  sts_window* ud = lua_newuserdata(lua, sizeof *ud);
  if (!ud) luaL_error(lua, "memory allocation failed");
  *ud = win;

  luaL_getmetatable(lua, mozsvc_sax_window);
  lua_setmetatable(lua, -2);
  return 1;
}

static void push_word(lua_State* lua, const struct sts_word* a)
{
  const struct sts_word **ud = lua_newuserdata(lua, sizeof *ud);
  if (!ud) luaL_error(lua, "memory allocation failed");
  *ud = a;

  luaL_getmetatable(lua, mozsvc_sax_word);
  lua_setmetatable(lua, -2);
}

static int sax_add(lua_State* lua)
{
  luaL_argcheck(lua, lua_gettop(lua) == 2, 0, "incorrect number of args");
  sts_window win = check_sax_window(lua, 1);
  double d = luaL_checknumber(lua, 2);
  const struct sts_word* a = sts_append_value(win, d);
  if (!a) {
    lua_pushnil(lua);
  } else {
    push_word(lua, a);
  }
  return 1;
}

static int sax_mindist(lua_State* lua)
{
  luaL_argcheck(lua, lua_gettop(lua) == 2, 0, "incorrect number of args");
  const struct sts_word *a = check_word_or_window(lua, 1);
  const struct sts_word *b = check_word_or_window(lua, 2);

  double d = sts_mindist(a, b);
  if (isnan(d)) {
    lua_pushnil(lua);
  } else {
    lua_pushnumber(lua, d);
  }
  return 1;
}

static int sax_word_to_string(lua_State* lua)
{
  luaL_argcheck(lua, lua_gettop(lua) == 1, 0, "incorrect number of args");
  sts_word a = check_sax_word(lua, 1);
  size_t w = a->w;
  size_t c = a->c;
  char *str = malloc(w + 1 * sizeof *str);
  str[w] = '\0';
  for (size_t i = 0; i < w; ++i) {
    unsigned char dig = a->symbols[i];
    luaL_argcheck(lua, dig <= c, 1, "symbol out of range encountered");
    str[i] = c - a->symbols[i] - 1 + 'A';
  }
  lua_pushstring(lua, str);
  free(str);
  return 1;
}

static int sax_equal(lua_State* lua)
{
  luaL_argcheck(lua, lua_gettop(lua) == 2, 0, "incorrect number of args");
  const struct sts_word *a = check_word_or_window(lua, 1);
  const struct sts_word *b = check_word_or_window(lua, 2);
  if (a->w != b->w || a->c != b->c) {
    lua_pushboolean(lua, 0);
    return 1;
  }
  size_t w = a->w;
  for (size_t i = 0; i < w; ++i) {
    if (a->symbols[i] != b->symbols[i]) {
      lua_pushboolean(lua, 0);
      return 1;
    }
  }
  lua_pushboolean(lua, 1);
  return 1;
}

static int sax_word_copy(lua_State* lua)
{
  luaL_argcheck(lua, lua_gettop(lua) == 1, 0, "incorrect number of args");
  sts_word a = check_sax_word(lua, 1);
  sts_word new_a = sts_dup_word(a);
  push_word(lua, new_a);
  return 1;
}

static int sax_from_double_array(lua_State* lua) 
{
  int w = luaL_checkint(lua, 2);
  int c = luaL_checkint(lua, 3);
  if (!lua_istable(lua, 1)) 
    luaL_argerror(lua, 1, "array-like table expected");

  int size = lua_objlen(lua, 1);
  check_nwc(lua, size, w, c, 2, 0);

  double *buf = malloc(size * sizeof *buf);
  if (!buf) luaL_error(lua, "memory allocation failed");

  for (int i = 1; i <= size; ++i) {
    lua_pushnumber(lua, i);
    lua_gettable(lua, 1);
    buf[i-1] = luaL_checknumber(lua, -1);
    lua_pop(lua, 1);
  }

  sts_word a = sts_from_double_array(buf, size, w, c);
  if (!a) luaL_error(lua, "memory allocation failed");
  free(buf);

  push_word(lua, a);
  return 1;
}

static int sax_from_string(lua_State* lua) 
{
  const char *s = luaL_checkstring(lua, 1);
  int c = luaL_checkint(lua, 2);
  sts_word a = sts_from_sax_string(s, c);
  if (!a) luaL_argerror(lua, 1, 
        "illegal symbols for given cardinality or bad cardinality itself");
  push_word(lua, a);
  return 1;
}

static int sax_new_word(lua_State* lua) 
{
  int argc = lua_gettop(lua);
  switch (argc) {
    case 2:
      return sax_from_string(lua);
    case 3:
      return sax_from_double_array(lua);
    default:
      return luaL_argerror(lua, 0, "incorrect number of arguments");
  }
}

static int sax_clear(lua_State* lua)
{
  luaL_argcheck(lua, lua_gettop(lua) == 1, 0, "incorrect number of arguments");
  sts_window win = check_sax_window(lua, 1);
  sts_reset_window(win);
  return 0;
}

static int sax_gc_window(lua_State* lua)
{
  luaL_argcheck(lua, lua_gettop(lua) == 1, 0, "incorrect number of arguments");
  sts_window win = check_sax_window(lua, 1);
  sts_free_window(win);
  return 0;
}

static int sax_gc_word(lua_State* lua)
{
  luaL_argcheck(lua, lua_gettop(lua) == 1, 0, "incorrect number of arguments");
  sts_word a = check_sax_word(lua, 1);
  sts_free_word(a);
  return 0;
}

static const struct luaL_Reg saxlib_f[] =
{
  { "new_window", sax_new_window }
  , { "new_word", sax_new_word }
  , { "mindist", sax_mindist }
  , { NULL, NULL }
};

static const struct luaL_Reg saxlib_word[] =
{
  { "__gc", sax_gc_word }
  , { "__tostring", sax_word_to_string }
  , { "copy", sax_word_copy }
  , { NULL, NULL }
};

static const struct luaL_Reg saxlib_win[] =
{
  { "add", sax_add }
  , { "clear", sax_clear }
  , { "__gc", sax_gc_window }
  , { NULL, NULL }
};

void reg_module(lua_State* lua, const char *name, const struct luaL_Reg *module) {
  luaL_newmetatable(lua, name);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, module);
  lua_pushstring(lua, "__eq");
  lua_pushvalue(lua, -3); // Copy sax_equal on top
  lua_settable(lua, -3);
  lua_pop(lua, 1); // Pop table
}

int luaopen_sax(lua_State* lua)
{
  /* We're registering sax_equal separately since it's the only way to make lua
   * aware that it's exactly the same function each time 
   * (otherwise it doesn't get called on different object types) */
  lua_pushcfunction(lua, sax_equal);

  reg_module(lua, mozsvc_sax_window, saxlib_win);
  reg_module(lua, mozsvc_sax_word, saxlib_word);

  luaL_register(lua, mozsvc_sax_table, saxlib_f);
  return 1;
}
