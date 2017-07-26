#ifndef VHOOK_H
#define VHOOK_H

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

	VirtualReplacer(Class *instance, bool deinit = false) {
		this->deinit = deinit;
		this->instance = instance;

		int size = 0;

		old_vtable = *(void ***)instance;

		void **vtable_end = old_vtable;

		while (vtable_end[size++]);
		size--;

		new_vtable = new void *[size];

		for (int i = 0; i < size; i++)
			new_vtable[i] = old_vtable[i];

		*(void ***)instance = new_vtable;
	}
	~VirtualReplacer() {
		if (deinit) {
			*(void ***)instance = old_vtable;
		}
		delete new_vtable;
	}
	template <typename t>
	void *Hook(int index, t func) {
		_u_addr<t> u;
		u.func = func;
		new_vtable[index] = u.addr;
		return old_vtable[index];
	}
	template <typename RetType, typename... Args> 
	RetType Call(void *&func, Args... args) {
		auto typedfunc = (RetType (Class::* *)(Args... args))&func;
		return instance->**typedfunc(args...);
	}
	template <typename RetType, typename... Args>
	RetType Call(int funcindex, Args... args) {
		auto typedfunc = (RetType (Class::* *)(Args...))&old_vtable[funcindex];
		return (instance->**typedfunc)(args...);
	}

public:
	bool deinit;
	void **old_vtable;
	Class *instance;
	void **new_vtable;
};

#endif // VHOOK_H