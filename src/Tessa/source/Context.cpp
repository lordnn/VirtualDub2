#include <stdio.h>
#include <stdarg.h>
#include <vd2/Tessa/Context.h>

void VDTBeginScopeF(IVDTProfiler *profiler, uint32 color, const char *format, ...) {
	char buf[256];
	buf[0] = 0;

	va_list val;
	va_start(val, format);
	_vsnprintf_s(buf, _TRUNCATE, format, val);
	va_end(val);

	profiler->BeginScope(color, buf);
}
