#ifndef VFUNCS_H
#define VFUNCS_H
#include "filesystem.h"
#include "vhook.h"
#include "GarrysMod/Lua/Interface.h"

class VirtualFunctionHooks {
public:
	uint32 mainthread;
	GarrysMod::Lua::ILuaBase *lua;
	VirtualReplacer<IBaseFileSystem> *FileSystemReplacer;

	FileHandle_t IBaseFileSystem__Open(const char *pFileName, const char *pOptions, const char *pathID);

};

extern VirtualFunctionHooks *FunctionHooks;
int ThinkHook(lua_State *state);

#endif // VFUNCS_H