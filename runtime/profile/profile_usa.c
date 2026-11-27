/*
 * Profile of Ys Book I & II (USA): the values moved from runtime/app and runtime/main.c, unchanged
 * (review_jp/05_phase3a_plan.md; checked with tools/profile_values.py).
 */
#include "profile/ys_profile.h"

static const YsIdentity k_identity = {
    .app_id = "YsRecomp",
    .ini_name = "YsRecomp.ini",
    .window_title = "Ys Book I & II",
    .disc_name = "Ys Book I & II (USA)",
};

/* Redump format (.cue + 43 .bin), measured on the user's disc with `app_tests --fingerprint <cue>`. */
static const YsDiscIdentity k_disc = {
    .track_count = 43,
    .toc_crc = 0x34B2F40Eu,
    .data_crc = 0xEC4E1070u,
    .title = "Ancient Ys Vanished",
    .title_sector = 1,
    .game_name = "Ys Book I & II",
    .version_name = "Ys Book I & II, USA",
};

#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))

/* docs/ys_notes.md, "CD music": $80:4244, the same in Ys I and Ys II */
static const YsMusicProfile k_music = {
    .bank = 0x80,
    .pc = 0x4244,
    .fade_pc = 0x4247,
    .sig = { 0xDA, 0x48, 0x62, 0x20, 0x2D, 0xE0 }, /* PHX; PHA; CLA; JSR $E02D */
};

/* docs/ys_notes.md, "Border and panel on screen" */
static const YsScreenProfile k_screen = {
    .bank = 0x80,
    .loader_pc = 0x40E7,
    .loader_sig = { 0xDA, 0x20, 0x65, 0x41 }, /* PHX; JSR $4165 */
    .boot_pc = 0x40DC,
    .boot_sig = { 0x20, 0x00, 0x48 }, /* JSR $4800 */
    .intro_program = 0x04,
    .game1_program = 0x08,
    .game2_program = 0x30,
    .load_program = 0x01,
};

/* docs/ys_notes.md, "Mid-frame scroll changes" */
static const YsScrollProfile k_scroll = {
    .bank = 0x80,
    .hooks = {
        [YS_SCROLL_HOOK_MAP] = { 0x4AE1, { 0x20, 0x5B, 0x4A } },
        [YS_SCROLL_HOOK_WRAP] = { 0x4B05, { 0x20, 0x88, 0x4A } },
        [YS_SCROLL_HOOK_LOW] = { 0x4AD8, { 0x20, 0x75, 0x4A } },
    },
    .map_x = 0x2C2F,
    .map_line = 0x2C31,
    .cr = 0x20F3,
    .wrap_flags = 0x2082,
    .wrap_x_zero = 0x40,
};

/* docs/ys_notes.md, "Enemies: HP, EXP and GOLD" */
static const YsRewardHook k_reward_hooks[] = {
    /* LDA #$17 / JSR $5E06|$5E0B (sound) / LDX $22 / LDA #$02 / STA $2826,X */
    { 0x7959, 0x81, YS_REWARD_KILL, 12,
      { 0xA9, 0x17, 0x20, 0x06, 0x5E, 0xA6, 0x22, 0xA9, 0x02, 0x9D, 0x26, 0x28 }, 1 << 3 },
    { 0x7998, 0x81, YS_REWARD_KILL, 12,
      { 0xA9, 0x17, 0x20, 0x0B, 0x5E, 0xA6, 0x22, 0xA9, 0x02, 0x9D, 0x26, 0x28 }, 1 << 3 },
    /* STY $2C41 / STA $2C42 / RTS */
    { 0x7097, 0x82, YS_REWARD_GOLD, 7, { 0x8C, 0x41, 0x2C, 0x8D, 0x42, 0x2C, 0x60 }, 0 },
    { 0x7160, 0x82, YS_REWARD_GOLD, 7, { 0x8C, 0x41, 0x2C, 0x8D, 0x42, 0x2C, 0x60 }, 0 },
    /* STY $2ABB / STX $2ABC / STA $2ABD / RTS */
    { 0x707C, 0x82, YS_REWARD_EXP, 10, { 0x8C, 0xBB, 0x2A, 0x8E, 0xBC, 0x2A, 0x8D, 0xBD, 0x2A, 0x60 }, 0 },
    { 0x7145, 0x82, YS_REWARD_EXP, 10, { 0x8C, 0xBB, 0x2A, 0x8E, 0xBC, 0x2A, 0x8D, 0xBD, 0x2A, 0x60 }, 0 },
};
static const YsRewardProfile k_reward = {
    .hooks = k_reward_hooks,
    .hook_count = COUNT(k_reward_hooks),
    .walker = 0x2022,
    .actor = 0x2736,
    .gold = 0x2C41,
    .exp = 0x2ABB,
};

const YsProfile g_ys_profile = {
    .identity = &k_identity,
    .disc = &k_disc,
    .music = &k_music,
    .screen = &k_screen,
    .scroll = &k_scroll,
    .reward = &k_reward,
};
