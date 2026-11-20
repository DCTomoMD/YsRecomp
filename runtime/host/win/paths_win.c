/* Windows: the user's data next to the executable (the portable package; the same as before the host layer). */
#include "host/host_paths_os.h"

const char *host_paths_os_user_dir(const char *base, const char *app_id)
{
    (void)app_id;
    return base;
}
