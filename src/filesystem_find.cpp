#include "openresult.h"
#include "vfuncs.h"
#include "vhook.h"
#include <map>
#include <tuple>
#include "fs_funcs.h"

static bool IsOK(std::string full, std::string pathid) {
	std::string relative_str;
	try {
		RelativeFrom(full, "BASE_PATH", relative_str);
	}
	catch (std::exception e) {
		OpenResult opener("", full, pathid);
		return opener.GetResult();
		return true;
	}
	OpenResult opener(relative_str, full);
	return opener.GetResult();
}

static const char *NextFile(FileFindHandle_t pHandle) {
	return FunctionHooks->FileSystemReplacer->Call<const char *, FileFindHandle_t>(FunctionHooks->IFileSystem__FindNext__index, pHandle);
}

static auto handle_to_stuff = std::map<FileFindHandle_t, std::tuple<const char *, const char *>>();
static std::mutex handle_to_stuff_m;

static const char *NextOK(const char *name, FileFindHandle_t Handle) {
	char pathname[4096];
	std::string full_path;
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

		try {
			ToFull(pathname, pPathID, full_path);
		} 
		catch (std::exception e) {
			return 0;
		}

		if (IsOK(full_path, pPathID ? pPathID : "NULL"))
			break;
		name = NextFile(Handle);
	}
	return name;
}

const char *VirtualFunctionHooks::IFileSystem__FindFirstEx(const char *pWildCard, const char *pPathID, FileFindHandle_t *pHandle) {
	const char *ret = FunctionHooks->FileSystemReplacer->Call<const char *, const char *, const char *, FileFindHandle_t *>(FunctionHooks->IFileSystem__FindFirstEx__index, pWildCard, pPathID, pHandle);
	
	handle_to_stuff_m.lock();
	handle_to_stuff[*pHandle] = std::tuple<const char *, const char *>(pPathID, pWildCard);
	handle_to_stuff_m.unlock();

	return NextOK(ret, *pHandle);
}

const char *VirtualFunctionHooks::IFileSystem__FindNext(FileFindHandle_t Handle) {
	return NextOK(NextFile(Handle), Handle);
}

void VirtualFunctionHooks::IFileSystem__FindClose(FileFindHandle_t Handle) {
	FunctionHooks->FileSystemReplacer->Call<void, FileFindHandle_t>(FunctionHooks->IFileSystem__FindClose__index, Handle);
	handle_to_stuff_m.lock();
	handle_to_stuff.erase(Handle);
	handle_to_stuff_m.unlock();
}