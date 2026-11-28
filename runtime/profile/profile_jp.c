/*
 * Profile of Ys I & II (Japan), PC Engine CD-ROM2 (Phase 3B-2, review_jp/04_phase3b_plan.md): the identity, the BIOS
 * policy and the disc's fingerprint only. Every feature is off (NULL) until its values are confirmed on the Japanese
 * disc (Phase 3C); no value of the USA edition is copied here.
 */
#include "profile/ys_profile.h"

/* Names chosen for this edition (review_jp/01_profile_design.md, P6, P9, P10; 04, 3B-2): English, ASCII */
static const YsIdentity k_identity = {
    .edition_id = "jp",
    .app_id = "YsIandII_JP",
    .ini_name = "YsIandII_JP.ini",
    .window_title = "Ys I & II (Japan)",
    .disc_name = "Ys I & II (Japan)",
};

/* Redump format (.cue + 43 .bin), measured on the user's disc with `app_tests --fingerprint <cue>`
 * (review_jp/03_disc_analysis.md, 3.2). No title text: the edition is told by the numbers only. */
static const YsDiscIdentity k_disc = {
    .track_count = 43,
    .toc_crc = 0xD09EEBFBu,
    .data_crc = 0xD47ABAE2u,
    .title = 0,
    .title_sector = 0,
    .game_name = "Ys I & II",
    .version_name = "Ys I & II, Japan",
};

/* The real System Card is required (P1, P12: the kanji font is in it). CRC32 of the user's System Card 3.0
 * (review_jp/03_disc_analysis.md, 8.1); another one gives a warning and still starts (04, Q5). */
static const YsBiosPolicy k_bios = {
    .required = 1,
    .check_crc = 1,
    .crc = 0x6D9A73EFu,
};

const YsProfile g_ys_profile = {
    .identity = &k_identity,
    .disc = &k_disc,
    .bios = &k_bios,
    /* music, screen, scroll, reward, windows, intro, dialog, fade, common, engine, panel, equip, menu_font,
     * launcher_sfx, intro_skip, trace: NULL (UNKNOWN on the Japanese disc; feature off) */
};
