/* Ys's mid-image scroll changes, read from the game: see ys_scroll.h */
#include "app/ys_scroll.h"

#include <string.h>

#include "pce/vce.h"
#include "profile/ys_profile.h"

#define RAM(p, a) ((p)->ram[(a) - 0x2000])

static uint8_t read_logical(Pce *pce, uint16_t addr)
{
    return pce_bus_read(pce, (uint32_t)pce->cpu.mpr[addr >> 13] * 0x2000 + (addr & 0x1FFF));
}

/* Lowest and highest PC of the hooks: a quick test before the table (computed once from the profile) */
static uint16_t g_pc_min, g_pc_max;
static int g_pc_range;

static void pc_range(const YsScrollProfile *p)
{
    g_pc_min = g_pc_max = p->hooks[0].pc;
    for (int h = 1; h < YS_SCROLL_HOOKS; h++) {
        if (p->hooks[h].pc < g_pc_min)
            g_pc_min = p->hooks[h].pc;
        if (p->hooks[h].pc > g_pc_max)
            g_pc_max = p->hooks[h].pc;
    }
    g_pc_range = 1;
}

static int which_hook(Pce *pce)
{
    const YsScrollProfile *p = g_ys_profile.scroll;
    uint16_t pc = pce->cpu.pc;
    if (!g_pc_range)
        pc_range(p);
    if (pc < g_pc_min || pc > g_pc_max || pce->cpu.mpr[2] != p->bank)
        return -1;
    for (int h = 0; h < YS_SCROLL_HOOKS; h++) {
        if (p->hooks[h].pc != pc)
            continue;
        for (int k = 0; k < (int)sizeof(p->hooks[h].sig); k++)
            if (read_logical(pce, (uint16_t)(pc + k)) != p->hooks[h].sig[k])
                return -1;
        return h;
    }
    return -1;
}

static void clear(YsScrollFrame *f)
{
    memset(f, 0, sizeof(*f));
    f->row_map = f->row_wrap = f->row_low = -1;
}

void ys_scroll_init(YsScroll *s)
{
    clear(&s->cur);
}

/* Image line the console is drawing (-1 outside it) */
static int image_row(Pce *pce)
{
    int r = pce->vce.vpos - VCE_ACTIVE_FIRST_LINE;
    return r >= 0 && r < VCE_ACTIVE_LINES ? r : -1;
}

void ys_scroll_hook(YsScroll *s, Pce *pce)
{
    int h = which_hook(pce);
    if (h < 0)
        return;
    const YsScrollProfile *p = g_ys_profile.scroll;
    YsScrollFrame *f = &s->cur;
    int row = image_row(pce);
    switch (h) {
    case YS_SCROLL_HOOK_MAP:
        clear(f);
        f->map_x = (uint16_t)(RAM(pce, p->map_x) | RAM(pce, p->map_x + 1) << 8);
        f->map_y = RAM(pce, p->map_line);
        f->cr = RAM(pce, p->cr);
        f->row_map = (int16_t)row;
        break;
    case YS_SCROLL_HOOK_WRAP:
        if (f->row_map < 0)
            break;
        f->wrap = 1;
        f->wrap_x = (RAM(pce, p->wrap_flags) & p->wrap_x_zero)
                        ? 0
                        : (uint16_t)(RAM(pce, p->map_x) | RAM(pce, p->map_x + 1) << 8);
        f->row_wrap = (int16_t)row;
        break;
    case YS_SCROLL_HOOK_LOW:
        if (f->row_map < 0 || f->row_low >= 0)
            break;
        /* $220E/$220F and $2212/$2213: System Card work area, kept in the code (review_jp/05, Q3) */
        f->low_x = (uint16_t)(RAM(pce, 0x220E) | RAM(pce, 0x220F) << 8);
        f->low_byr = (uint16_t)(RAM(pce, 0x2212) | RAM(pce, 0x2213) << 8);
        f->row_low = (int16_t)row;
        f->valid = 1;
        break;
    }
}

void ys_scroll_frame(YsScroll *s, YsScrollFrame *out)
{
    *out = s->cur;
    clear(&s->cur);
}
