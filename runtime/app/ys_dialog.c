/* Instant dialog: see ys_dialog.h */
#include "app/ys_dialog.h"

#include "cd/cdrom.h"
#include "profile/ys_profile.h"

/* The speed test and its signature (from the accumulator's LDA of the speed to the wait for the next frame):
 * g_ys_profile.dialog */

/* test = address of the speed test in one of the three disc programs that have the text program (the profile's bank
 * in window 2), checked against the bytes */
static int sig_ok(const Pce *pce, uint16_t test)
{
    const YsDialogProfile *p = g_ys_profile.dialog;
    if ((test != p->test_pc[0] && test != p->test_pc[1] && test != p->test_pc[2]) || pce->cpu.mpr[2] != p->bank ||
        !pce->cd)
        return 0;
    for (int k = 0; k < (int)sizeof(p->sig); k++)
        if (pce->cd->ram[(p->bank - 0x80) * 0x2000 + ((test - p->sig_test + k) & 0x1FFF)] != p->sig[k])
            return 0;
    return 1;
}

int ys_dialog_at(const Pce *pce)
{
    return sig_ok(pce, pce->cpu.pc);
}

int ys_dialog_wait_at(const Pce *pce)
{
    const YsDialogProfile *p = g_ys_profile.dialog;
    return sig_ok(pce, (uint16_t)(pce->cpu.pc - (p->sig_wait - p->sig_test)));
}

void ys_dialog_patch(Pce *pce)
{
    if (!ys_dialog_at(pce))
        return;
    uint32_t addr = 0xF8u * 0x2000 + ((g_ys_profile.dialog->speed + pce->cpu.x) & 0x1FFF);
    if (pce_bus_read(pce, addr) != 0)
        pce_bus_write(pce, addr, 0);
}

void ys_dialog_set(Pce *pce, int on)
{
    pce->game_patch = on ? ys_dialog_patch : NULL;
}
