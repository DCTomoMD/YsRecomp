/* Framed screen read from the game: see ys_screen.h */
#include "app/ys_screen.h"

#include "app/intro_skip.h"

#include "app/crop.h"
#include "profile/ys_profile.h"

void ys_screen_init(YsScreen *s)
{
    s->program = -1;
    s->loads = 0;
}

static uint8_t read_logical(Pce *pce, uint16_t addr)
{
    return pce_bus_read(pce, (uint32_t)pce->cpu.mpr[addr >> 13] * 0x2000 + (addr & 0x1FFF));
}

void ys_screen_hook(YsScreen *s, Pce *pce)
{
    const YsScreenProfile *p = g_ys_profile.screen;
    if ((pce->cpu.pc != p->loader_pc && pce->cpu.pc != p->boot_pc) || pce->cpu.mpr[2] != p->bank)
        return;
    /* The 1st program (the one the boot already put at $4800) is called directly, without the loader; also after a
     * reset */
    if (pce->cpu.pc == p->boot_pc) {
        for (int k = 0; k < (int)sizeof(p->boot_sig); k++)
            if (read_logical(pce, (uint16_t)(p->boot_pc + k)) != p->boot_sig[k])
                return;
        s->program = -1;
        return;
    }
    for (int k = 0; k < (int)sizeof(p->loader_sig); k++)
        if (read_logical(pce, (uint16_t)(p->loader_pc + k)) != p->loader_sig[k])
            return;
    s->program = pce->cpu.a;
    s->loads++;
}

int ys_screen_in_opening(int program, long frame)
{
    return program == g_ys_profile.screen->intro_program || (program == -1 && frame < INTRO_MAX_FRAMES);
}

int ys_screen_layout(int program)
{
    const YsScreenProfile *p = g_ys_profile.screen;
    /* in the order of the former switch: gameplay (Ys I, Ys II), then the load menu */
    if (program == p->game1_program || program == p->game2_program)
        return CROP_SCREEN_GAME;
    if (program == p->load_program)
        return CROP_SCREEN_LOAD;
    return CROP_SCREEN_NONE;
}
