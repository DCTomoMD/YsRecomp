/* Reward for killing an enemy, read from the game: see ys_reward.h */
#include "app/ys_reward.h"

#include <string.h>

#include "app/ys_engine.h"
#include "profile/ys_profile.h"

#define RAM(p, a) ((p)->ram[(a) - 0x2000])

static uint8_t read_logical(Pce *pce, uint16_t addr)
{
    return pce_bus_read(pce, (uint32_t)pce->cpu.mpr[addr >> 13] * 0x2000 + (addr & 0x1FFF));
}

/* Lowest and highest PC of the hooks: a quick test before the table (computed once from the profile) */
static uint16_t g_pc_min, g_pc_max;
static int g_pc_range;

static void pc_range(const YsRewardProfile *p)
{
    g_pc_min = 0xFFFF;
    g_pc_max = 0;
    for (int h = 0; h < p->hook_count; h++) {
        if (p->hooks[h].pc < g_pc_min)
            g_pc_min = p->hooks[h].pc;
        if (p->hooks[h].pc > g_pc_max)
            g_pc_max = p->hooks[h].pc;
    }
    g_pc_range = 1;
}

static int which_hook(Pce *pce)
{
    const YsRewardProfile *p = g_ys_profile.reward;
    uint16_t pc = pce->cpu.pc;
    if (!g_pc_range)
        pc_range(p);
    if (pc < g_pc_min || pc > g_pc_max)
        return -1;
    for (int h = 0; h < p->hook_count; h++) {
        const YsRewardHook *r = &p->hooks[h];
        if (r->pc != pc || pce->cpu.mpr[3] != r->bank)
            continue;
        for (int k = 0; k < r->n; k++)
            if (!(r->skip >> k & 1) && read_logical(pce, (uint16_t)(pc + k)) != r->sig[k])
                return -1;
        return r->kind;
    }
    return -1;
}

void ys_reward_init(YsReward *r)
{
    memset(r, 0, sizeof(*r));
}

void ys_reward_hook(YsReward *r, Pce *pce)
{
    int h = which_hook(pce);
    if (h < 0)
        return;
    const YsRewardProfile *p = g_ys_profile.reward;
    if (h == YS_REWARD_KILL) {
        uint8_t w = RAM(pce, p->walker);
        r->open = 1;
        r->kills++;
        memset(&r->cur, 0, sizeof(r->cur));
        r->cur.walker = w;
        r->cur.actor = w < YS_WALKERS ? RAM(pce, p->actor + w) : 0xFF;
        return;
    }
    if (!r->open) { /* event reward, paid by the area program (not an enemy death) */
        r->events++;
        return;
    }
    if (h == YS_REWARD_GOLD) {
        uint32_t now = (uint32_t)(pce->cpu.y | pce->cpu.a << 8);
        uint32_t old = (uint32_t)(RAM(pce, p->gold) | RAM(pce, p->gold + 1) << 8);
        r->cur.gold = now > old ? now - old : 0;
        return;
    }
    uint32_t now = (uint32_t)(pce->cpu.y | pce->cpu.x << 8 | pce->cpu.a << 16);
    uint32_t old = (uint32_t)(RAM(pce, p->exp) | RAM(pce, p->exp + 1) << 8 | RAM(pce, p->exp + 2) << 16);
    r->cur.exp = now > old ? now - old : 0;
    r->open = 0;
    r->paid++;
    if (r->queue.n < YS_REWARD_MAX)
        r->queue.ev[r->queue.n++] = r->cur;
    else
        r->lost++;
}

void ys_reward_take(YsReward *r, YsRewardFrame *out)
{
    for (int i = 0; i < r->queue.n; i++) {
        if (out->n < YS_REWARD_MAX)
            out->ev[out->n++] = r->queue.ev[i];
        else
            r->lost++;
    }
    r->queue.n = 0;
}
