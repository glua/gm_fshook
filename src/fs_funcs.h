#include <string>
#include <algorithm>

static bool Canonicalize(std::string &path);
static bool ToRelative(std::string from, std::string to, std::string &out);
static bool ToFull(std::string relative, const char *pathID, std::string &out);

#ifdef _WIN32 
#include <Windows.h>
#include <Shlwapi.h>
bool Canonicalize(std::string &path) {
	std::replace(path.begin(), path.end(), '/', '\\');
	char out[MAX_PATH + 1];
	BOOL ret = PathCanonicalizeA(out, path.c_str());
	out[MAX_PATH] = 0;
	if (ret != FALSE) {
		path = out;
		std::replace(path.begin(), path.end(), '\\', '/');
	}
	return ret != FALSE;
}

bool ToRelative(std::string from, std::string to, std::string &out) {
	std::replace(from.begin(), from.end(), '/', '\\');
	std::replace(to.begin(), to.end(), '/', '\\');
	char outc[MAX_PATH + 1];
	BOOL ret = PathRelativePathToA(outc, from.c_str(), FILE_ATTRIBUTE_DIRECTORY, to.c_str(), 0);
	outc[MAX_PATH] = 0;
	if (ret != FALSE) {
		out = outc;
		std::replace(out.begin(), out.end(), '\\', '/');
	}
	out = out.substr(2);
	return ret != FALSE;
}

bool ToFull(std::string relative, const char *pathID, std::string &out) {
	char full_path_c[4096];
	full_path_c[4095];
	if (!g_pFullFileSystem->RelativePathToFullPath("./", pathID, full_path_c, sizeof full_path_c - 1))
		return 0;
	char outc[MAX_PATH];
	LPSTR ret = PathCombineA(outc, full_path_c, relative.c_str());
	outc[MAX_PATH - 1] = 0;
	if (ret != NULL) {
		out = outc;
		return true;
	}
	return false;
}
#endif

static bool RelativeFrom(std::string full_path, const char *pPathId, std::string &out) {
	const char *path_id_path;
	char dest[4096];
	dest[4095] = 0;
	path_id_path = g_pFullFileSystem->RelativePathToFullPath("./", pPathId, dest, sizeof dest - 1);
	if (!path_id_path)
		return false;
	std::string path_id = path_id_path;
	if (!Canonicalize(path_id) || !Canonicalize(full_path))
		return false;
	return ToRelative(path_id, full_path, out);
}