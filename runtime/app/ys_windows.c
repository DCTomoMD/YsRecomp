/* Game windows read by the PC: see ys_windows.h and docs/ys_notes.md ("The game's window system"
 * system). */
#include "app/ys_windows.h"

#include "app/crop.h"

#include <string.h>

#include "profile/ys_profile.h"

#define RAM(p, a) ((p)->ram[(a) - 0x2000])

/* Window system routines and the first bytes of each one: g_ys_profile.windows */

static uint8_t read_logical(Pce *pce, uint16_t addr)
{
    return pce_bus_read(pce, (uint32_t)pce->cpu.mpr[addr >> 13] * 0x2000 + (addr & 0x1FFF));
}

/* Screen layout switch (docs/ys_notes.md, "Large menu screens"): the game saves or restores the map cells and copies
 * the border and the panel to cells 0-3 and 13-15, which only show in their place with the new scroll. Hooks on the
 * instructions that pick the interrupt line */
static int code_is(Pce *pce, const uint8_t *sig, int n)
{
    for (int k = 0; k < n; k++)
        if (read_logical(pce, (uint16_t)(pce->cpu.pc + k)) != sig[k])
            return 0;
    return 1;
}

static int is_layout_switch(Pce *pce)
{
    const YsWindowsProfile *p = g_ys_profile.windows;
    if (pce->cpu.mpr[3] != p->layout_bank)
        return 0;
    /* in the order of the old switch: Ys I, Ys II to the single scroll, then back to the split */
    for (int k = 0; k < (int)(sizeof(p->one_scroll_pc) / sizeof(p->one_scroll_pc[0])); k++)
        if (pce->cpu.pc == p->one_scroll_pc[k])
            return code_is(pce, p->one_scroll_sig, (int)sizeof(p->one_scroll_sig));
    for (int k = 0; k < (int)(sizeof(p->split_pc) / sizeof(p->split_pc[0])); k++)
        if (pce->cpu.pc == p->split_pc[k])
            return code_is(pce, p->split_sig, (int)sizeof(p->split_sig));
    return 0;
}

/* Lowest and highest PC of the hooks: a quick test before the table (computed once from the profile) */
static uint16_t g_pc_min, g_pc_max;
static int g_pc_range;

static void pc_range(const YsWindowsProfile *p)
{
    g_pc_min = g_pc_max = p->hooks[0].pc;
    for (int h = 1; h < YS_WIN_HOOKS; h++) {
        if (p->hooks[h].pc < g_pc_min)
            g_pc_min = p->hooks[h].pc;
        if (p->hooks[h].pc > g_pc_max)
            g_pc_max = p->hooks[h].pc;
    }
    g_pc_range = 1;
}

static int which_hook(Pce *pce)
{
    const YsWindowsProfile *p = g_ys_profile.windows;
    uint16_t pc = pce->cpu.pc;
    if (!g_pc_range)
        pc_range(p);
    if (pc < g_pc_min || pc > g_pc_max || pce->cpu.mpr[3] != p->bank)
        return -1;
    for (int h = 0; h < YS_WIN_HOOKS; h++) {
        if (p->hooks[h].pc != pc)
            continue;
        for (int k = 0; k < (int)sizeof(p->hooks[h].sig); k++)
            if (!(p->hooks[h].skip >> k & 1) && read_logical(pce, (uint16_t)(pc + k)) != p->hooks[h].sig[k])
                return -1;
        return h;
    }
    return -1;
}

/* Byte i of the current window in RAM: column, row, width, height, page */
static uint8_t rect_byte(Pce *pce, int i)
{
    return RAM(pce, g_ys_profile.windows->rect + i);
}

void ys_windows_init(YsWindows *w)
{
    memset(w, 0, sizeof(*w));
    w->first_mismatch = -1;
}

/* The window's map cell from the current logic camera ($80:54D4). The game treats the window as pinned to the screen:
 * on closing it restores the background using the current camera, and when the camera jumps with windows open (session
 * 20261031_000404, frame 8200: 80 pixels when opening a 2nd window over the SELECT menu) it redraws the map and the
 * window below at the map's new place; so the cell is recomputed every frame, not kept from opening time */
static void set_bat(YsWindow *win, Pce *pce)
{
    const YsWindowsProfile *p = g_ys_profile.windows;
    uint16_t cx = (uint16_t)(RAM(pce, p->camera_x) | RAM(pce, p->camera_x + 1) << 8);
    uint16_t cy = (uint16_t)(RAM(pce, p->camera_y) | RAM(pce, p->camera_y + 1) << 8);
    win->bat_x = (uint8_t)(((cx >> 3) + win->col) & 63);
    win->bat_y = (uint8_t)(((cy >> 3) + win->row) & 31);
}

/* Image line the console is drawing: what the game changes now shows from it downward (outside the image, in the
 * retrace: 0, the whole next image; the frame ends right after the last line, vce.c) */
static int image_row(Pce *pce)
{
    int r = pce->vce.vpos - VCE_ACTIVE_FIRST_LINE;
    return r >= 0 && r < VCE_ACTIVE_LINES ? r : 0;
}

static int same_rect(const YsWindow *win, Pce *pce)
{
    return win->col == rect_byte(pce, 0) && win->row == rect_byte(pce, 1) && win->w == rect_byte(pce, 2) &&
           win->h == rect_byte(pce, 3) && win->page == rect_byte(pce, 4);
}

void ys_windows_hook(YsWindows *w, Pce *pce)
{
    if (is_layout_switch(pce))
        w->layout_switch = 1;
    int h = which_hook(pce);
    if (h < 0)
        return;
    if (h == YS_WIN_HOOK_OPENING && w->count == 0) /* $3E may come dirty from the game's intro (ys_windows.h, n3e_extra) */
        w->n3e_extra = RAM(pce, g_ys_profile.windows->count);
    YsWindow *top = w->count ? &w->win[w->count - 1] : NULL;
    switch (h) {
    case YS_WIN_HOOK_OPENING: {
        if (w->count == YS_WIN_MAX)
            break;
        YsWindow *n = &w->win[w->count++];
        n->col = rect_byte(pce, 0);
        n->row = rect_byte(pce, 1);
        n->w = rect_byte(pce, 2);
        n->h = rect_byte(pce, 3);
        n->page = rect_byte(pce, 4);
        set_bat(n, pce);
        n->state = YS_WIN_OPENING;
        n->done = 0;
        n->vis0 = n->vis1 = n->row;
        n->chg_row = n->start_row = (uint8_t)image_row(pce);
        break;
    }
    case YS_WIN_HOOK_DRAWN: /* the border resets $40 to the 1st line at the end: without this, "opening" at the frame's end looked empty */
        if (top && top->state == YS_WIN_OPENING) {
            top->done = 1;
            top->vis1 = (uint8_t)(top->row + top->h);
            top->chg_row = (uint8_t)image_row(pce);
        }
        break;
    case YS_WIN_HOOK_OPEN:
        if (top && top->state == YS_WIN_OPENING) {
            top->state = YS_WIN_OPEN;
            top->vis0 = top->row;
            top->vis1 = (uint8_t)(top->row + top->h);
            w->opens++;
        }
        break;
    case YS_WIN_HOOK_CLOSING:
        /* the top one, the one the game just popped into $3F-$43 */
        for (int k = w->count - 1; k >= 0; k--)
            if (w->win[k].state == YS_WIN_OPEN && same_rect(&w->win[k], pce)) {
                w->win[k].state = YS_WIN_CLOSING;
                w->win[k].done = 0;
                w->win[k].start_row = (uint8_t)image_row(pce);
                break;
            }
        break;
    case YS_WIN_HOOK_RESTORED: /* the saved background fully restored (with no saved background, $6052 jumps to $605A) */
        for (int k = w->count - 1; k >= 0; k--)
            if (w->win[k].state == YS_WIN_CLOSING) {
                w->win[k].done = 1;
                w->win[k].vis0 = w->win[k].vis1;
                w->win[k].chg_row = (uint8_t)image_row(pce);
                break;
            }
        break;
    case YS_WIN_HOOK_CLOSED:
        for (int k = w->count - 1; k >= 0; k--)
            if (w->win[k].state == YS_WIN_CLOSING) {
                if (w->gone_n < YS_WIN_MAX) { /* leaves the list; in this frame's image, still above the change line */
                    YsWindow *g = &w->gone[w->gone_n++];
                    *g = w->win[k];
                    if (!g->done) /* no saved background: vanishes now (the game redraws the scene) */
                        g->chg_row = (uint8_t)image_row(pce);
                }
                memmove(&w->win[k], &w->win[k + 1], (size_t)(w->count - 1 - k) * sizeof(w->win[0]));
                w->count--;
                w->closes++;
                break;
            }
        break;
    }
}

void ys_windows_frame(YsWindows *w, Pce *pce, long frame)
{
    const YsWindowsProfile *p = g_ys_profile.windows;
    int row = rect_byte(pce, 1), open = 0;
    for (int k = 0; k < w->count; k++) {
        YsWindow *win = &w->win[k];
        int end = win->row + win->h;
        set_bat(win, pce);
        int v0 = win->vis0, v1 = win->vis1;
        if (win->done) /* border done, background restored */
            ;
        else if (win->state == YS_WIN_OPENING) /* the border goes down line by line ($40 = the next one) */
            win->vis1 = (uint8_t)(row >= win->row && row <= end ? row : win->row);
        else if (win->state == YS_WIN_CLOSING) /* the saved background comes back from top to bottom */
            win->vis0 = (uint8_t)(row >= win->row && row <= end ? row : win->row);
        if ((win->vis0 != v0 || win->vis1 != v1) && !win->chg_row)
            win->chg_row = 1; /* moved during the frame: the image started as it was */
        open += win->state == YS_WIN_OPEN;
    }
    int n3e = RAM(pce, p->count) - w->n3e_extra;
    w->last_ok = 1;
    if (w->count == 0)
        return; /* no window (in the game's intro, $3E is something else) */
    w->frames++;
    int ok = open == n3e;
    /* the 5 bytes of each open one, in order, on the game's stack (shared with other uses: no fixed place) */
    const uint8_t *s = &RAM(pce, p->stack);
    int len = RAM(pce, p->stack_len), pos = 0, room = (int)sizeof(pce->ram) - (p->stack - 0x2000);
    if (len > room) /* no further than the end of RAM */
        len = room;
    for (int k = 0; k < w->count && ok; k++) {
        const YsWindow *b = &w->win[k];
        if (b->state != YS_WIN_OPEN)
            continue;
        while (pos + 5 <= len && !(s[pos] == b->col && s[pos + 1] == b->row && s[pos + 2] == b->w &&
                                   s[pos + 3] == b->h && s[pos + 4] == b->page))
            pos++;
        ok = pos + 5 <= len;
        pos += 5;
    }
    if (!ok) {
        w->last_ok = 0;
        if (w->mismatch++ == 0)
            w->first_mismatch = frame;
        if (n3e == 0) /* the game closed everything outside the routines (e.g. new screen): so does the list here */
            w->count = 0;
    }
}

int ys_window_rect(const YsWindow *win, uint16_t cam_x, uint16_t cam_y, int r[4])
{
    if (win->vis1 <= win->vis0)
        return 0;
    int x0 = ((win->bat_x * 8 - cam_x) % 512 + 512 + 256) % 512 - 256;
    int y0 = (((win->bat_y + win->vis0 - win->row) * 8 - cam_y) % 256 + 256 + 128) % 256 - 128;
    r[0] = x0;
    r[1] = y0;
    r[2] = x0 + win->w * 8;
    r[3] = y0 + (win->vis1 - win->vis0) * 8;
    return 1;
}

int ys_window_sprites(const YsList *l, uint8_t mine[YS_ACTORS])
{
    int n = 0;
    for (int i = 0; i < YS_ACTORS; i++) {
        int a = l->owner[i];
        mine[i] = (uint8_t)(i < l->count && a < YS_ACTORS && l->drawn[a] && !(l->flags[a] & 0x20));
        n += mine[i];
    }
    return n;
}

void ys_sprite_box(const YsList *l, int i, int r[4])
{
    static const int k_w[2] = { 16, 32 }, k_h[4] = { 16, 32, 64, 64 };
    uint16_t at = l->sat[i * 4 + 3];
    r[0] = (l->sat[i * 4 + 1] & 1023) - 32;
    r[1] = (l->sat[i * 4] & 1023) - 88;
    r[2] = r[0] + k_w[(at >> 8) & 1];
    r[3] = r[1] + k_h[(at >> 12) & 3];
}

void ys_windows_snapshot(YsWindows *w, const YsFrameList *eng, YsWinFrame *out)
{
    out->count = w->count;
    memcpy(out->win, w->win, sizeof(out->win));
    out->prev_count = w->snap_n;
    memcpy(out->prev, w->snap_win, sizeof(out->prev));
    out->gone_n = w->gone_n;
    memcpy(out->gone, w->gone, sizeof(out->gone));
    w->gone_n = 0;
    out->layout_switch = w->layout_switch;
    w->layout_switch = 0;
    w->snap_n = w->count;
    memcpy(w->snap_win, w->win, sizeof(w->snap_win));
    for (int k = 0; k < w->count; k++) /* in the next frame, no change until a hook marks another */
        w->win[k].chg_row = w->win[k].start_row = 0;
    out->scene = 0;
    for (int k = 0; k < w->count; k++)
        out->scene |= !w->win[k].page;
    /* the previous frame's ones (the table shown may be the one from its list) */
    out->prev_sprites_ok = w->last_sprites_ok;
    memcpy(out->prev_own, w->last_own, sizeof(out->prev_own));
    memcpy(out->prev_sat, w->last_sat, sizeof(out->prev_sat));
    memset(out->own, 0, sizeof(out->own));
    out->sprites_ok = eng && eng->kind != YS_LIST_NONE;
    w->last_sprites_ok = 0;
    if (!out->sprites_ok) {
        w->prev_n = 0;
        return;
    }
    memcpy(out->sat, eng->list.sat, sizeof(out->sat));
    /* this frame's windows and the previous one's (the list shown was built in it) */
    int rects[2 * YS_WIN_MAX][4], nr = 0;
    for (int k = 0; k < w->count; k++)
        nr += ys_window_rect(&w->win[k], eng->list.cam_x, eng->list.cam_y, rects[nr]);
    int cur = nr;
    for (int p = 0; p < w->prev_n; p++)
        memcpy(rects[nr++], w->prev_r[p], sizeof(rects[0]));
    uint8_t mine[YS_ACTORS];
    if (ys_window_sprites(&eng->list, mine))
        for (int i = 0; i < YS_ACTORS; i++) {
            if (!mine[i])
                continue;
            int b[4];
            ys_sprite_box(&eng->list, i, b);
            for (int k = 0; k < nr && !out->own[i]; k++)
                out->own[i] = b[0] >= rects[k][0] && b[1] >= rects[k][1] && b[2] <= rects[k][2] && b[3] <= rects[k][3];
        }
    w->prev_n = cur;
    memcpy(w->prev_r, rects, (size_t)cur * sizeof(rects[0]));
    w->last_sprites_ok = 1;
    memcpy(w->last_own, out->own, sizeof(w->last_own));
    memcpy(w->last_sat, out->sat, sizeof(w->last_sat));
}

/* Each window of the frame as it was at the end of the previous frame (before) and at the end of this one (after), and
 * the image line where it changed (chg; 0 = before the 1st). Returns how many */
static int before_after(const YsWinFrame *f, YsWindow *before, YsWindow *after, int *chg)
{
    int na = 0;
    uint8_t used[YS_WIN_MAX] = { 0 };
    for (int k = 0; k < f->count + f->gone_n; k++) {
        const YsWindow *v = k < f->count ? &f->win[k] : &f->gone[k - f->count];
        before[na] = after[na] = *v;
        before[na].vis0 = before[na].vis1 = 0; /* new: nothing before */
        for (int p = 0; p < f->prev_count; p++) {
            const YsWindow *o = &f->prev[p];
            if (!used[p] && o->col == v->col && o->row == v->row && o->w == v->w && o->h == v->h && o->page == v->page) {
                used[p] = 1;
                before[na] = *o;
                break;
            }
        }
        if (k >= f->count) /* closed in this frame: nothing after */
            after[na].vis0 = after[na].vis1 = 0;
        chg[na++] = v->chg_row;
    }
    return na;
}

int ys_windows_cells(const YsWinFrame *f, int tx, int ty, uint8_t *cells)
{
    memset(cells, 0, (size_t)tx * ty);
    YsWindow before[3 * YS_WIN_MAX], after[3 * YS_WIN_MAX];
    int chg[3 * YS_WIN_MAX], na = before_after(f, before, after, chg), n = 0;
    /* Windows wrap around the game's 64x32 map, also with the VDC at 64x64 */
    int wx = tx < 64 ? tx : 64, wy = ty < 32 ? ty : 32;
    for (int k = 0; k < na; k++) {
        const YsWindow *a = &after[k], *b = &before[k];
        /* changed in this frame (opened, border going down or done, background coming back, closed): the lines that were
         * window at the end of the previous one and no longer are stay until the game rewrites the cell; the new ones,
         * from when it writes it */
        int changed = chg[k] > 0 || a->start_row > 0;
        for (int y = a->row; y < a->row + a->h; y++) {
            int in_a = y >= a->vis0 && y < a->vis1, in_b = changed ? y >= b->vis0 && y < b->vis1 : in_a;
            if (!in_a && !in_b)
                continue;
            uint8_t v = in_a && in_b ? YS_CELL_WIN : (uint8_t)((in_b ? YS_CELL_UNTIL : YS_CELL_FROM) | k);
            for (int x = 0; x < a->w; x++) {
                uint8_t *c = &cells[((a->bat_y + y - a->row) & (wy - 1)) * tx + ((a->bat_x + x) & (wx - 1))];
                n += !*c;
                /* one window over another: certain > until rewritten > from when written */
                if (!*c || v == YS_CELL_WIN || ((*c & YS_CELL_FROM) && (v & YS_CELL_UNTIL)))
                    *c = v;
            }
        }
    }
    return n;
}

void ys_windows_write(const YsWinFrame *f, uint8_t *cells, int tx, int ty, uint32_t addr, int row)
{
    if (addr >= (uint32_t)(tx * ty))
        return;
    uint8_t c = cells[addr];
    if (!(c & (YS_CELL_UNTIL | YS_CELL_FROM)))
        return;
    int k = c & 0x3F;
    const YsWindow *v = k < f->count ? &f->win[k] : k - f->count < f->gone_n ? &f->gone[k - f->count] : NULL;
    if (v && v->start_row > 1 && row < v->start_row) /* before the window starts changing (its text, for example) */
        return;
    cells[addr] = (c & YS_CELL_UNTIL) ? 0 : YS_CELL_WIN;
}

/* The camera the image shows on the map, from the lines' scroll: windows are pinned to the screen, so their cell in the
 * image comes from it, not from the logic camera ($2C37-$2C3A), which may be ahead (returning from the save screen it
 * comes back 2 frames before the scroll). The middle of the map area always holds the map section; if that starts after
 * the map's 1st line (the late scroll change), the camera is the scroll at its start. 0 = no map in the image */
static int shown_camera(const VdcLineInfo *rows, int h, int *cx, int *cy)
{
    int mid = CROP_MAP_Y + CROP_MAP_H / 2, s = mid;
    if (h <= mid || !rows[mid].valid)
        return 0;
    while (s > CROP_MAP_Y && rows[s - 1].valid && rows[s - 1].bxr == rows[mid].bxr &&
           ((rows[s - 1].bgy - (s - 1) - (rows[mid].bgy - mid)) & 255) == 0)
        s--;
    *cx = rows[mid].bxr & 511;
    *cy = rows[s].bgy & 255;
    return 1;
}

int ys_windows_boxes(const YsWinFrame *f, const VdcLineInfo *rows, int h, int boxes[][4], int max, int *scene)
{
    *scene = f->count && f->scene;
    if (*scene)
        return 0;
    int cam_x, cam_y;
    if (!shown_camera(rows, h, &cam_x, &cam_y))
        return 0;
    /* each window as it was at the end of the previous frame (before) and at the end of this one (after). The image goes
     * out during the frame: if the change (chg_row: border done, background restored, closed) came after the 1st line,
     * it started with before. The window counts at the start of the image or at the end of the frame (one that opens
     * mid-image already shrinks in the same frame) */
    YsWindow before[3 * YS_WIN_MAX], after[3 * YS_WIN_MAX];
    int chg[3 * YS_WIN_MAX], na = before_after(f, before, after, chg);
    int n = 0;
    for (int k = 0; k < na && n < max; k++) {
        int x0 = 1 << 20, x1 = -(1 << 20), y0 = -1, y1 = -1;
        /* only the map lines: returning from the save screen, the top border and the panel still have BG and sprites on */
        for (int r = CROP_MAP_Y; r < h && r < CROP_MAP_Y + CROP_MAP_H; r++) {
            const VdcLineInfo *li = &rows[r];
            if (!li->valid || (li->cr & 0xC0) != 0xC0)
                continue;
            /* on the image's 1st line (before, if it changed after it) or at the end of the frame (after) */
            const YsWindow *win = NULL;
            for (int s = 0; s < 2 && !win; s++) {
                const YsWindow *t = s == 0 && chg[k] > 0 ? &before[k] : &after[k];
                int nv = t->vis1 - t->vis0, top = (cam_y >> 3) + t->vis0;
                if (nv > 0 && ((((li->bgy & 255) >> 3) - top) & 31) < nv)
                    win = t;
            }
            if (!win)
                continue;
            int dx = ((((cam_x >> 3) + win->col) * 8 - (li->bxr & 511)) & 511);
            if (dx >= 256)
                dx -= 512;
            int a = li->x0 + dx, b = a + win->w * 8 - 1;
            if (a < li->x0)
                a = li->x0;
            if (b > li->x0 + li->width - 1)
                b = li->x0 + li->width - 1;
            if (a > b)
                continue;
            x0 = a < x0 ? a : x0;
            x1 = b > x1 ? b : x1;
            if (y0 < 0)
                y0 = r;
            y1 = r;
        }
        if (y0 >= 0) {
            boxes[n][0] = x0;
            boxes[n][1] = y0;
            boxes[n][2] = x1;
            boxes[n][3] = y1;
            n++;
        }
    }
    return n;
}
