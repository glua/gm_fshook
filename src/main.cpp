#include "filesystem.h"
#include "GarrysMod/Lua/Interface.h"
#include "interface.h"
#include "threadtools.h"
#include <vector>
#include <mutex>

#ifdef _WIN32
#include <Windows.h>
#else
#error "pls put this here"
#endif

IBaseFileSystem *g_pBaseFileSystem;

class CVTableHooker {
public:
	CVTableHooker(void *_interface) {
		this->_interface = _interface;
		this->oldvtable = *(void ***)_interface;
		void **vtable = this->oldvtable;
		this->vtable_size = 0;
		while (*vtable++)
			this->vtable_size++;

		void **newvtable = new void *[this->vtable_size];

		vtable = this->oldvtable;
		for (size_t i = 0; i < this->vtable_size; i++)
			newvtable[i] = vtable[i];

		*(void ***)_interface = newvtable;
	}
	~CVTableHooker() {
		delete *(void ***)this->_interface;
		*(void ***)this->_interface = this->oldvtable;
	}

	template<typename T>
	T *GetIndex(int which) {
		return (T *)this->oldvtable[which];
	}

	void SetIndex(int which, void *newval) {
		(*(void ***)this->_interface)[which] = newval;
	}

private:
	size_t vtable_size;
	void **oldvtable;
	void *_interface;
};

class OpenResult {
public:
	OpenResult(const char *relativetobase) {
		file = relativetobase;
	}

	bool GetResult() {
		if (ThreadGetCurrentId() == OpenResult::mainthread) {
			return RunLua();
		}
		OpenResult::m.lock();
		this->has_result = false;
		OpenResult::waiting_list.push_back(this);
		OpenResult::m.unlock();

		while (!this->has_result)
			ThreadSleep(2);

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
	static decltype(ThreadGetCurrentId()) mainthread;
	const char *file;
	bool has_result;
	bool result;
};

std::mutex OpenResult::m;
std::vector<OpenResult *> OpenResult::waiting_list;
GarrysMod::Lua::ILuaBase *OpenResult::lua;
decltype(ThreadGetCurrentId()) OpenResult::mainthread;

CVTableHooker *filesystem_hooker;


FileHandle_t Open_hook_internal(IBaseFileSystem *fs, const char *pFileName, const char *pOptions, const char *pathID);
#ifdef _WIN32
FileHandle_t __fastcall Open_hook(IBaseFileSystem *fs, void *, const char *pFileName, const char *pOptions, const char *pathID = 0) {
	return Open_hook_internal(fs, pFileName, pOptions, pathID);
}
#else
#error "pls put this here"
#endif

FileHandle_t Open_hook_internal(IBaseFileSystem *fs, const char *pFileName, const char *pOptions, const char *pathID) {
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

#ifdef _WIN32 
	return filesystem_hooker->GetIndex<FileHandle_t(__fastcall)(IBaseFileSystem *fs, void *, const char *pFileName, const char *pOptions, const char *pathID)>(2)
		(fs, 0, pFileName, pOptions, pathID);
	
#else
#error "pls put this here"
#endif
}

int ThinkHook(lua_State *state) {
	auto &v = OpenResult::waiting_list;
	OpenResult::m.lock();
	for (int i = v.size() - 1; i >= 0; i--) {
		OpenResult *res = v[i];
		res->result = res->RunLua();
		v.erase(v.begin() + i);
		res->has_result = true;
	}
	OpenResult::m.unlock();
	return 0;
}

GMOD_MODULE_OPEN() {
	OpenResult::mainthread = ThreadGetCurrentId();
	OpenResult::lua = LUA;
#ifdef _WIN32
	int retcode;
	g_pFullFileSystem = (decltype(g_pFullFileSystem))CreateInterfaceFn(GetProcAddress(GetModuleHandleA("filesystem_stdio"), "CreateInterface"))(FILESYSTEM_INTERFACE_VERSION, &retcode);
#else
#error "plz put here this"
#endif
	g_pBaseFileSystem = g_pFullFileSystem;
	filesystem_hooker = new CVTableHooker(g_pBaseFileSystem);
	filesystem_hooker->SetIndex(2, &Open_hook);

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
	return 0;
}