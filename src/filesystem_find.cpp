#include "openresult.h"
#include "vfuncs.h"
#include "vhook.h"
#include <map>
#include <tuple>

static bool IsOK(const char *name) {
	OpenResult opener(name);
	return opener.GetResult();
}

static const char *NextFile(FileFindHandle_t pHandle) {
	return FunctionHooks->FileSystemReplacer->Call<const char *, FileFindHandle_t>(FunctionHooks->IFileSystem__FindNext__index, pHandle);
}

auto handle_to_stuff = std::map<FileFindHandle_t, std::tuple<const char *, const char *>>();

static const char *NextOK(const char *name, FileFindHandle_t Handle) {
	char pathname[4096];
	char temp[4096];
	temp[4095] = 0;
	char relative_path[4096];
	relative_path[4095] = 0;
	const char *pPathID, *pWildCard;
	std::tie(pPathID, pWildCard) = handle_to_stuff[Handle];

	if (!pPathID)
		return name;

	while (name) {

		snprintf(pathname, sizeof(pathname), "%s", pWildCard);
		char *forward = strrchr(pathname, '/');
		char *backward = strrchr(pathname, '\\');
		char *enddir = forward > backward ? forward : backward;
		enddir = enddir ? enddir + 1 : pathname;
		int offset = enddir - pathname;
		snprintf(enddir, sizeof(pathname) - offset, "%s", name);

		g_pFullFileSystem->RelativePathToFullPath(pathname, pPathID, temp, sizeof temp - 1);
		if (g_pFullFileSystem->FullPathToRelativePathEx(temp, "BASE_PATH", relative_path, sizeof relative_path - 1) && IsOK(relative_path))
			break;
		name = NextFile(Handle);
	}
	return name;
}

const char *VirtualFunctionHooks::IFileSystem__FindFirstEx(const char *pWildCard, const char *pPathID, FileFindHandle_t *pHandle) {
	const char *ret = FunctionHooks->FileSystemReplacer->Call<const char *, const char *, const char *, FileFindHandle_t *>(FunctionHooks->IFileSystem__FindFirstEx__index, pWildCard, pPathID, pHandle);
	
	handle_to_stuff[*pHandle] = std::tuple<const char *, const char *>(pPathID, pWildCard);

	return NextOK(ret, *pHandle);
}

const char *VirtualFunctionHooks::IFileSystem__FindNext(FileFindHandle_t Handle) {
	return NextOK(NextFile(Handle), Handle);
}

void VirtualFunctionHooks::IFileSystem__FindClose(FileFindHandle_t Handle) {
	FunctionHooks->FileSystemReplacer->Call<void, FileFindHandle_t>(FunctionHooks->IFileSystem__FindClose__index, Handle);
	handle_to_stuff.erase(Handle);
}

bool VirtualFunctionHooks::IFileSystem__FindIsDirectory(FileFindHandle_t Handle) {
	return FunctionHooks->FileSystemReplacer->Call<bool, FileFindHandle_t>(FunctionHooks->IFileSystem__FindIsDirectory__index, Handle);
}