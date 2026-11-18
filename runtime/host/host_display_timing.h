/* Time of the next screen refresh and the period, on the SDL_GetPerformanceCounter clock. 0 = unknown
 * (no implementation for this OS, or the compositor did not answer).
 * Host layer: plain C interface; the OS implementation is chosen by CMake (host/win, host/null). */
#ifndef PCE_HOST_DISPLAY_TIMING_H
#define PCE_HOST_DISPLAY_TIMING_H

#include <stdint.h>

int vblank_query(uint64_t *next_qpc, uint64_t *period_qpc);

#endif
