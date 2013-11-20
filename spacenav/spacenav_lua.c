#include <lua.h>
#include <lauxlib.h>
#include "spacenav.h"

static spacenav_t *gethandle(lua_State *L)
{
	return *((spacenav_t**)luaL_checkudata(L,1,"spacenav"));
}

static int delete(lua_State *L)
{
	spacenav_t *sn = gethandle(L);
	spacenav_destroy(sn);
	lua_pushnil(L);
	lua_setmetatable(L,1);
	return 0;
}

static int get_vals(lua_State *L)
{
	spacenav_t *sn = gethandle(L);
	sn_axes_t v;
	int res;
	
	res = spacenav_get(sn, &v);
	
	if (res)
		return 0;
	
	lua_createtable(L, 0, 8);
	
	lua_pushnumber(L, v.x);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, v.y);
	lua_setfield(L, -2, "y");
	lua_pushnumber(L, v.z);
	lua_setfield(L, -2, "z");
	
	lua_pushnumber(L, v.rx);
	lua_setfield(L, -2, "rx");
	lua_pushnumber(L, v.ry);
	lua_setfield(L, -2, "ry");
	lua_pushnumber(L, v.rz);
	lua_setfield(L, -2, "rz");
	
	lua_pushboolean(L, v.btn_l);
	lua_setfield(L, -2, "btn_l");
	
	lua_pushboolean(L, v.btn_r);
	lua_setfield(L, -2, "btn_r");
	
	return 1;
}

static int open(lua_State *L)
{
	spacenav_t *sn = NULL;
	const char *dev = "/dev/input/spacenavigator";
	
	if(lua_isstring(L, 2)) {
		dev = lua_tostring(L, 2);
	}
	
	sn = spacenav_create(dev);
	if(!sn) {
		luaL_error(L, "cannot connect to space navigator");
	}
	else {
		spacenav_t **p=lua_newuserdata(L,sizeof(spacenav_t*));
		*p=sn;
		lua_pushvalue(L, lua_upvalueindex(1));
		lua_setmetatable(L, -2);
	}
	return 1;
}

static const luaL_Reg R[] =
{
	{ "__gc",			delete },
	{ "get_vals",		get_vals },
	{ NULL,				NULL }
};

LUALIB_API int luaopen_spacenav(lua_State *L)
{
	luaL_newmetatable(L,"spacenav");
	lua_pushnil(L);
	lua_setmetatable(L, -2);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");
	luaL_setfuncs(L, R, 0);
	lua_pushcclosure(L, open, 1);
	lua_setglobal(L, "spacenav");
	return 1;
}
