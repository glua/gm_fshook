#include <string>
#include <algorithm>
#include <exception>



static void Canonicalize(std::string &path);
static void ToRelative(std::string from, std::string to, std::string &out);
static void ToFull(std::string relative, const char *pathID, std::string &out);
extern void FSLog(std::string msg);

class FSException : public std::exception {
public:
	FSException(std::string s, bool log = true) {
		if (log)
			FSLog(s);
		str = s;
	}

	const char *what() const throw() override {
		return str.c_str();
	}
public:
	std::string str;
};

#ifdef _WIN32 
#include <Windows.h>
#include <Shlwapi.h>
void Canonicalize(std::string &path) {
	std::replace(path.begin(), path.end(), '/', '\\');
	char out[MAX_PATH + 1];
	BOOL ret = PathCanonicalizeA(out, path.c_str());
	out[MAX_PATH] = 0;
	if (ret == FALSE)
		throw FSException("PathCanonicalizeA returned FALSE");
	path = out;
	std::replace(path.begin(), path.end(), '\\', '/');
}

void ToRelative(std::string from, std::string to, std::string &out) {
	std::replace(from.begin(), from.end(), '/', '\\');
	std::replace(to.begin(), to.end(), '/', '\\');
	char outc[MAX_PATH + 1];
	BOOL ret = PathRelativePathToA(outc, from.c_str(), FILE_ATTRIBUTE_DIRECTORY, to.c_str(), 0);
	outc[MAX_PATH] = 0;
	if (ret == FALSE)
		throw FSException("PathRelativePathToA returned FALSE (" + from + ", " + to + ")", false);
	out = outc;
	out = out.substr(2);
	std::replace(out.begin(), out.end(), '\\', '/');
}

void ToFull(std::string relative, const char *pathID, std::string &out) {
	char full_path_c[4096];
	full_path_c[4095];
	if (!g_pFullFileSystem->RelativePathToFullPath("./", pathID, full_path_c, sizeof full_path_c - 1))
		throw FSException(std::string("Cannot find ") + (pathID ? pathID : "NULL") + "'s full path for " + relative);
	char outc[MAX_PATH];
	LPSTR ret = PathCombineA(outc, full_path_c, relative.c_str());
	outc[MAX_PATH - 1] = 0;
	if (ret == NULL)
		throw FSException("PathCombineA returned NULL");
	out = outc;
}
#endif

static void RelativeFrom(std::string full_path, const char *pPathId, std::string &out) {
	const char *path_id_path;
	char dest[4096];
	dest[4095] = 0;
	path_id_path = g_pFullFileSystem->RelativePathToFullPath("./", pPathId, dest, sizeof dest - 1);
	if (!path_id_path)
		throw FSException(std::string("Cannot find ") + (pPathId ? pPathId : "NULL") + "'s full path");
	std::string path_id = path_id_path;
	Canonicalize(path_id);
	Canonicalize(full_path);
	ToRelative(path_id, full_path, out);
}