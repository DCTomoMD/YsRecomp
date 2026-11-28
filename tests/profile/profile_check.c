/*
 * Checks one edition's profile, linked alone (no runtime/app, no SDL): which parts it gives and which are NULL
 * (feature off), and its identity and BIOS policy (review_jp/04_phase3b_plan.md, 3B-2).
 *
 *   profile_check EDITION BIOS_REQUIRED PRESENT
 *     EDITION        the expected identity->edition_id
 *     BIOS_REQUIRED  the expected bios->required (0 or 1)
 *     PRESENT        comma-separated list of the parts that must be non-NULL; every other part must be NULL
 *
 * Exit: 0 = "profile_check: OK"; 1 = some difference ("FAILED: ..."); 2 = usage error.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "profile/ys_profile.h"

typedef struct {
    const char *name;
    const void *ptr;
} Part;

#define PART(f) { #f, (const void *)g_ys_profile.f }

static const char *text(const char *s) { return s ? s : "(null)"; }

static int listed(const char *list, const char *name)
{
    size_t n = strlen(name);
    for (const char *p = list; *p;) {
        const char *end = strchr(p, ',');
        size_t len = end ? (size_t)(end - p) : strlen(p);
        if (len == n && strncmp(p, name, n) == 0)
            return 1;
        if (!end)
            break;
        p = end + 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: profile_check EDITION BIOS_REQUIRED PRESENT\n");
        return 2;
    }
    const Part parts[] = {
        PART(identity), PART(disc), PART(bios), PART(music), PART(screen), PART(scroll), PART(reward),
        PART(windows), PART(intro), PART(dialog), PART(fade), PART(common), PART(engine), PART(panel),
        PART(equip), PART(menu_font), PART(launcher_sfx), PART(intro_skip), PART(trace),
    };
    /* every pointer of YsProfile is in parts[] (a new one must be added above) */
    _Static_assert(sizeof(YsProfile) == 19 * sizeof(const void *), "parts[] does not list every part of YsProfile");
    int fails = 0;

    for (size_t i = 0; i < sizeof(parts) / sizeof(parts[0]); i++) {
        int want = listed(argv[3], parts[i].name);
        int have = parts[i].ptr != NULL;
        printf("profile_check: %-12s %s\n", parts[i].name, have ? "present" : "NULL (off)");
        if (want != have) {
            printf("profile_check: FAILED: %s is %s, expected %s\n", parts[i].name, have ? "present" : "NULL",
                   want ? "present" : "NULL");
            fails++;
        }
    }
    const YsIdentity *id = g_ys_profile.identity;
    if (id) {
        printf("profile_check: edition %s, app %s, ini %s, window \"%s\", disc \"%s\"\n", text(id->edition_id),
               text(id->app_id), text(id->ini_name), text(id->window_title), text(id->disc_name));
        if (!id->edition_id || strcmp(id->edition_id, argv[1]) != 0) {
            printf("profile_check: FAILED: edition %s, expected %s\n", text(id->edition_id), argv[1]);
            fails++;
        }
        const char *names[] = { id->app_id, id->ini_name, id->window_title, id->disc_name };
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++)
            if (!names[i] || !names[i][0]) {
                printf("profile_check: FAILED: an identity text is empty\n");
                fails++;
                break;
            }
    }
    const YsBiosPolicy *bios = g_ys_profile.bios;
    if (bios) {
        printf("profile_check: bios required %d, crc %s\n", bios->required, bios->check_crc ? "checked" : "not checked");
        if (bios->required != atoi(argv[2])) {
            printf("profile_check: FAILED: bios required %d, expected %s\n", bios->required, argv[2]);
            fails++;
        }
    }
    const YsDiscIdentity *disc = g_ys_profile.disc;
    if (disc) {
        printf("profile_check: disc %d tracks, title %s, game \"%s\", version \"%s\"\n", disc->track_count,
               disc->title ? "given" : "none", text(disc->game_name), text(disc->version_name));
        if (disc->track_count <= 0 || !disc->game_name || !disc->version_name) {
            printf("profile_check: FAILED: the disc identity is incomplete\n");
            fails++;
        }
    }
    printf("profile_check: %s\n", fails ? "FAILED" : "OK");
    return fails ? 1 : 0;
}
