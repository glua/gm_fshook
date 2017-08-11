#include <vector>
#include <mutex>
#include "vfuncs.h"
#include "openresult.h"

std::mutex OpenResult::m;
std::vector<OpenResult *> OpenResult::waiting_list;
void *Open_original;

FileHandle_t VirtualFunctionHooks::IBaseFileSystem__Open(const char *pFileName, const char *pOptions, const char *pathID) {
	IBaseFileSystem *ths = (IBaseFileSystem *)this;
	char temp[4096];
	temp[4095] = 0;
	g_pFullFileSystem->RelativePathToFullPath(pFileName, pathID, temp, sizeof(temp) - 1);
	char relative_path[4096];
	relative_path[4095] = 0;
	IOpenResult res;

	if (g_pFullFileSystem->FullPathToRelativePathEx(temp, "BASE_PATH", relative_path, sizeof relative_path - 1)) {
		OpenResult opener(relative_path);
		res = opener.GetResult();
		if (!res.shouldOpen) {
			return 0;
		}
	}

	if (res.shouldRedirect) {
		return FunctionHooks->BaseFileSystemReplacer->Call<FileHandle_t, const char *, const char *, const char *>(FunctionHooks->IBaseFileSystem__Open__index, res.redirect.c_str(), pOptions, pathID);
	}
	else {
		return FunctionHooks->BaseFileSystemReplacer->Call<FileHandle_t, const char *, const char *, const char *>(FunctionHooks->IBaseFileSystem__Open__index, pFileName, pOptions, pathID);
	}
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