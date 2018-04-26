#include <vector>
#include <mutex>
#include "vfuncs.h"
#include "openresult.h"
#include <string>
#include "fs_funcs.h"


FileHandle_t VirtualFunctionHooks::IBaseFileSystem__Open(const char *pFileName, const char *pOptions, const char *pathID) {
	IBaseFileSystem *ths = (IBaseFileSystem *)this;
	std::string relative_path, full_path;
	
	if (!pathID || strcmp("SKIN", pathID))
		if (FunctionHooks->mainthread == ThreadGetCurrentId()) {

			try {
				ToFull(pFileName, pathID, full_path);
				RelativeFrom(full_path, "BASE_PATH", relative_path);
			}
			catch (std::exception e) {
				return 0;
			}

			OpenResult opener(relative_path, full_path);
			if (!opener.GetResult())
				return 0;
		}
		else {
			FSLog(std::string("Path ID ") + (pathID ? pathID : "NULL") + " accessed from non-main thread for file " + pFileName + " (" + (pOptions ? pOptions : "no options") + ")");
		}

	return FunctionHooks->BaseFileSystemReplacer->Call<FileHandle_t, const char *, const char *, const char *>(FunctionHooks->IBaseFileSystem__Open__index, pFileName, pOptions, pathID);
}