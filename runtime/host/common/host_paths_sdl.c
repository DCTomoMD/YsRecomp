/* Resource and user-data folders (host/host_paths.h), with SDL only.
 * Resources: the executable's folder (SDL_GetBasePath), on every OS.
 * User data: chosen by the OS (host/host_paths_os.h): next to the executable on Windows,
 * ~/Library/Application Support/<app_id>/ on macOS (review_portability/05 3.1, decision D3).
 * The user folder is chosen on the first host_user_dir() call: the OS may create it then (macOS), so runs that never
 * use it (a disc given on the command line, tests with --config) leave nothing in the user's folders. */
#include "host/host_paths.h"
#include "host/host_paths_os.h"

#include <SDL3/SDL.h>

static int g_ready, g_user_ready;
static const char *g_resource = "";
static const char *g_user = "";
static char g_app_id[64];

int host_paths_init(const char *app_id)
{
    const char *base = SDL_GetBasePath(); /* owned by SDL, kept for the whole run */
    g_resource = base ? base : "";
    SDL_strlcpy(g_app_id, app_id ? app_id : "", sizeof(g_app_id));
    g_user_ready = 0;
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
    if (!g_user_ready) {
        const char *user = host_paths_os_user_dir(g_resource, g_app_id[0] ? g_app_id : NULL);
        g_user = user ? user : g_resource;
        g_user_ready = 1;
    }
    return g_user;
}
