#include <vector>
#include <mutex>
#include "vfuncs.h"
#include "GarrysMod/Lua/Interface.h"

class OpenResult {
public:
	OpenResult(const char *relativetobase) {
		file = relativetobase;
	}

	bool GetResult() {
		if (ThreadGetCurrentId() == FunctionHooks->mainthread) {
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
		auto lua = FunctionHooks->lua;
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
	const char *file;
	bool has_result;
	bool result;
};

std::mutex OpenResult::m;
std::vector<OpenResult *> OpenResult::waiting_list;
void *Open_original;

FileHandle_t VirtualFunctionHooks::IBaseFileSystem__Open(const char *pFileName, const char *pOptions, const char *pathID) {
	IBaseFileSystem *ths = (IBaseFileSystem *)this;
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

	return FunctionHooks->FileSystemReplacer->Call<FileHandle_t, const char *, const char *, const char *>(2, pFileName, pOptions, pathID);
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