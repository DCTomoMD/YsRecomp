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

/* docs/ys_notes.md, "The game's window system": routines of bank $81 of the CD RAM in the $6000-$7FFF window */
static const YsWindowsProfile k_windows = {
    .bank = 0x81,
    .hooks = {
        /* JSR $6148 (sound) / JSR $6061 (border) */
        [YS_WIN_HOOK_OPENING] = { 0x6031, { 0x20, 0x48, 0x61, 0x20, 0x61 }, 0 },
        /* (border done) JSR (masks) / JSR $617F; the masks' address: $80:4E2F in Ys I, $80:4F78 in Ys II */
        [YS_WIN_HOOK_DRAWN] = { 0x6037, { 0x20, 0x78, 0x4F, 0x20, 0x7F }, 6 },
        /* INC $3E / RTS */
        [YS_WIN_HOOK_OPEN] = { 0x6040, { 0xE6, 0x3E, 0x60, 0xA5, 0x3E }, 0 },
        /* JSR $61DE / LDA $43 */
        [YS_WIN_HOOK_CLOSING] = { 0x604D, { 0x20, 0xDE, 0x61, 0xA5, 0x43 }, 0 },
        /* (background restored by $60F5) JSR $6143 / JSR $6157 */
        [YS_WIN_HOOK_RESTORED] = { 0x6057, { 0x20, 0x43, 0x61, 0x20, 0x57 }, 0 },
        /* JSR (masks) / RTS; also called without closing ($605A) */
        [YS_WIN_HOOK_CLOSED] = { 0x605D, { 0x20, 0x78, 0x4F, 0x60, 0xA5 }, 6 },
    },
    /* docs/ys_notes.md, "Large menu screens": $82:7AF6 enters the single scroll, $82:7B2F goes back; the hooks are on
     * the instructions that pick the interrupt line ($2210 = $68 / $148) */
    .layout_bank = 0x82,
    .one_scroll_pc = { 0x7AFC, 0x7AAB },
    .one_scroll_sig = { 0x17, 0xF5, 0xA9, 0x68, 0x8D, 0x10, 0x22, 0xA9, 0x00, 0x8D, 0x11, 0x22 },
    .split_pc = { 0x7B32, 0x7AE1 },
    .split_sig = { 0xA9, 0x48, 0x8D, 0x10, 0x22, 0xA9, 0x01, 0x8D, 0x11, 0x22, 0x20, 0xB1, 0x53 },
    .count = 0x203E,
    .rect = 0x203F,
    .camera_x = 0x2C37,
    .camera_y = 0x2C39,
    .stack_len = 0x33C6,
    .stack = 0x33C7,
};

/* docs/ys_notes.md, "Intro" (app/ys_intro.h): bank $80 in window 2 */
static const YsIntroProfile k_intro = {
    .bank = 0x80,
    .script_pc = 0x5AA4,
    .script_sig = { 0xB2, 0x62, 0xC9, 0xFF }, /* LDA ($62); CMP #$FF */
    .menu_pc = 0x494C,
    .menu_sig = { 0xA9, 0x08, 0x8D, 0xBF, 0x2E }, /* LDA #$08; STA $2EBF (only RUN counts) */
    .screens = {
        [YS_INTRO_OP_NEC] = { 0x4C4A, { 0x18, 0x06, 0x00, 0x00, 0x05, 0x00, 0x00, 0x02, 0x00 } },
        [YS_INTRO_OP_LOGO] = { 0x4C6D, { 0x18, 0x06, 0x00, 0x00, 0x05, 0x00, 0x00, 0x01, 0x00 } },
        [YS_INTRO_OP_COPYRIGHT] = { 0x4D3D, { 0x18, 0x06, 0x00, 0x00, 0x05, 0x00, 0x00, 0xFF, 0x00 } },
    },
    .script = 0x2062,
    .lock = 0x203A,
    .flags = 0x2024,
    .hurry = 1,
    .mask = 0x2EBF,
};

/* app/ys_dialog.h: LDA $27F6,X; CLC; ADC $2826,X; STA $2826,X; LDA $27F6,X; BEQ +$1D; LDA $2826,X; SEC; SBC $92;
 * BCS +$12; JSR $402A */
static const YsDialogProfile k_dialog = {
    .bank = 0x80,
    .test_pc = { 0x58D6, 0x5BD0, 0x5BD5 },
    .sig = { 0xBD, 0xF6, 0x27, 0x18, 0x7D, 0x26, 0x28, 0x9D, 0x26, 0x28, 0xBD, 0xF6, 0x27,
             0xF0, 0x1D, 0xBD, 0x26, 0x28, 0x38, 0xE5, 0x92, 0xB0, 0x12, 0x20, 0x2A, 0x40 },
    .sig_test = 10, /* the LDA $27F6,X before the BEQ */
    .sig_wait = 23, /* the JSR $402A */
    .speed = 0x27F6,
};

/* docs/ys_notes.md, "Game fades" */
static const YsFadeProfile k_fade = {
    /* STX zp_lo; STA zp_lo+1; TXA; AND #7; ASL; ASL; ASL; ORA level; TAX; LDA table,X; then, in the programs' one:
     * STA $88 (0x85); in the scenes' one: ADC brightening (0x65 zp) */
    .transform = { 0x86, 0xFFFF, 0x85, 0xFFFF, 0x8A, 0x29, 0x07, 0x0A, 0x0A, 0x0A,
                   0x05, 0xFFFF, 0xAA, 0xBD, 0xFFFF, 0xFFFF },
    .level_at = 11,
    .table_at = 14,
    .family_at = 16,
    .scenes_op = 0x65,
    /* the caller: (LDA zp;) DEY; STA (dest),Y */
    .caller_skip_op = 0xA5,
    .caller_op = { 0x88, 0x91 },
    /* LDA (zp); TAX; AND #7; TAY; LDA table,Y; STA $00; TXA; AND #$38; CMP #$38; BEQ +2; ADC #8 */
    .flash = { 0xB2, 0xFFFF, 0xAA, 0x29, 0x07, 0xA8, 0xB9, 0xFFFF, 0xFFFF, 0x85, 0x00,
               0x8A, 0x29, 0x38, 0xC9, 0x38, 0xF0, 0x02, 0x69, 0x08 },
    .flash_zp_at = 1,
    .colors = 0x3C00,
};

const YsProfile g_ys_profile = {
    .identity = &k_identity,
    .disc = &k_disc,
    .music = &k_music,
    .screen = &k_screen,
    .scroll = &k_scroll,
    .reward = &k_reward,
    .windows = &k_windows,
    .intro = &k_intro,
    .dialog = &k_dialog,
    .fade = &k_fade,
};
