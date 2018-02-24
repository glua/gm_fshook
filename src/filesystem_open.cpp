#include <vector>
#include <mutex>
#include "vfuncs.h"
#include "openresult.h"
#include <string>
#include "fs_funcs.h"

std::mutex OpenResult::m;
std::vector<OpenResult *> OpenResult::waiting_list;
void *Open_original;


FileHandle_t VirtualFunctionHooks::IBaseFileSystem__Open(const char *pFileName, const char *pOptions, const char *pathID) {
	IBaseFileSystem *ths = (IBaseFileSystem *)this;
	char full_path_c[4096];
	full_path_c[4095] = 0;
	
	if (!g_pFullFileSystem->RelativePathToFullPath(pFileName, pathID, full_path_c, sizeof full_path_c - 1))
		return 0;

	std::string relative_path, full_path = full_path_c;

	if (!RelativeFrom(full_path, "BASE_PATH", relative_path))
		return 0;

	OpenResult opener(relative_path, full_path);
	if (!opener.GetResult()) {
		return 0;
	}

	return FunctionHooks->BaseFileSystemReplacer->Call<FileHandle_t, const char *, const char *, const char *>(FunctionHooks->IBaseFileSystem__Open__index, pFileName, pOptions, pathID);
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