#include <vector>
#include <mutex>
#include "vfuncs.h"
#include "openresult.h"
#include <string>
#include "fs_funcs.h"


FileHandle_t VirtualFunctionHooks::IBaseFileSystem__Open(const char *pFileName, const char *pOptions, const char *pathID) {
	IBaseFileSystem *ths = (IBaseFileSystem *)this;
	std::string relative_path, full_path;
	
	if (FunctionHooks->mainthread == ThreadGetCurrentId()) {
		if (!ToFull(pFileName, pathID, full_path))
			return 0;

		if (!RelativeFrom(full_path, "BASE_PATH", relative_path))
			return 0;

		OpenResult opener(relative_path, full_path);
		if (!opener.GetResult())
			return 0;
	}

	return FunctionHooks->BaseFileSystemReplacer->Call<FileHandle_t, const char *, const char *, const char *>(FunctionHooks->IBaseFileSystem__Open__index, pFileName, pOptions, pathID);
}