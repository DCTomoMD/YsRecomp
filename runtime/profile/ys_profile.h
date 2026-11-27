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

typedef struct YsProfile {
    const YsIdentity *identity;
    const YsDiscIdentity *disc;
    const YsMusicProfile *music;
    const YsScreenProfile *screen;
    const YsScrollProfile *scroll;
    const YsRewardProfile *reward;
} YsProfile;

extern const YsProfile g_ys_profile;

#endif
