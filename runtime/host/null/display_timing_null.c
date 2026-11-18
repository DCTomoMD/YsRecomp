/* No compositor refresh time on this OS (host/host_display_timing.h): smooth motion pacing goes on with its own
 * estimate. Said once on stderr, so the disabled feature is visible in the log. */
#include "host/host_display_timing.h"

#include <stdio.h>

int vblank_query(uint64_t *next_qpc, uint64_t *period_qpc)
{
    static int said;
    (void)next_qpc;
    (void)period_qpc;
    if (!said) {
        said = 1;
        fprintf(stderr, "host: display refresh timing not available on this platform (pacing uses its own estimate)\n");
    }
    return 0;
}
