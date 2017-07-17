#include "filesystem.h"
#include "GarrysMod/Lua/Interface.h"
#include "interface.h"
#include "threadtools.h"
#include "vfnhook.h"
#include <vector>
#include <mutex>

IFileSystem *g_pFullFileSystem = NULL;
static CDllDemandLoader filesystem_stdio_factory( "filesystem_stdio" );

class OpenResult {
public:
	OpenResult(const char *relativetobase) {
		this->file = relativetobase;
	}

	bool GetResult() {
		if (ThreadGetCurrentId() == OpenResult::mainthread) {
			return this->RunLua();
		}
		this->has_result = false;
		OpenResult::m.lock();
		OpenResult::waiting_list.push_back(this);
		OpenResult::m.unlock();
		while (!this->has_result)
		{
			ThreadSleep(2);
		}
		return this->result;
	}

	bool RunLua() {
		auto lua = OpenResult::lua;
		lua->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		lua->GetField(-1, "hook");
		lua->GetField(-1, "Run");
		lua->PushString("ShouldOpenFile");
		lua->PushString(file);
		lua->Call(2, 1);
		bool ret = !lua->GetBool(-1);
		lua->Pop(3);
		return ret;
	}

public:
	static std::mutex m;
	static std::vector<OpenResult *> waiting_list;
	static GarrysMod::Lua::ILuaBase *lua;
	static uint32 mainthread;
	const char *file;
	bool result;
	bool has_result;
};

std::mutex OpenResult::m;
std::vector<OpenResult *> OpenResult::waiting_list;
GarrysMod::Lua::ILuaBase *OpenResult::lua = NULL;
uint32 OpenResult::mainthread = 0;

DEFVFUNC_(Open, FileHandle_t, (IBaseFileSystem *fs, const char *pFileName, const char *pOptions, const char *pathID));

FileHandle_t VFUNC Open_hook(IBaseFileSystem *fs, const char *pFileName, const char *pOptions, const char *pathID) {
	char temp[4096];
	temp[4095] = 0;
	g_pFullFileSystem->RelativePathToFullPath(pFileName, pathID, temp, sizeof temp - 1);
	char relative_path[4096];
	relative_path[4095] = 0;
	if (g_pFullFileSystem->FullPathToRelativePathEx(temp, "BASE_PATH", relative_path, sizeof relative_path - 1)) {
		OpenResult opener(relative_path);
		if (!opener.GetResult()) {
			pFileName = "steam_appid.txt";
			pathID = "BASE_PATH";
			pOptions = "rb";
		}
	}

	return Open(fs, pFileName, pOptions, pathID);
}

int ThinkHook(lua_State *state) {
	auto v = OpenResult::waiting_list;
	while (v.size()) {
		OpenResult::m.lock();
		OpenResult *res = v[0];
		v.erase(v.begin());
		OpenResult::m.unlock();
		if (res->has_result)
			continue;
		res->result = res->RunLua();
		res->has_result = true;
	}
	return 0;
}

GMOD_MODULE_OPEN() {
	OpenResult::mainthread = ThreadGetCurrentId();
	OpenResult::lua = LUA;

	CreateInterfaceFn factory = filesystem_stdio_factory.GetFactory();
	if(factory == nullptr) {
		LUA->ThrowError("failed to get filesystem_stdio interface factory");
	}

	g_pFullFileSystem = (IFileSystem *)factory(FILESYSTEM_INTERFACE_VERSION, NULL);
	if(g_pFullFileSystem == nullptr) {
		LUA->ThrowError("failed to get IFileSystem interface");
	}

	HOOKVFUNC((IBaseFileSystem *)g_pFullFileSystem, 2, Open, Open_hook);

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "hook");
	LUA->GetField(-1, "Add");
	LUA->PushString("Think");
	LUA->PushString("gm_fs");
	LUA->PushCFunction(ThinkHook);
	LUA->Call(3, 0);
	LUA->Pop(1);

	return 0;
}

GMOD_MODULE_CLOSE() {
	if(g_pFullFileSystem != NULL) {
		UNHOOKVFUNC((IBaseFileSystem *)g_pFullFileSystem, 2, Open);
	}

	return 0;
}
