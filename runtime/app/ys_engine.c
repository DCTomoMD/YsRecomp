/* Ys engine read by the PC: see ys_engine.h and docs/ys_notes.md ("Engine memory map"). */
#include "app/ys_engine.h"

#include <string.h>

#include "profile/ys_profile.h"

YsEngine *g_ys_engine;

/* Console RAM ($2000-$3FFF) */
#define RAM(p, a) ((p)->ram[(a) - 0x2000])

/* Engine routines and the first bytes of each: g_ys_profile.engine */

static uint8_t read_logical(Pce *pce, uint16_t addr)
{
    return pce_bus_read(pce, (uint32_t)pce->cpu.mpr[addr >> 13] * 0x2000 + (addr & 0x1FFF));
}

/* Ys I mine light: copies the position of character Y to the light's */
static void copy_hook(YsEngine *e, Pce *pce)
{
    const YsEngineProfile *p = g_ys_profile.engine;
    if (pce->cpu.pc != p->copy_pc || pce->cpu.mpr[3] != p->copy_bank)
        return;
    for (int k = 0; k < (int)sizeof(p->copy_sig); k++)
        if (read_logical(pce, (uint16_t)(p->copy_pc + k)) != p->copy_sig[k])
            return;
    int x = read_logical(pce, p->copy_light), y = pce->cpu.y;
    if (x < YS_ACTORS && y < YS_ACTORS && x != y) {
        e->copy_of[x] = (uint8_t)y;
        e->copies++;
    }
}

/* Adol is born: the routine that creates the hero in the gameplay program (window $6000), with the new number in A.
 * $75 and $76 are only valid for the gameplay program: other programs use them as scratch, which is why the number is
 * noted here and not read from them later */
static void hero_hook(YsEngine *e, Pce *pce)
{
    const YsEngineProfile *p = g_ys_profile.engine;
    uint16_t pc = pce->cpu.pc;
    if (pc != p->hero_pc[0] && pc != p->hero_pc[1])
        return;
    for (int k = 0; k < (int)sizeof(p->hero_sig); k++)
        if (read_logical(pce, (uint16_t)(pc + k)) != p->hero_sig[k])
            return;
    e->hero = pce->cpu.a < YS_ACTORS ? pce->cpu.a : YS_NO_ACTOR;
}

/* Lowest and highest PC of the hooks: a quick test before the table (computed once from the profile) */
static uint16_t g_pc_min, g_pc_max;
static int g_pc_range;

static void pc_range(const YsEngineProfile *p)
{
    g_pc_min = g_pc_max = p->hooks[0].pc;
    for (int h = 1; h < YS_ENGINE_HOOKS; h++) {
        if (p->hooks[h].pc < g_pc_min)
            g_pc_min = p->hooks[h].pc;
        if (p->hooks[h].pc > g_pc_max)
            g_pc_max = p->hooks[h].pc;
    }
    g_pc_range = 1;
}

static int which_hook(Pce *pce)
{
    const YsEngineProfile *p = g_ys_profile.engine;
    uint16_t pc = pce->cpu.pc;
    if (!g_pc_range)
        pc_range(p);
    if (pc < g_pc_min || pc > g_pc_max || pce->cpu.mpr[2] != p->bank)
        return -1;
    for (int h = 0; h < YS_ENGINE_HOOKS; h++) {
        if (p->hooks[h].pc != pc)
            continue;
        for (int k = 0; k < (int)sizeof(p->hooks[h].sig); k++)
            if (read_logical(pce, (uint16_t)(pc + k)) != p->hooks[h].sig[k])
                return -1;
        return h;
    }
    return -1;
}

/* 16 bits at a RAM address, low byte first */
static uint16_t ram16(Pce *pce, uint16_t a)
{
    return (uint16_t)(RAM(pce, a) | RAM(pce, a + 1) << 8);
}

/* n bytes from address ptr of window $8000-$9FFF with bank bank (CD RAM only, $80-$87); 0 = outside it */
static int read_banked(Pce *pce, int bank, uint16_t ptr, uint8_t *out, int n)
{
    uint32_t a = (uint32_t)bank * 0x2000 + (ptr & 0x1FFF);
    if (bank < 0x80 || a + (uint32_t)n > 0x88u * 0x2000)
        return 0;
    for (int i = 0; i < n; i++)
        out[i] = pce_bus_read(pce, a + (uint32_t)i);
    return 1;
}

/* Tables of the map's front cells in use (ys_engine.h, YsFront) */
static void read_front(Pce *pce, YsFront *f)
{
    const YsEngineProfile *p = g_ys_profile.engine;
    f->base = ram16(pce, p->front_base);
    f->count = ram16(pce, p->front_count);
    uint16_t attr = ram16(pce, p->front_attr);
    uint16_t bits = ram16(pce, p->front_bits);
    if (!read_banked(pce, RAM(pce, p->front_attr_bank), attr, f->attr, 256) ||
        !read_banked(pce, RAM(pce, p->front_bits_bank), bits, f->bits, 256 * 8))
        f->count = 0;
}

void ys_engine_init(YsEngine *e)
{
    memset(e, 0, sizeof(*e));
    memset(e->copy_of, YS_NO_ACTOR, sizeof(e->copy_of));
    e->hero = YS_NO_ACTOR;
}

/* The VDC started copying the sprite table (from R13): the shown list becomes the newest one with that
 * address finished before the copy */
static void sat_copy(YsEngine *e, uint16_t base)
{
    e->shown_valid = 0;
    for (long k = 0; k < 4 && k < e->done_count; k++) {
        const YsList *l = &e->done[(e->done_count - 1 - k) & 3];
        if (l->base == base) {
            e->shown = *l;
            e->shown_valid = 1;
            return;
        }
    }
}

void ys_engine_hook(Pce *pce, int after)
{
    YsEngine *e = g_ys_engine;
    if (after || !e)
        return;
    /* By the VDC's counter: a long TIA may contain the whole copy, and the shown list went stale */
    if (pce->vdc.sat_copies != e->sat_copies) {
        e->sat_copies = pce->vdc.sat_copies;
        sat_copy(e, pce->vdc.sat_copy_base);
    }
    copy_hook(e, pce);
    hero_hook(e, pce);
    int h = which_hook(pce);
    if (h < 0)
        return;
    const YsEngineProfile *p = g_ys_profile.engine;
    const YsCommonRam *ram = g_ys_profile.common;
    YsList *l = &e->cur;
    switch (h) {
    case YS_ENGINE_HOOK_START:
        memset(l, 0, sizeof(*l));
        memset(l->owner, YS_NO_ACTOR, sizeof(l->owner));
        e->building = 1;
        e->cur_actor = YS_NO_ACTOR;
        break;
    case YS_ENGINE_HOOK_ACTOR: {
        if (!e->building)
            break;
        int x = pce->cpu.x & (YS_ACTORS - 1);
        e->cur_actor = x;
        l->drawn[x] = 1;
        l->ax[x] = (int16_t)(RAM(pce, p->actor_x_lo + x) | RAM(pce, p->actor_x_hi + x) << 8);
        l->ay[x] = (int16_t)(RAM(pce, p->actor_y_lo + x) | RAM(pce, p->actor_y_hi + x) << 8);
        l->flags[x] = RAM(pce, p->actor_flags + x);
        l->pose[x] = RAM(pce, p->actor_pose + x);
        l->state[x] = RAM(pce, p->actor_state + x);
        break;
    }
    case YS_ENGINE_HOOK_PIECE: {
        int slot = RAM(pce, 0x2216);
        if (e->building && slot < YS_ACTORS)
            l->owner[slot] = (uint8_t)e->cur_actor;
        break;
    }
    case YS_ENGINE_HOOK_END:
        if (!e->building)
            break;
        e->building = 0;
        l->seq = ++e->done_count;
        l->count = RAM(pce, 0x2216);
        l->cam_x = ram16(pce, p->camera_x);
        l->cam_y = ram16(pce, p->camera_y);
        l->base = pce->vdc.reg[0x13];
        for (int i = 0; i < YS_ACTORS * 4; i++)
            l->sat[i] = pce->vdc.vram[(l->base + i) & (VDC_VRAM_SIZE - 1)];
        for (int w = 0; w < YS_WALKERS; w++) {
            l->w_actor[w] = RAM(pce, ram->walker_actor + w);
            l->w_vel[w] = RAM(pce, p->walker_vel + w);
            l->w_acc[w] = RAM(pce, p->walker_acc + w);
            l->w_left[w] = RAM(pce, p->walker_left + w);
            l->w_mask[w] = RAM(pce, p->walker_mask + w);
            l->w_next[w] = RAM(pce, p->walker_next + w);
            l->w_hp[w] = RAM(pce, p->walker_hp + w);
            l->w_hpmax[w] = RAM(pce, p->walker_hpmax + w);
        }
        l->exp = ram16(pce, ram->exp);
        l->gold = ram16(pce, ram->gold);
        l->unit = RAM(pce, p->unit);
        read_front(pce, &l->front);
        memcpy(l->copy_of, e->copy_of, sizeof(l->copy_of));
        memset(e->copy_of, YS_NO_ACTOR, sizeof(e->copy_of));
        l->hero = e->hero;
        e->done[(l->seq - 1) & 3] = *l;
        /* ex_sprdma set this list's R13 and the copy started before the routine got here: sat_copy only had the
         * previous list with the same address, but the VDC is copying this one (it reads the table word by word, 1024 cycles) */
        if (pce->vdc.sat_transfer_pending > 0 && pce->vdc.reg[0x13] == l->base) {
            e->shown = *l;
            e->shown_valid = 1;
        }
        break;
    }
}

void ys_engine_frame(YsEngine *e, YsFrameList *out)
{
    out->kind = YS_LIST_NONE;
    out->shown_ok = 0;
    if (!e->shown_valid) {
        e->prev_shown = e->shown_seq = 0;
        return;
    }
    out->shown_ok = 1;
    out->shown = e->shown;
    out->shown_cam_x = e->shown.cam_x;
    out->shown_cam_y = e->shown.cam_y;
    long seq = e->shown.seq, want = e->shown_seq + 1;
    /* The list after the previous frame's, if the VDC repeated one (the next one finished after the copy) or if we were
     * already ahead of it: after a delay, the following ones also come out one late (the console shows 1, 1, 2,
     * 4: 3 never appears) until it skips one */
    const YsList *pick = &e->shown;
    if (e->shown_seq && want > seq && want - seq <= 2 && (seq == e->prev_shown || e->shown_seq > e->prev_shown)) {
        const YsList *c = &e->done[(want - 1) & 3];
        const YsList *held = &e->done[(e->shown_seq - 1) & 3];
        if (c->seq == want)
            pick = c;
        else if (e->shown_seq > seq && held->seq == e->shown_seq)
            pick = held; /* no new one: keeps the one already on screen */
    }
    out->kind = pick == &e->shown ? YS_LIST_SHOWN : YS_LIST_LATE;
    out->list = *pick;
    e->late_frames += out->kind == YS_LIST_LATE;
    e->shown_seq = pick->seq;
    e->prev_shown = seq;
}
