/* The operating system's choice of the user-data folder, used by host/common/host_paths_sdl.c only.
 * Each OS folder (host/win, host/mac) implements it; the platform's CMake file picks one. */
#ifndef PCE_HOST_PATHS_OS_H
#define PCE_HOST_PATHS_OS_H

/* base: the executable's folder (SDL_GetBasePath, may be ""); app_id: as given to host_paths_init (may be NULL).
 * Returns the user-data folder, ending with a separator; the string lives for the whole run. */
const char *host_paths_os_user_dir(const char *base, const char *app_id);

#endif
