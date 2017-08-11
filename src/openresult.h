#ifndef OPENRESULT_H
#define OPENRESULT_H

#include <mutex>
#include <vector>
#include <string>
#include "threadtools.h"
#include "GarrysMod/Lua/Interface.h"
#include "vfuncs.h"

struct IOpenResult {
	bool shouldOpen;
	bool shouldRedirect;
	std::string redirect;
};

class OpenResult {
public:
	OpenResult(const char *relativetobase) {
		file = relativetobase;
	}

	IOpenResult GetResult() {
		if (ThreadGetCurrentId() == FunctionHooks->mainthread) {
			return RunLua();
		}
		OpenResult::m.lock();
		this->has_result = false;
		OpenResult::waiting_list.push_back(this);
		OpenResult::m.unlock();

		while (!this->has_result)
			ThreadSleep(2);

		return this->result;
	}

	IOpenResult RunLua() {
		auto lua = FunctionHooks->lua;
		lua->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		lua->GetField(-1, "hook");
		lua->GetField(-1, "Run");
		lua->PushString("ShouldHideFile");
		lua->PushString(file);
		lua->Call(2, 1);

		IOpenResult ret;

		if (lua->IsType(-1,GarrysMod::Lua::Type::BOOL)) {
			ret.shouldOpen = !lua->GetBool(-1);
			ret.shouldRedirect = false;
		}
		else if (lua->IsType(-1,GarrysMod::Lua::Type::STRING)) {
			ret.shouldOpen = true;
			ret.shouldRedirect = true;
			ret.redirect = lua->GetString(-1);
		}
		lua->Pop(3);

		return ret;
	}

public:
	static std::mutex m;
	static std::vector<OpenResult *> waiting_list;
	const char *file;
	bool has_result;
	IOpenResult result;
};
#endif // OPENRESULT_H