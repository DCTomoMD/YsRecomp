/* Process priority. Host layer: plain C interface; the OS implementation is chosen by CMake (host/win, host/null). */
#ifndef PCE_HOST_PROCESS_H
#define PCE_HOST_PROCESS_H

/* The whole process above normal priority (also the video driver and sound threads): with all cores
 * busy (12 Python loops), at 180 Hz 4977 images came out instead of ~7100; this way, 7021. Without an
 * implementation for this OS, nothing. */
void vblank_raise_priority(void);

#endif
