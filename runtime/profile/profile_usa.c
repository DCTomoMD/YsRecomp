/*
 * Profile of Ys Book I & II (USA): the values moved from runtime/app and runtime/main.c, unchanged
 * (review_jp/05_phase3a_plan.md; checked with tools/profile_values.py).
 */
#include "profile/ys_profile.h"

static const YsIdentity k_identity = {
    .edition_id = "usa",
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

/* The C BIOS by default; --bios gives the real one, unchecked (as before Phase 3B) */
static const YsBiosPolicy k_bios = {
    .required = 0,
    .check_crc = 0,
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
};

/* docs/ys_notes.md, "Engine memory map" */
static const YsCommonRam k_common = {
    .walker_actor = 0x2736,
    .exp = 0x2ABB,
    .gold = 0x2C41,
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

/* docs/ys_notes.md, "Engine memory map": routines of bank $80 of the CD RAM in the $4000-$5FFF window */
static const YsEngineProfile k_engine = {
    .bank = 0x80,
    .hooks = {
        /* start of the build: LDA #1 / EOR $2215 */
        [YS_ENGINE_HOOK_START] = { 0x4D58, { 0xA9, 0x01, 0x4D, 0x15, 0x22 } },
        /* a character (X): LDA $2F38,X / BIT #8 */
        [YS_ENGINE_HOOK_ACTOR] = { 0x4DAC, { 0xBD, 0x38, 0x2F, 0x89, 0x08 } },
        /* a piece (line already written): STY $0002 / STA $0003 */
        [YS_ENGINE_HOOK_PIECE] = { 0x4E60, { 0x8C, 0x02, 0x00, 0x8D, 0x03 } },
        /* end (after ex_sprdma): CLI / RTS */
        [YS_ENGINE_HOOK_END] = { 0x4DAA, { 0x58, 0x60, 0xBD, 0x38, 0x2F } },
    },
    /* "Ys I mine light": $86:7108 (window $6000) LDX $7673 / LDA $2F78,Y / STA $2F78,X ... */
    .copy_bank = 0x86,
    .copy_pc = 0x7108,
    .copy_sig = { 0xAE, 0x73, 0x76, 0xB9, 0x78, 0x2F },
    .copy_light = 0x7673,
    /* $75A5 in Ys I, $75A7 in Ys II (window $6000), with the new number in A: STA $76 / TAY / CLA / STA $2EF8,Y */
    .hero_pc = { 0x75A5, 0x75A7 },
    .hero_sig = { 0x85, 0x76, 0xA8, 0x62, 0x99, 0xF8, 0x2E },
    .actor_x_lo = 0x2F78,
    .actor_x_hi = 0x2FB8,
    .actor_y_lo = 0x2FF8,
    .actor_y_hi = 0x3038,
    .actor_flags = 0x2F38,
    .actor_pose = 0x2EF8,
    .actor_state = 0x2EB6,
    .camera_x = 0x2C33,
    .camera_y = 0x2C35,
    .walker_vel = 0x2A66,
    .walker_acc = 0x2ADF,
    .walker_left = 0x2B3F,
    .walker_mask = 0x2B9F,
    .walker_next = 0x2BCF,
    .walker_hp = 0x2A36,
    .walker_hpmax = 0x28E6,
    .unit = 0x2092,
    .front_base = 0x205F,
    .front_count = 0x205D,
    .front_attr = 0x2052,
    .front_attr_bank = 0x2C52,
    .front_bits = 0x2C55,
    .front_bits_bank = 0x2C57,
};

/* docs/ys_notes.md, "Panel: the cells the game writes" (app/ys_panel.h); the same tiles in Ys I and Ys II */
static const YsPanelProfile k_panel = {
    .next_level = 0x2C4A,
    .hp_max = 0x2C48,
    .tile_blank = 256,
    .tile_label_first = 257,
    .tile_label_last = 266,
    .tile_h = 257,
    .tile_e = 260,
    .tile_g = 262,
    .tile_m = 266,
    .tile_digit_first = 449,
    .tile_digit_last = 460,
    .tile_slash = 460,
    .tile_bar_side = 282,
    .tile_bar_yellow = 506,
    .tile_bar_red = 507,
};

/* docs/ys_notes.md, "Equipment on Adol" */
static const YsEquipProfile k_equip = {
    .items_bank = 0x82,
    .items = 0x6321,
    .ys1_items = { 0, 5, 10, 15 },
    .ys2_items = { 0, 6, 12, 18 },
    .slots = 0x2C71,
    .adol_pal = 13,
    .ice_pal = 14,
};

/* app/menu_font.h: tiles 449-487 (0-9, ':', '/', A-C, a solid tile, D-Z) and 504 */
static const YsMenuFontProfile k_menu_font = {
    .first = 449,
    .arrow = 504,
    .crc = 0xAADA7A4Cu,
};

/* app/launcher_sound.c: frames measured without the BIOS with pce_trace --input; the tables reach the sound driver
 * at about frame 3705 */
static const YsLauncherSfxProfile k_launcher_sfx = {
    .numbers = { 0x13, 0x12, 0x17 },
    .fixed = { { 1644, 1650 }, { 1965, 1971 }, { 2487, 2496 } },
    .first_pulse = 2560,
    .pulse_every = 40,
    .pulse_len = 6,
    .max_frames = 7000,
    .settle = 150,
    .sfx_flags = 0x20E7,
    .sfx_play_mask = 0x7F,
};

/* app/intro_skip.h: the intro reaches the menu at about frame 2400 */
static const YsIntroSkipProfile k_intro_skip = {
    .max_frames = 4000,
};

/* docs/ys_notes.md, "Engine memory map" (tools/trace/ys_actors.c) and "Panel" (ys_trace_ext.c, --panel-log) */
static const YsTraceProfile k_trace = {
    .hooks = {
        /* end of the wait for the next frame: LDA $2241 */
        [YS_TRACE_HOOK_WAIT] = { 0x4B1B, { 0xAD, 0x41, 0x22, 0x8D, 0x12 } },
        /* VBlank: copies the camera of the finished list to the scroll */
        [YS_TRACE_HOOK_VBLANK] = { 0x4AA3, { 0x1F, 0x80, 0x14, 0x17, 0x80 } },
    },
    .first_actor = 0x2E4A,
    .piece = 0x2010,
    .vbl_flags = 0x2080,
    .vbl_copy = 0x02,
    .hp = 0x2C43,
    .label_chars = "H.PEXGOLDM", /* tiles 257-266 */
    .digit_chars = "0123456789:/", /* tiles 449-460 */
};

const YsProfile g_ys_profile = {
    .identity = &k_identity,
    .disc = &k_disc,
    .bios = &k_bios,
    .music = &k_music,
    .screen = &k_screen,
    .scroll = &k_scroll,
    .reward = &k_reward,
    .windows = &k_windows,
    .intro = &k_intro,
    .dialog = &k_dialog,
    .fade = &k_fade,
    .common = &k_common,
    .engine = &k_engine,
    .panel = &k_panel,
    .equip = &k_equip,
    .menu_font = &k_menu_font,
    .launcher_sfx = &k_launcher_sfx,
    .intro_skip = &k_intro_skip,
    .trace = &k_trace,
};
