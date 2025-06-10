#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

#include "aikartos/sdk/aikartos_api.h"

int simple_vsnprintf(char *buf, size_t size, const char *fmt, va_list args) {
	char *out = buf;
	const char *end = buf + size;
	auto putc = [&](char c) {
		if (out + 1 < end) {
			*out++ = c;
		}
	};

	while (*fmt) {
		if (*fmt != '%') {
			putc(*fmt++);
			continue;
		}
		++fmt;
		if (*fmt == 'd' || *fmt == 'i' || *fmt == 'x' || *fmt == 'p') {
			int num = va_arg(args, int);
			unsigned int unum = (unsigned int)(*fmt == 'd' && num < 0 ? -num : num);
			if (*fmt == 'd' && num < 0) {
				putc('-');
			}

			char tmp[11];
			int base = (*fmt == 'x' || *fmt == 'p') ? 16 : 10;
			int i = 0;
			do {
				int digit = unum % base;
				tmp[i++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
				unum /= base;
			} while (unum && i < 10);

			while (i--) {
				putc(tmp[i]);
			}
		}
		else if (*fmt == 'c') {
			char c = (char)va_arg(args, int);
			putc(c);
		}
		else if (*fmt == 's') {
			const char *s = va_arg(args, const char *);
			while (*s)
				putc(*s++);
		}
		else {
			putc('%');
			putc(*fmt);
		}
		++fmt;
	}

	if (out < end) {
		*out = '\0';
	}
	else if (size > 0) {
		buf[size - 1] = '\0';
	}

	return out - buf;
}

namespace aikartos::sdk {
	using api = aikartos_api;
}

using uart_write_fn = void (*)(const char *buf, std::size_t len);

uart_write_fn g_write = nullptr;

template <std::size_t BufSize = 128>
void printer(const char *format, ...) {
	if (g_write == nullptr) {
		return; // No write function set
	}

	char buf[BufSize];
	va_list args;
	va_start(args, format);
	int len = simple_vsnprintf(buf, BufSize, format, args);
	va_end(args);

	if (len > 0) {
		if (static_cast<std::size_t>(len) > BufSize) {
			len = BufSize;
		}
		g_write(buf, len);
	}
}	

int g_counter = 0x0;

const char *TEST_MESSAGES[5] = {
	"Hello,", "World", "from", "module", "!"
};

namespace  sandbox {
	void call(const char *desc);
	void task(void *ptr) {
		aikartos::sdk::api *apis = reinterpret_cast<aikartos::sdk::api *>(ptr);
		printer("Hello from module TASK!\r\n");
		while(1) {
			printer("%s %i %p\r\n", "sandbox", g_counter++, (void *)&g_counter);
			apis->this_task.sleep(3000);
		}
	}
}

extern "C" int module_entry(aikartos::sdk::api *apis) {
	g_write = apis->device.uart_write;
	const char *p = "\r\n==========\r\n";
	printer(p);
	for(int i=0; i<5; ++i) {
		printer("%s %i\r\n", TEST_MESSAGES[i], g_counter++);
		g_counter++;
	}

	printer("Hello from module!\r\n");
	printer("Module base: %p, size: %d\r\n", (void *)apis->module.module_base, apis->module.module_size);
	printer("\r\n==========\r\n");
	sandbox::call("Call 1");
	sandbox::call("Call 2");
	apis->kernel.add_task(sandbox::task, apis);
	while(1) {
		printer("%s %i %p\r\n", "common", g_counter++, (void *)&g_counter);
		apis->this_task.sleep(10000);
	}
	return 0;
}

namespace  sandbox {
void call(const char *desc) {
	printer("Hello from module CALL %s!\r\n", desc);
	
} 
}
