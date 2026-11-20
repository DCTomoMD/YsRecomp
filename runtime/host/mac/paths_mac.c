/* macOS: the user's data in ~/Library/Application Support/<app_id>/ (decision D3; no organization folder, H2),
 * because the .app bundle is read-only once signed and may sit in /Applications. Created by SDL if missing.
 * Without an app id, or if SDL cannot make the folder, the executable's folder (as on Windows). */
#include "host/host_paths_os.h"

#include <SDL3/SDL.h>

const char *host_paths_os_user_dir(const char *base, const char *app_id)
{
    if (!app_id || !*app_id)
        return base;
    char *pref = SDL_GetPrefPath("", app_id); /* kept for the whole run */
    if (!pref) {
        SDL_Log("host_paths: no user folder for %s (%s): using the executable's folder", app_id, SDL_GetError());
        return base;
    }
    return pref;
}
