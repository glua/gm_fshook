#ifndef OPENRESULT_H
#define OPENRESULT_H

#include <mutex>
#include <vector>
#include "threadtools.h"
#include "GarrysMod/Lua/Interface.h"
#include "vfuncs.h"

class OpenResult {
public:
	OpenResult(const char *relativetobase) {
		file = relativetobase;
	}

	bool GetResult() {
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

	bool RunLua() {
		auto lua = FunctionHooks->lua;
		lua->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		lua->GetField(-1, "hook");
		lua->GetField(-1, "Run");
		lua->PushString("ShouldHideFile");
		lua->PushString(file);
		lua->Call(2, 1);
		bool ret = !lua->GetBool(-1);
		lua->Pop(3);
		return ret;
	}

public:
	static std::mutex m;
	static std::vector<OpenResult *> waiting_list;
	const char *file;
	bool has_result;
	bool result;
};
#endif // OPENRESULT_H