/*
 * Profile of one edition of the game (review_jp/01_profile_design.md): the values that come from the disc and the
 * texts that name it, as plain data. Each executable links exactly one profile_<edition>.c, which defines
 * g_ys_profile; the common code reads it and never asks which edition it is.
 *
 * Types and constants only: plain C, no OS headers, no SDL, nothing from app/ or host/ (tools/check_layers.py).
 */
#ifndef YS_PROFILE_H
#define YS_PROFILE_H

#include <stdint.h>

/* Names the player sees and the files of this edition. */
typedef struct YsIdentity {
    const char *app_id;       /* folder of the user's data (host_paths_init): ASCII, safe as a folder name */
    const char *ini_name;     /* settings file in that folder */
    const char *window_title; /* window and message boxes */
    const char *disc_name;    /* the disc as the player knows it (folder dialog and messages) */
} YsIdentity;

/* What identifies the supported copy of the disc (app/disc_id.c). Numbers about the disc, not its content. */
typedef struct YsDiscIdentity {
    int track_count;          /* tracks in the .cue (one .bin each) */
    uint32_t toc_crc;         /* CRC32 of (type, start, sectors) of each track */
    uint32_t data_crc;        /* CRC32 of the user data of the 1st data track */
    const char *title;        /* text in the data track that tells this game apart from other discs */
    int title_sector;         /* sector of the data track where it is, counted from the track's first sector */
    const char *game_name;    /* the game, in the messages about a wrong disc */
    const char *version_name; /* the supported version, in the message about different data */
} YsDiscIdentity;

/*
 * Instruction hooks (stage 3A-2). Each table keeps the shape it had in its module (review_jp/05, Q6). A "bank" is the
 * physical bank the hook expects in the MPR window of its PC; "pc" is the CPU address; a "sig" is the code bytes that
 * must be there; RAM addresses are CPU addresses ($2000-$3FFF, Q5).
 */

/* app/ys_music.c: the resident routine that plays a song */
typedef struct YsMusicProfile {
    uint8_t bank;     /* in window 2 */
    uint16_t pc;      /* the routine: sig is read from here */
    uint16_t fade_pc; /* its JSR to cd_fade: the hook */
    uint8_t sig[6];
} YsMusicProfile;

/* app/ys_screen.c: the disc program running and its screen layout */
typedef struct YsScreenProfile {
    uint8_t bank; /* resident file, in window 2 */
    uint16_t loader_pc; /* program loader (the number of the next program in A) */
    uint8_t loader_sig[4];
    uint16_t boot_pc;   /* call of the 1st program, without the loader */
    uint8_t boot_sig[3];
    uint8_t intro_program;        /* the intro and the main menu */
    uint8_t game1_program;        /* gameplay, Ys I: map with the frame and the panel */
    uint8_t game2_program;        /* gameplay, Ys II: the same */
    uint8_t load_program;         /* load game menu: the frame, no panel */
} YsScreenProfile;

/* app/ys_scroll.c: the game's mid-image scroll changes (order of hooks[] = the YS_SCROLL_HOOK_* values) */
enum { YS_SCROLL_HOOK_MAP, YS_SCROLL_HOOK_WRAP, YS_SCROLL_HOOK_LOW, YS_SCROLL_HOOKS };
typedef struct YsScrollHook {
    uint16_t pc;
    uint8_t sig[3]; /* JSR of the change */
} YsScrollHook;
typedef struct YsScrollProfile {
    uint8_t bank; /* in window 2 */
    YsScrollHook hooks[YS_SCROLL_HOOKS];
    uint16_t map_x;        /* BXR of the map (16 bits, low byte first) */
    uint16_t map_line;     /* 1st line of the map in the ring */
    uint16_t cr;           /* copy of the VDC's CR */
    uint16_t wrap_flags;   /* flags read at the ring wrap */
    uint8_t wrap_x_zero;   /* bit of wrap_flags: BXR after the wrap is 0 */
} YsScrollProfile;

/* app/ys_reward.c: EXP and GOLD paid when an enemy dies */
enum { YS_REWARD_KILL, YS_REWARD_GOLD, YS_REWARD_EXP };
typedef struct YsRewardHook {
    uint16_t pc;
    uint8_t bank; /* in window 3 */
    uint8_t kind; /* YS_REWARD_* */
    uint8_t n;    /* bytes of sig */
    uint8_t sig[12];
    uint16_t skip; /* bits of the bytes that do not count */
} YsRewardHook;
typedef struct YsRewardProfile {
    const YsRewardHook *hooks;
    int hook_count;
    uint16_t walker; /* slot of the walker hit (its character, GOLD and EXP: YsCommonRam) */
} YsRewardProfile;

/* app/ys_windows.c: the game's window system (order of hooks[] = the YS_WIN_HOOK_* values) and the screen layout
 * switch of the large menus */
enum { YS_WIN_HOOK_OPENING, YS_WIN_HOOK_DRAWN, YS_WIN_HOOK_OPEN, YS_WIN_HOOK_CLOSING, YS_WIN_HOOK_RESTORED,
       YS_WIN_HOOK_CLOSED, YS_WIN_HOOKS };
typedef struct YsWindowsHook {
    uint16_t pc;
    uint8_t sig[5];
    uint8_t skip; /* bits of the bytes that do not count */
} YsWindowsHook;
typedef struct YsWindowsProfile {
    uint8_t bank; /* window system, in window 3 */
    YsWindowsHook hooks[YS_WIN_HOOKS];
    uint8_t layout_bank;        /* layout switch, in window 3 */
    uint16_t one_scroll_pc[2];  /* Ys I, Ys II: enters the single scroll */
    uint8_t one_scroll_sig[12];
    uint16_t split_pc[2];       /* Ys I, Ys II: back to the split */
    uint8_t split_sig[13];
    uint16_t count;      /* windows open (the game's counter) */
    uint16_t rect;       /* the current window: column, row, width, height, page (5 bytes) */
    uint16_t camera_x;   /* logic camera (16 bits each, low byte first) */
    uint16_t camera_y;
    uint16_t stack_len;  /* bytes on the game's window stack */
    uint16_t stack;      /* the stack: 5 bytes (as rect) for each window */
} YsWindowsProfile;

/* app/ys_intro.c: the intro program's script and main menu (order of screens[] = the YS_INTRO_OP_* values) */
enum { YS_INTRO_OP_NEC, YS_INTRO_OP_LOGO, YS_INTRO_OP_COPYRIGHT, YS_INTRO_OPS };
typedef struct YsIntroOp {
    uint16_t addr; /* the script operation that starts the screen (the fade that lights it up) */
    uint8_t op[9];
} YsIntroOp;
typedef struct YsIntroProfile {
    uint8_t bank;        /* in window 2 */
    uint16_t script_pc;  /* the script loop */
    uint8_t script_sig[4];
    uint16_t menu_pc;    /* the main menu routine */
    uint8_t menu_sig[5];
    YsIntroOp screens[YS_INTRO_OPS];
    uint16_t script;     /* pointer to the next operation (16 bits, low byte first) */
    uint16_t lock;       /* frames without accepting buttons */
    uint16_t flags;      /* its bit hurry: button accepted */
    uint8_t hurry;
    uint16_t mask;       /* buttons that count */
} YsIntroProfile;

/* app/ys_dialog.c: the text program's letter speed */
typedef struct YsDialogProfile {
    uint8_t bank;        /* in window 2 */
    uint16_t test_pc[3]; /* the speed test in each of the three disc programs that have the text program */
    uint8_t sig[26];     /* from the speed's LDA to the wait for the next frame */
    uint8_t sig_test;    /* byte of sig at test_pc */
    uint8_t sig_wait;    /* byte of sig of the wait (the text waits for the next frame for the next letter) */
    uint16_t speed;      /* speed of the text task, index X */
} YsDialogProfile;

/* app/ys_fade.c: the game's fade routines, found by their code in any bank of the CD RAM (0xFFFF = any byte) */
typedef struct YsFadeProfile {
    uint16_t transform[16]; /* start of the fade routine (both families) */
    uint8_t level_at;       /* bytes of transform: the zero-page level, */
    uint8_t table_at;       /* the table's address (16 bits), */
    uint8_t family_at;      /* the byte after transform that tells the families apart */
    uint8_t scenes_op;      /* its value in the scenes' family (ADC zp; the brightening's zero page follows) */
    uint8_t caller_skip_op; /* after the caller's JSR: optional instruction (2 bytes), */
    uint8_t caller_op[2];   /* then these two; the zero page of the destination follows */
    uint16_t flash[20];     /* flash loop (end-of-Ys-I area) */
    uint8_t flash_zp_at;    /* byte of flash: the zero page of the color */
    uint16_t colors;        /* shown colors in RAM: 256 background, then 256 sprites (2 bytes each) */
} YsFadeProfile;

/* Game RAM read by more than one module (app/ys_engine.c, app/ys_reward.c) */
typedef struct YsCommonRam {
    uint16_t walker_actor; /* character of each walker (indexed by the walker) */
    uint16_t exp;          /* the player's EXP (24 bits, low byte first) */
    uint16_t gold;         /* the player's GOLD (16 bits, low byte first) */
} YsCommonRam;

/* app/ys_engine.c: the sprite list the engine builds (order of hooks[] = the YS_ENGINE_HOOK_* values) */
enum { YS_ENGINE_HOOK_START, YS_ENGINE_HOOK_ACTOR, YS_ENGINE_HOOK_PIECE, YS_ENGINE_HOOK_END, YS_ENGINE_HOOKS };
typedef struct YsEngineHook {
    uint16_t pc;
    uint8_t sig[5];
} YsEngineHook;
typedef struct YsEngineProfile {
    uint8_t bank; /* engine routines, in window 2 */
    YsEngineHook hooks[YS_ENGINE_HOOKS];
    /* Ys I mine light: the copy of a character's position to the light's */
    uint8_t copy_bank;  /* in window 3 */
    uint16_t copy_pc;
    uint8_t copy_sig[6];
    uint16_t copy_light; /* (logical address) the light's character */
    /* Adol is born: the routine that creates the hero, in the gameplay programs of Ys I and Ys II */
    uint16_t hero_pc[2];
    uint8_t hero_sig[7];
    /* each character (indexed by X) */
    uint16_t actor_x_lo, actor_x_hi, actor_y_lo, actor_y_hi; /* position on screen */
    uint16_t actor_flags, actor_pose, actor_state;
    /* the list's camera (16 bits each, low byte first) */
    uint16_t camera_x, camera_y;
    /* each walker (indexed by the walker; the character: YsCommonRam.walker_actor) */
    uint16_t walker_vel, walker_acc, walker_left, walker_mask, walker_next, walker_hp, walker_hpmax;
    uint16_t unit; /* the walkers' unit */
    /* the map's front cells (16 bits each, low byte first; banks: 1 byte) */
    uint16_t front_base, front_count, front_attr, front_attr_bank, front_bits, front_bits_bank;
} YsEngineProfile;

/* app/ys_panel.c, .h: the panel's RAM and the fixed tiles the game writes in it (docs/ys_notes.md, "Panel") */
typedef struct YsPanelProfile {
    uint16_t next_level; /* EXP of the next level (24 bits, low byte first) */
    uint16_t hp_max;     /* max H.P */
    uint16_t tile_blank;
    uint16_t tile_label_first, tile_label_last; /* label letters */
    uint16_t tile_h, tile_e, tile_g, tile_m;    /* 1st letter of each label: H.P, E(XP), G(OLD), M.P */
    uint16_t tile_digit_first, tile_digit_last; /* 0-9, ':' and '/' */
    uint16_t tile_slash;
    uint16_t tile_bar_side;
    uint16_t tile_bar_yellow; /* 8 HP points left */
    uint16_t tile_bar_red;    /* 8 HP points lost */
} YsPanelProfile;

/* app/equip.c: what is equipped and Adol's palettes */
typedef struct YsEquipProfile {
    uint8_t items_bank;     /* CD RAM bank of the table of items */
    uint16_t items;         /* (logical address) the 1st item of each type */
    uint8_t ys1_items[4];   /* its values in Ys I */
    uint8_t ys2_items[4];   /* in Ys II */
    uint16_t slots;         /* the item equipped: sword, shield, armor (3 bytes) */
    uint8_t adol_pal;       /* Adol's sprite palette */
    uint8_t ice_pal;        /* Adol's sprite palette on ice */
} YsEquipProfile;

/* app/menu_font.c: the game's 8x8 font in VRAM */
typedef struct YsMenuFontProfile {
    uint16_t first; /* 1st tile of the font */
    uint16_t arrow; /* the game menu's little arrow */
    uint32_t crc;   /* CRC32 of the copied words (little-endian) */
} YsMenuFontProfile;

/* app/launcher_sound.c: the effects of the launcher, taken from the disc with a hidden console script */
typedef struct YsLauncherSfxProfile {
    uint8_t numbers[3];   /* cursor, confirm, "not released" (LAUNCHER_SFX_*) */
    int32_t fixed[3][2];  /* RUN pressed in these frames [from, to): copyright, goddess, main menu */
    int32_t first_pulse;  /* from here on, RUN pressed for pulse_len frames every pulse_every */
    int32_t pulse_every;
    int32_t pulse_len;
    int32_t max_frames;   /* the game must give the tables to the sound driver before this */
    int32_t settle;       /* frames after the tables: the game music starts, the effects stop */
    uint16_t sfx_flags;   /* the sound driver's flags: sfx_play_mask is ANDed in to play an effect */
    uint8_t sfx_play_mask;
} YsLauncherSfxProfile;

/* app/intro_skip.c: the intro reaches the main menu well before this frame (otherwise: not this disc) */
typedef struct YsIntroSkipProfile {
    int32_t max_frames;
} YsIntroSkipProfile;

/* tools/trace (ys_actors.c, ys_trace_ext.c): what only the trace tools read; the rest they share with the modules above
 * (order of hooks[] = the YS_TRACE_HOOK_* values, after the engine's own hooks) */
enum { YS_TRACE_HOOK_WAIT, YS_TRACE_HOOK_VBLANK, YS_TRACE_HOOKS };
typedef struct YsTraceProfile {
    YsEngineHook hooks[YS_TRACE_HOOKS]; /* engine routines, in the engine's bank and window */
    uint16_t first_actor; /* 1st character of the build */
    uint16_t piece;       /* pointer to the piece being written (16 bits, low byte first) */
    uint16_t vbl_flags;   /* its bit vbl_copy: the game's VBlank copied the camera */
    uint8_t vbl_copy;
    uint16_t hp;          /* the player's H.P (16 bits, low byte first) */
    const char *label_chars; /* the panel's label tiles from YsPanelProfile.tile_label_first, as text */
    const char *digit_chars; /* its digit tiles from tile_digit_first, as text */
} YsTraceProfile;

typedef struct YsProfile {
    const YsIdentity *identity;
    const YsDiscIdentity *disc;
    const YsMusicProfile *music;
    const YsScreenProfile *screen;
    const YsScrollProfile *scroll;
    const YsRewardProfile *reward;
    const YsWindowsProfile *windows;
    const YsIntroProfile *intro;
    const YsDialogProfile *dialog;
    const YsFadeProfile *fade;
    const YsCommonRam *common;
    const YsEngineProfile *engine;
    const YsPanelProfile *panel;
    const YsEquipProfile *equip;
    const YsMenuFontProfile *menu_font;
    const YsLauncherSfxProfile *launcher_sfx;
    const YsIntroSkipProfile *intro_skip;
    const YsTraceProfile *trace;
} YsProfile;

extern const YsProfile g_ys_profile;

/*
 * Constants (stage 3A-5, review_jp/05 Q2): the geometry of the game's image that many files use and the sizes of
 * arrays (and of a cache file). They stay compile-time constants, gathered here; the values are the USA edition's.
 * [A] the same in both editions: not yet checked on the Japanese disc (review_jp/01 U12). If it differs, these move
 * into the profile structures then.
 */
/* The game image (the border, the map and the panel), in console image pixels */
#define YS_IMG_FRAME_LEFT 8      /* border outline columns */
#define YS_IMG_FRAME_RIGHT 327
#define YS_IMG_FRAME_TOP 11      /* outline lines */
#define YS_IMG_FRAME_MAP_TOP 25
#define YS_IMG_FRAME_MAP_BOTTOM 156
#define YS_IMG_FRAME_BOTTOM 170
#define YS_IMG_MAP_X 24          /* the map: columns 24-311, lines 27-154 */
#define YS_IMG_MAP_Y 27
#define YS_IMG_MAP_W 288
#define YS_IMG_MAP_H 128
#define YS_IMG_PANEL_TOP 171     /* 171-178 black; the panel 179-219, columns 8-327 */
#define YS_IMG_PANEL_Y 179
#define YS_IMG_PANEL_W 320
#define YS_IMG_PANEL_H 41
#define YS_IMG_LOAD_INNER_BOTTOM 218 /* load screen: inner area down to 218, outlines on lines 220 and 234 */
#define YS_IMG_LOAD_BOTTOM 234
#define YS_IMG_MIN_W 336         /* the smallest image of the game screen */
#define YS_IMG_MIN_H 220
/* The panel in the background map: 8-line cells (text, outline, player bar, outline, enemy bar, outline) */
#define YS_PANEL_ROWS 6
#define YS_PANEL_COLS 40
#define YS_BAR_FIRST_COL 8 /* 1st inner cell of the bar (the left side is at 7) */
/* Adol's frames for the equipment (16x32 sprites) */
#define YS_EQUIP_FRAMES 36
#define YS_EQUIP_W 16
#define YS_EQUIP_H 32
/* The game's 8x8 font copied for the menu (also the size of the cache file, with the little arrow) */
#define YS_MENU_FONT_TILES 39

#endif
