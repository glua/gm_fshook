#ifndef OPENRESULT_H
#define OPENRESULT_H

#include <mutex>
#include <vector>
#include "threadtools.h"
#include "GarrysMod/Lua/Interface.h"
#include "vfuncs.h"

class OpenResult {
public:
	OpenResult(std::string relativetobase, std::string full_path, std::string pathid = "") {
		relative = relativetobase;
		full = full_path;
		path = pathid;
	}

	bool GetResult() {
		return RunLua();
	}

	bool RunLua() {
		auto lua = FunctionHooks->lua;
		lua->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		lua->GetField(-1, "hook");
		lua->GetField(-1, "Run");
		lua->PushString("ShouldHideFile");
		lua->PushString(relative.c_str());
		lua->PushString(full.c_str());
		lua->PushString(path.c_str());
		lua->Call(4, 1);
		bool ret = !lua->GetBool(-1);
		lua->Pop(3);
		return ret;
	}

public:
	std::string relative, full, path;
};
#endif // OPENRESULT_H