#include <cstdint>

namespace {
	using printer_type = void (*)(const char*, ...);
	using sleep_type = void (*)(std::uint32_t);
}

struct module_param {
	const char *name;
	printer_type printer;
	sleep_type sleep;
};

int g_counter = 0x0;

const char *TEST_MESSAGES[5] = {
	"Hello,", "World", "from", "module", "!"
};


namespace  sandbox {
	void call(module_param *param, const char *desc);
}

extern "C" int module_entry(module_param *param) {
	const char *p = "\r\n==========\r\n";
	param->printer(p);
	for(int i=0; i<5; ++i) {
		param->printer("%s %i\r\n", TEST_MESSAGES[i], g_counter++);
		g_counter++;
	}
	param->printer("Hello from module!\r\n");
	param->printer("\r\n==========\r\n");
	sandbox::call(param, "Call 1");
	sandbox::call(param, "Call 2");
	while(1) {
		param->printer("%s %i %p\r\n", param->name, g_counter++, (void *)&g_counter);
		param->sleep(10000);
	}
	return 0;
}

namespace  sandbox {
void call(module_param *param, const char *desc) {
	param->printer("Hello from module CALL %s!\r\n", desc);
	
} 
}
