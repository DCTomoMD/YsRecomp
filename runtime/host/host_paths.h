/* Where the program reads its resources and writes the player's data.
 * Host layer: plain C, no OS headers, no edition headers. The caller passes the app id as a plain string. */
#ifndef PCE_HOST_PATHS_H
#define PCE_HOST_PATHS_H

/* Called once by main.c before the other functions (calling them first initializes with app_id = NULL).
 * app_id: a short ASCII folder-safe name chosen by the caller. 0 = ok. */
int host_paths_init(const char *app_id);

/* Read-only resources (launcher images, ...). Empty or ends with a separator. */
const char *host_resource_dir(void);

/* Writable user data (ini; saves/, sessions/, menu_font.bin and launcher_sfx.bin follow the ini's folder).
 * Empty or ends with a separator. Next to the executable on Windows; ~/Library/Application Support/<app_id>/ on macOS,
 * created on the first call. With PCE_USER_DIR=FOLDER (tests), that folder instead, on every OS. */
const char *host_user_dir(void);

#endif
