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
	std::string redirectPathID;
};

class OpenResult {
public:
	OpenResult(const char *relativetobase) {
		file = relativetobase;
	}

	IOpenResult GetResult() {
		if (ThreadGetCurrentId() == FunctionHooks->mainthread) {
			RunLua();
		}
		else {
			OpenResult::m.lock();
			this->has_result = false;
			OpenResult::waiting_list.push_back(this);
			OpenResult::m.unlock();

			while (!this->has_result)
				ThreadSleep(2);
		}

		return this->result;
	}

	void RunLua() {
		auto lua = FunctionHooks->lua;
		lua->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		lua->GetField(-1, "hook");
		lua->GetField(-1, "Run");
		lua->PushString("ShouldHideFile");
		lua->PushString(file);
		lua->Call(2, 2);

		if (lua->IsType(-2,GarrysMod::Lua::Type::BOOL)) {
			this->result.shouldOpen = !lua->GetBool(-2);
			this->result.shouldRedirect = false;
		}
		else if (lua->IsType(-2,GarrysMod::Lua::Type::STRING)) {
			this->result.shouldOpen = true;
			this->result.shouldRedirect = true;
			this->result.redirect = std::string(lua->GetString(-2));

			if (lua->IsType(-1, GarrysMod::Lua::Type::STRING)) {
				this->result.redirectPathID = std::string(lua->GetString(-1));
			}
			else {
				this->result.redirectPathID = "GAME";
			}
		}
		else {
			this->result.shouldOpen = true;
			this->result.shouldRedirect = false;
		}

		lua->Pop(4);
	}

public:
	static std::mutex m;
	static std::vector<OpenResult *> waiting_list;
	const char *file;
	bool has_result;
	IOpenResult result;
};
#endif // OPENRESULT_H