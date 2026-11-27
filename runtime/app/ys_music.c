/* New music started by the game: see ys_music.h */
#include "app/ys_music.h"

#include "cd/cdrom.h"
#include "profile/ys_profile.h"

void ys_music_hook(Pce *pce)
{
    const YsMusicProfile *m = g_ys_profile.music;
    if (pce->cpu.pc != m->fade_pc || pce->cpu.mpr[2] != m->bank || !pce->cd)
        return;
    for (int k = 0; k < (int)sizeof(m->sig); k++)
        if (pce_bus_read(pce, (uint32_t)m->bank * 0x2000 + ((m->pc + k) & 0x1FFF)) != m->sig[k])
            return;
    /* with no fade active the volume does not change: the old track keeps going as it was until the command */
    if (cdrom_fader_enabled(pce->cd, 0) && pce->cd->audio.state == CD_AUDIO_PLAYING) {
        pce->cd->audio.pc_mute = 1;
        pce->cd->audio.pc_mutes++;
    }
}
