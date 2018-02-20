#include <vector>
#include <mutex>
#include "vfuncs.h"
#include "openresult.h"
#include <string>
#include <algorithm>

std::mutex OpenResult::m;
std::vector<OpenResult *> OpenResult::waiting_list;
void *Open_original;

bool Canonicalize(std::string &path);

#ifdef _WIN32 
#include <Windows.h>
#include <Shlwapi.h>
bool Canonicalize(std::string &path) {
	std::replace(path.begin(), path.end(), '/', '\\');
	char out[MAX_PATH + 1];
	BOOL ret = PathCanonicalizeA(out, path.c_str());
	out[MAX_PATH] = 0;
	path = out;
	std::replace(path.begin(), path.end(), '\\', '/');
	return ret != FALSE;
}
#else
bool Canonicalize(std::string &path) {
	return true;
}
#endif


FileHandle_t VirtualFunctionHooks::IBaseFileSystem__Open(const char *pFileName, const char *pOptions, const char *pathID) {
	IBaseFileSystem *ths = (IBaseFileSystem *)this;
	char full_path[4096];
	full_path[4095] = 0;
	
	if (!g_pFullFileSystem->RelativePathToFullPath(pFileName, pathID, full_path, sizeof(full_path) - 1))
		return 0;

	std::string full_path_str = full_path;

	if (!Canonicalize(full_path_str))
		return 0; // fuck off

	char relative_path[4096];
	relative_path[4095] = 0;
	if (!g_pFullFileSystem->FullPathToRelativePathEx(full_path_str.c_str(), "BASE_PATH", relative_path, sizeof relative_path - 1))
		return 0;
	
	OpenResult opener(relative_path);
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