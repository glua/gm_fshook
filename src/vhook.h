#ifndef VHOOK_H
#define VHOOK_H
#include <cstdint>

#if !defined(__linux) && !defined(__APPLE__) && !defined(_WIN32)
#error "this is not supported on non apple/linux/win32"
#endif


template <typename Class>
class VirtualReplacer {
public:
	template <typename t>
	union _u_addr {
		void *addr;
		t func;
	};
	template <typename t>
	union _u_addr_linux {
		t func;
		int offset_plus_one;
	};

	using address_t = void *;
	template <typename RetType, typename... Args>
	using classcall = RetType (Class::*)(Args...);

	// can be used with interfaces and implementations
	template<typename RetType, typename... Args>
	static address_t GetVirtualAddress(Class *instance, classcall<RetType, Args...> ClassCaller) {
#ifdef _WIN32
		unsigned char *addr = *(unsigned char **)&ClassCaller;
		// check for jmp functions
		if (addr[0] == 0x8B)
			addr += 2;
		if (addr[0] == 0xFF && ((addr[1] >> 4) & 3) == 2) {
			unsigned char jumptype = addr[1] >> 6;
			std::uint32_t offset;
			if (jumptype == 1) { // byte
				offset = addr[2];
			}
			else if (jumptype == 2) {
				offset = *(std::uint32_t *)&addr[2];
			}
			return (*(address_t **)instance)[offset / 4];
		}
		return *(address_t *)&ClassCaller;
#elif defined(__linux)
		// g++ always compiles with offset + 1
		_u_addr_linux<classcall<RetType, Args...>> u;
		u.func = ClassCaller;
		int offset = (u.offset_plus_one - 1) / sizeof(address_t);
		return (*(address_t **)instance)[offset];
#elif defined(__APPLE__) 
		//??
#endif
	}

	template<typename RetType, typename... Args>
	static int GetVirtualIndex(Class *instance, classcall<RetType, Args...> Caller) {
		address_t address = GetVirtualAddress<RetType, Args...>(instance, Caller);
		address_t *vtable = *(address_t **)instance;
		int index = 0;
		while (vtable[index]) {
			if (vtable[index] == address)
				break;
			index++;
		}
		return vtable[index] ? index : -1;
	}

	VirtualReplacer(Class *instance, bool deinit = false) {
		this->deinit = deinit;
		this->instance = instance;

		int size = 0;

		old_vtable = *(address_t **)instance;

		address_t *vtable_end = old_vtable;

		while (vtable_end[size++]);
		size--;

		new_vtable = new address_t[size];

		for (int i = 0; i < size; i++)
			new_vtable[i] = old_vtable[i];

		*(address_t **)instance = new_vtable;
	}
	~VirtualReplacer() {
		if (deinit) {
			*(address_t **)instance = old_vtable;
		}
		delete new_vtable;
	}
	void *Hook(int index, address_t func) {
		new_vtable[index] = func;
		return old_vtable[index];
	}
	template <typename RetType, typename... Args> 
	RetType Call(void *&func, Args... args) {
		auto typedfunc = (RetType (Class::* *)(Args...))&func;
		return (instance->**typedfunc)(args...);
	}
	template <typename RetType, typename... Args>
	RetType Call(int funcindex, Args... args) {
		return Call<RetType, Args...>(old_vtable[funcindex], args...);
	}

public:
	bool deinit;
	address_t *old_vtable;
	Class *instance;
	address_t *new_vtable;
};


template <typename Class, typename RetType, typename... Args>
static void *GetVirtualAddress(Class *instance, RetType(Class::* ClassCaller)(Args...)) {
	return VirtualReplacer<Class>::GetVirtualAddress(instance, ClassCaller);
}
template <typename Class, typename RetType, typename... Args>
static int GetVirtualIndex(Class *instance, RetType(Class::* ClassCaller)(Args...)) {
	return VirtualReplacer<Class>::GetVirtualIndex(instance, ClassCaller);
}

#endif // VHOOK_H