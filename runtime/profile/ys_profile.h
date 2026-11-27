/*
 * Profile of one edition of the game (review_jp/01_profile_design.md): the values that come from the disc and the
 * texts that name it, as plain data. Each executable links exactly one profile_<edition>.c, which defines
 * g_ys_profile; the common code reads it and never asks which edition it is.
 *
 * Types only: plain C, no OS headers, no SDL, nothing from app/ or host/ (tools/check_layers.py).
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
    uint16_t walker; /* slot of the walker hit */
    uint16_t actor;  /* character of each walker (indexed by the slot) */
    uint16_t gold;   /* GOLD (16 bits, low byte first) */
    uint16_t exp;    /* EXP (24 bits, low byte first) */
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
} YsProfile;

extern const YsProfile g_ys_profile;

#endif
