/* No GPU time measurement on this OS (host/host_gpu_timing.h): --frame-timing says so and goes on without it. */
#include "host/host_gpu_timing.h"

#include <stdio.h>

int gpu_time_init(SDL_Renderer *r)
{
    (void)r;
    fprintf(stderr, "host: GPU time measurement not available on this platform (--frame-timing shows no gpu line)\n");
    return 0;
}
void gpu_time_mark(SDL_Renderer *r, int k)
{
    (void)r;
    (void)k;
}
void gpu_time_presented(SDL_Renderer *r, uint64_t target_ns)
{
    (void)r;
    (void)target_ns;
}
void gpu_time_report(void)
{
}
void gpu_time_quit(void)
{
}
