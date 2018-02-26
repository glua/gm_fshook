#ifndef VFUNCS_H
#define VFUNCS_H
#include "filesystem.h"
#include "vhook.h"
#include "GarrysMod/Lua/Interface.h"

class VirtualFunctionHooks {
public:
	uint32 mainthread;
	GarrysMod::Lua::ILuaBase *lua;
	VirtualReplacer<IBaseFileSystem> *BaseFileSystemReplacer;
	int IBaseFileSystem__Open__index;

	VirtualReplacer<IFileSystem> *FileSystemReplacer;
	int IFileSystem__FindFirstEx__index;
	int IFileSystem__FindNext__index;
	int IFileSystem__FindClose__index;
	int IFileSystem__FindIsDirectory__index;

	FileHandle_t IBaseFileSystem__Open(const char *pFileName, const char *pOptions, const char *pathID);

	const char *IFileSystem__FindFirstEx(const char *pWildCard, const char *pPathID, FileFindHandle_t *pHandle);
	const char *IFileSystem__FindNext(FileFindHandle_t pHandle);
	void IFileSystem__FindClose(FileFindHandle_t Handle);
};

extern VirtualFunctionHooks *FunctionHooks;

#endif // VFUNCS_H