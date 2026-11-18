/* Process priority on Windows (host/host_process.h). Windows host (compiled only on Windows). */
#include "host/host_process.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void vblank_raise_priority(void)
{
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
}
