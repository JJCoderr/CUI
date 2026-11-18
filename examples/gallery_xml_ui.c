#define _CRT_SECURE_NO_WARNINGS 1

/* The component gallery of examples/gallery_ui.c, with its interface in
 * examples/gallery.xml.
 *
 * The document holds the sidebar, the page list and every page; this file
 * holds what a document cannot say: the gallery's own values (the text of the
 * fields it binds, the page, the theme and the accent), the accent swatches,
 * which are painted, and the actions the document fires.
 *
 * The comparison test (tests/xml_gallery_shots.c) draws the same page through
 * gallery_frame and through this document and counts the pixels that differ.
 */
#include "gallery_xml_ui.h"

#include <cui/cui_xml.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- what the document writes and reads --------------------------------- */

static gallery *g_app;
static int g_mode;   /* the theme segmented holds an index, the gallery a bool */
static int g_theme_set = -1;   /* the theme the context already has */
static int g_upload;           /* the job of the feedback page: idle, busy, done, failed */
static bool g_grayscale;       /* the pattern meter draws its parts by density */

/* ---- the accent swatch, as examples/gallery_ui.c paints it -------------- */

typedef struct {
    cui_color color;
    bool selected;
} swatch_paint;

static void paint_swatch(cui_ctx *ui, cui_rect r, void *user) {
    swatch_paint *d = user;
    cui_vec2 c = { r.x + r.w / 2, r.y + r.h / 2 };
    if (d->selected) cui_draw_circle(ui, c, r.w / 2, cui_color_of(ui, CUI_COLOR_FOREGROUND));
    cui_draw_circle(ui, c, r.w / 2 - (d->selected ? 2.0f : 0.0f), cui_color_of(ui, CUI_COLOR_SURFACE_MUTED));
    cui_draw_circle(ui, c, r.w / 2 - 4, d->color);
}

/* <Swatch Index="3" Selected="{accent == 3}"/>: 22 points square, a tooltip
 * with the accent's name, a click sets it. */
static bool tag_swatch(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    static const unsigned hex[CUI_ACCENT_COUNT] = { 0x555555, 0x7747ff, 0x0562ef, 0x00864b,
                                                    0xf3ad20, 0xc25300, 0xd1403a, 0xd23286 };
    const char *index_text = cui_xml_node_attr_value(doc, node, "index");
    int index = index_text && *index_text ? atoi(index_text) : 0;
    const char *selected_text = cui_xml_node_attr_value(doc, node, "selected");
    swatch_paint d;
    cui_canvas_result r;
    (void)user;
    if (index < 0 || index >= CUI_ACCENT_COUNT) return false;
    d.color = CUI_RGB(hex[index]);
    d.selected = selected_text && (!strcmp(selected_text, "1") || !strcmp(selected_text, "true") ||
                                   !strcmp(selected_text, "yes"));
    r = cui_canvas(ui, cui_xml_node_name(node), CUI_PX(22), CUI_PX(22), paint_swatch, &d, sizeof d);
    cui_tooltip(ui, cui_accent_name((cui_accent)index));
    if (r.clicked) {
        if (g_app) g_app->accent = index;
        cui_set_accent(ui, (cui_accent)index);
    }
    return true;
}

/* ---- the virtual list of the data page, as examples/gallery_ui.c builds it */

static bool tag_invoices(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    int first, last, i;
    (void)doc;
    (void)user;
    cui_virtual_list_begin(ui, cui_xml_node_name(node), 10000, 36, CUI_PX(220), &first, &last);
    for (i = first; i < last; i++) {
        char number[64], amount[32];
        unsigned h = (unsigned)(i + 1) * 2654435761u;
        snprintf(number, sizeof number, "Invoice 2026-%05d", 10000 - i);
        snprintf(amount, sizeof amount, "$%u.%02u", 40 + (h >> 8) % 2400, (h >> 4) % 100);
        cui_row(ui, .height = CUI_PX(36), .pad_x = 12, .gap = 12) {
            cui_icon(ui, CUI_ICON_FILE_TEXT, 16, CUI_COLOR_TEXT_MUTED);
            cui_text(ui, number, .nowrap = true, .width = CUI_FILL, .tabular = true);
            cui_text(ui, amount, .size = 13, .tabular = true, .nowrap = true);
            cui_text(ui, (h >> 20) % 5 == 0 ? "Due" : "Paid", .size = 12, .color = CUI_COLOR_TEXT_MUTED,
                     .nowrap = true, .width = CUI_PX(32));
        }
    }
    cui_virtual_list_end(ui);
    return true;
}

/* ---- actions ------------------------------------------------------------ */

static void on_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    gallery *g = user ? user : g_app;
    (void)doc;
    (void)node;
    if (!g) return;
    if (!strncmp(action, "page:", 5)) {
        int page = atoi(action + 5);
        int total = gallery_page_total();
        if (page >= 0 && page < total) g->page = page;
    } else if (!strcmp(action, "theme.sync")) {
        g->dark = g_mode == 1;
    } else if (!strcmp(action, "loading:on")) {
        g->loading = true;
    } else if (!strcmp(action, "loading:off")) {
        g->loading = false;
    } else if (!strncmp(action, "tab:", 4)) {
        int tab = atoi(action + 4);
        if (tab >= 0 && tab < 4) g->tab = tab;
    }
}

/* ---- wiring ------------------------------------------------------------- */

cui_xml *gallery_xml_open(const char *path, gallery *g) {
    cui_xml *doc = cui_xml_load(path);
    if (!doc) return NULL;
    g_app = g;
    g_mode = g->dark ? 1 : 0;

    /* what the document writes: the gallery's own state */
    cui_xml_bind_int(doc, "page", &g->page);
    cui_xml_bind_int(doc, "mode", &g_mode);
    cui_xml_bind_int(doc, "accent", &g->accent);
    cui_xml_bind_int(doc, "seg_view", &g->seg_view);
    cui_xml_bind_int(doc, "seg_size", &g->seg_size);
    cui_xml_bind_int(doc, "plan", &g->plan);
    cui_xml_bind_int(doc, "plan_card", &g->plan_card);
    cui_xml_bind_int(doc, "framework", &g->framework);
    cui_xml_bind_int(doc, "country", &g->country);
    cui_xml_bind_int(doc, "tab", &g->tab);
    cui_xml_bind_int(doc, "tab2", &g->tab2);
    cui_xml_bind_int(doc, "page_index", &g->page_index);
    cui_xml_bind_int(doc, "step", &g->step);
    cui_xml_bind_int(doc, "rating", &g->rating);
    cui_xml_bind_int(doc, "selected_row", &g->selected_row);
    cui_xml_bind_int(doc, "selected_file", &g->selected_file);
    cui_xml_bind_bool(doc, "terms", &g->terms);
    cui_xml_bind_bool(doc, "newsletter", &g->newsletter);
    cui_xml_bind_bool(doc, "marketing", &g->marketing);
    cui_xml_bind_bool(doc, "wifi", &g->wifi);
    cui_xml_bind_bool(doc, "bluetooth", &g->bluetooth);
    cui_xml_bind_bool(doc, "airplane", &g->airplane);
    cui_xml_bind_bool(doc, "loading", &g->loading);
    cui_xml_bind_int(doc, "upload", &g_upload);
    cui_xml_bind_bool(doc, "grayscale", &g_grayscale);
    cui_xml_bind_bool(doc, "tree_src", &g->tree_src);
    cui_xml_bind_bool(doc, "tree_ui", &g->tree_ui);
    cui_xml_bind_bool(doc, "tree_assets", &g->tree_assets);
    cui_xml_bind_bool(doc, "acc0", &g->acc[0]);
    cui_xml_bind_bool(doc, "acc1", &g->acc[1]);
    cui_xml_bind_bool(doc, "acc2", &g->acc[2]);
    cui_xml_bind_bool(doc, "chip0", &g->chips[0]);
    cui_xml_bind_bool(doc, "chip1", &g->chips[1]);
    cui_xml_bind_bool(doc, "chip2", &g->chips[2]);
    cui_xml_bind_bool(doc, "chip3", &g->chips[3]);
    cui_xml_bind_bool(doc, "chip4", &g->chips[4]);
    cui_xml_bind_bool(doc, "chip5", &g->chips[5]);
    cui_xml_bind_text(doc, "name", g->name, sizeof g->name);
    cui_xml_bind_text(doc, "email", g->email, sizeof g->email);
    cui_xml_bind_text(doc, "password", g->password, sizeof g->password);
    cui_xml_bind_text(doc, "search", g->search, sizeof g->search);
    cui_xml_bind_text(doc, "bio", g->bio, sizeof g->bio);
    cui_xml_bind_text(doc, "tags", g->tags, sizeof g->tags);
    cui_xml_bind_text(doc, "otp", g->otp, sizeof g->otp);

    cui_xml_on_any(doc, on_action, g);
    cui_xml_register_tag(doc, "Swatch", tag_swatch, g);
    cui_xml_register_tag(doc, "Invoices", tag_invoices, g);
    return doc;
}

void gallery_xml_values(cui_xml *doc, gallery *g) {
    char text[24];
    int page = g->page;
    snprintf(text, sizeof text, "v%d.%d", CUI_VERSION_MAJOR, CUI_VERSION_MINOR);
    cui_xml_set_var(doc, "version", text);
    cui_xml_set_var(doc, "pageTitle", page >= 0 && page < gallery_page_total() ? gallery_page_name(page) : "");
    /* the theme segmented shows the gallery's theme, whatever set it */
    g_mode = g->dark ? 1 : 0;
}

void gallery_xml_apply_theme(cui_ctx *ui, gallery *g) {
    int want = (int)(g->dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
    if (g_theme_set != want) {
        cui_set_theme(ui, (cui_theme_mode)want);
        g_theme_set = want;
    }
}
