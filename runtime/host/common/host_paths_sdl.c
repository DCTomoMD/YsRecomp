/* Resource and user-data folders (host/host_paths.h), with SDL only.
 * Today, on every OS: both are the executable's folder (SDL_GetBasePath), as before the host layer existed.
 * The macOS policy (user data in SDL_GetPrefPath, review_portability/05 3.1) comes in M3. */
#include "host/host_paths.h"

#include <SDL3/SDL.h>

static int g_ready;
static const char *g_resource = "";
static const char *g_user = "";

int host_paths_init(const char *app_id)
{
    (void)app_id; /* used by the M3 policy (per-app user folder) */
    const char *base = SDL_GetBasePath(); /* owned by SDL, kept for the whole run */
    g_resource = base ? base : "";
    g_user = g_resource;
    g_ready = 1;
    return 0;
}

const char *host_resource_dir(void)
{
    if (!g_ready)
        host_paths_init(NULL);
    return g_resource;
}

const char *host_user_dir(void)
{
    if (!g_ready)
        host_paths_init(NULL);
    return g_user;
}
