/* Intro screen and RUN accepted, read from the intro program: see ys_intro.h */
#include "app/ys_intro.h"

#include <string.h>

#include "profile/ys_profile.h"

#define RAM(p, a) ((p)->ram[(a) - 0x2000])

/* The screen each script operation of g_ys_profile.intro->screens starts (in the order of YS_INTRO_OP_*) */
static const IntroScreen k_screen_of[YS_INTRO_OPS] = {
    [YS_INTRO_OP_NEC] = INTRO_SCREEN_NEC,
    [YS_INTRO_OP_LOGO] = INTRO_SCREEN_LOGO,
    [YS_INTRO_OP_COPYRIGHT] = INTRO_SCREEN_COPYRIGHT,
};

void ys_intro_init(YsIntro *s)
{
    s->screen = INTRO_SCREEN_LOADING;
}

static int code_is(Pce *pce, uint16_t addr, const uint8_t *bytes, int n)
{
    for (int k = 0; k < n; k++) {
        uint16_t a = (uint16_t)(addr + k);
        if (pce_bus_read(pce, (uint32_t)pce->cpu.mpr[a >> 13] * 0x2000 + (a & 0x1FFF)) != bytes[k])
            return 0;
    }
    return 1;
}

void ys_intro_hook(YsIntro *s, Pce *pce, int program)
{
    const YsIntroProfile *p = g_ys_profile.intro;
    uint16_t pc = pce->cpu.pc;
    if ((pc != p->script_pc && pc != p->menu_pc) || program != g_ys_profile.screen->intro_program ||
        pce->cpu.mpr[2] != p->bank)
        return;
    if (pc == p->menu_pc) {
        if (code_is(pce, p->menu_pc, p->menu_sig, sizeof(p->menu_sig)))
            s->screen = INTRO_SCREEN_MENU;
        return;
    }
    if (!code_is(pce, p->script_pc, p->script_sig, sizeof(p->script_sig)))
        return;
    uint16_t op = (uint16_t)(RAM(pce, p->script) | RAM(pce, p->script + 1) << 8);
    for (int i = 0; i < YS_INTRO_OPS; i++)
        if (op == p->screens[i].addr && k_screen_of[i] > s->screen &&
            code_is(pce, op, p->screens[i].op, sizeof(p->screens[i].op)))
            s->screen = k_screen_of[i];
}

int ys_intro_run_accepted(const Pce *pce, int program)
{
    const YsIntroProfile *p = g_ys_profile.intro;
    return program == g_ys_profile.screen->intro_program && (RAM(pce, p->mask) & PCE_PAD_RUN) &&
           RAM(pce, p->lock) == 0 && !(RAM(pce, p->flags) & p->hurry);
}
