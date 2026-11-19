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

#include "gallery_more.h"

#include <cui/cui_xml.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- what the document writes and reads --------------------------------- */

static gallery *g_app;
static int g_mode;   /* the theme segmented holds an index, the gallery a bool */
static int g_theme_set = -1;   /* the theme the context already has */
static int g_upload;           /* the job of the feedback page: idle, busy, done, failed */
static bool g_grayscale;       /* the pattern meter draws its parts by density */
static void gallery_xml_voice_reset(void);
static int g_audio_frame;      /* the frame the audio meters are drawn in */

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

/* <CanvasPicture Width="234" Height="150" Radius="10"/>: the illustration the
 * gallery paints in C, which a document can place but not paint. */
static bool tag_canvas_picture(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_image_opts o;
    const char *w = cui_xml_node_attr_value(doc, node, "width");
    const char *h = cui_xml_node_attr_value(doc, node, "height");
    const char *r = cui_xml_node_attr_value(doc, node, "radius");
    (void)user;
    memset(&o, 0, sizeof o);
    if (w && *w) o.width = CUI_PX((float)atof(w));
    if (h && *h) o.height = CUI_PX((float)atof(h));
    if (r && *r) o.radius = (float)atof(r);
    cui_image_ex(ui, gallery_more_canvas_picture(ui), &o);
    return true;
}

/* <PriceChart Height="280"/>: two thousand candles a document cannot carry as
 * text; the data is made here, as examples/gallery_more.c makes it. */
static bool tag_price_chart(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    static cui_candle *candles;
    static int count;
    cui_candlestick_opts o;
    const char *h;
    (void)user;
    if (!candles) {
        int k;
        float price = 64;
        count = 2000;
        candles = malloc(sizeof *candles * (size_t)count);
        if (!candles) {
            count = 0;
            return true;
        }
        for (k = 0; k < count; k++) {
            float open = price, close = open * (1 + sinf(k * 0.37f) * 0.012f + cosf(k * 0.043f) * 0.008f);
            candles[k] = (cui_candle){ open, fmaxf(open, close) * 1.006f, fminf(open, close) * 0.994f, close,
                                       2000 + (float)((k * 7919) % 900) * 10 };
            price = close;
        }
    }
    memset(&o, 0, sizeof o);
    h = cui_xml_node_attr_value(doc, node, "height");
    if (h && *h) o.height = (float)atof(h);
    cui_candlestick_ex(ui, cui_xml_node_name(node), candles, count, &o);
    return true;
}

/* <SceneViewer>, <SceneCropper> and <SceneCompare>: the social page shows the
 * scene examples/gallery_more.c paints around, in colour and in gray. A document
 * can say where they go and how tall; the pictures and the view live here. */
#define SCENE_ID 0x5CE9E11u
#define SCENE_W 640
#define SCENE_H 400

typedef struct {
    cui_texture before, after;
    cui_image_view view;
    cui_rect crop;
    float split;
    bool init;
} scene_state;

static void scene_state_free(cui_ctx *ui, void *data) {
    scene_state *s = data;
    if (s->before) cui_image_destroy(ui, s->before);
    if (s->after) cui_image_destroy(ui, s->after);
}

static scene_state *scene_of(cui_ctx *ui) {
    scene_state *s = cui_state(ui, SCENE_ID, sizeof *s);
    if (!s) return NULL;
    if (!s->before) {
        static unsigned char gray[SCENE_W * SCENE_H * 4], color[SCENE_W * SCENE_H * 4];
        gallery_more_scene(gray, SCENE_W, SCENE_H, false);
        gallery_more_scene(color, SCENE_W, SCENE_H, true);
        s->before = cui_image_create(ui, SCENE_W, SCENE_H, gray);
        s->after = cui_image_create(ui, SCENE_W, SCENE_H, color);
        s->split = 0.45f;
        s->crop = (cui_rect){ 0, 0, 0, 0 };   /* the cropper fits the image until dragged */
        cui_state_on_free(ui, SCENE_ID, scene_state_free);
    }
    return s;
}

static bool tag_scene_viewer(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_image_viewer_opts o;
    scene_state *s = scene_of(ui);
    const char *h = cui_xml_node_attr_value(doc, node, "height");
    (void)user;
    if (!s) return true;
    memset(&o, 0, sizeof o);
    if (h && *h) o.height = CUI_PX((float)atof(h));
    o.no_toolbar = false;
    cui_image_viewer_ex(ui, cui_xml_node_name(node), s->after, SCENE_W, SCENE_H, &s->view, &o);
    return true;
}

static bool tag_scene_cropper(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_cropper_opts o;
    scene_state *s = scene_of(ui);
    const char *h = cui_xml_node_attr_value(doc, node, "height");
    const char *aspect = cui_xml_node_attr_value(doc, node, "aspect");
    (void)user;
    if (!s) return true;
    memset(&o, 0, sizeof o);
    if (h && *h) o.height = CUI_PX((float)atof(h));
    if (aspect && *aspect) o.aspect = (float)atof(aspect);
    cui_cropper_ex(ui, cui_xml_node_name(node), s->after, SCENE_W, SCENE_H, &s->crop, &o);
    return true;
}

static bool tag_scene_compare(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_compare_opts o;
    scene_state *s = scene_of(ui);
    const char *h = cui_xml_node_attr_value(doc, node, "height");
    (void)user;
    if (!s) return true;
    memset(&o, 0, sizeof o);
    if (h && *h) o.height = (float)atof(h);
    o.aspect = (float)SCENE_W / (float)SCENE_H;
    cui_image_compare_ex(ui, cui_xml_node_name(node), s->before, s->after, &s->split, &o);
    return true;
}

/* <PeopleGrid/>: the data grid of the Data grid page. Its two thousand rows are
 * generated, so they stay in C and the document only says where the grid goes. */
static bool tag_people_grid(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    gallery_more_people_grid(ui);
    return true;
}

/* <FileBrowser/> and <Shell/>: the browser and the terminal of the Files page.
 * Both read the disk and keep their own state, so they live in C. */
static bool g_open_dlg, g_save_dlg;

static bool tag_file_browser(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    gallery_more_file_browser(ui);
    gallery_more_file_pickers(ui, &g_open_dlg, &g_save_dlg);
    return true;
}

static bool tag_shell(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    gallery_more_terminal(ui);
    return true;
}

/* <CommitGraph/>, <GraphView/>, <Waterfall/> and <TraceView/>: the panels whose
 * data is built in C (history, dependencies, requests, a trace of 400 spans). */
static bool tag_commit_graph(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    gallery_more_commit_graph(ui);
    return true;
}

static bool tag_graph_view(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    gallery_more_graph_view(ui);
    return true;
}

static bool tag_waterfall(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    gallery_more_waterfall(ui);
    return true;
}

static bool tag_trace_view(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    gallery_more_trace(ui);
    return true;
}

static bool tag_blame(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    gallery_more_blame(ui);
    return true;
}

static bool tag_uptime(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    gallery_more_uptime(ui);
    return true;
}

static bool tag_time_histogram(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    gallery_more_time_histogram(ui);
    return true;
}

static bool tag_result_card(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    gallery_more_result_card(ui);
    return true;
}

static bool tag_canvas_files(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    gallery_more_canvas_files(ui);
    return true;
}

/* ---- actions ------------------------------------------------------------ */

static void on_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    gallery *g = user ? user : g_app;
    (void)doc;
    (void)node;
    if (!g) return;
    if (!strcmp(action, "open-file")) {
        g_open_dlg = true;
        return;
    }
    if (!strcmp(action, "save-file")) {
        g_save_dlg = true;
        return;
    }
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
    g_audio_frame = 0;
    gallery_xml_voice_reset();

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
    cui_xml_register_tag(doc, "CanvasPicture", tag_canvas_picture, g);
    cui_xml_register_tag(doc, "PriceChart", tag_price_chart, g);
    cui_xml_register_tag(doc, "SceneViewer", tag_scene_viewer, g);
    cui_xml_register_tag(doc, "SceneCropper", tag_scene_cropper, g);
    cui_xml_register_tag(doc, "SceneCompare", tag_scene_compare, g);
    cui_xml_register_tag(doc, "PeopleGrid", tag_people_grid, g);
    cui_xml_register_tag(doc, "FileBrowser", tag_file_browser, g);
    cui_xml_register_tag(doc, "Shell", tag_shell, g);
    cui_xml_register_tag(doc, "CommitGraph", tag_commit_graph, g);
    cui_xml_register_tag(doc, "GraphView", tag_graph_view, g);
    cui_xml_register_tag(doc, "Waterfall", tag_waterfall, g);
    cui_xml_register_tag(doc, "TraceView", tag_trace_view, g);
    cui_xml_register_tag(doc, "Blame", tag_blame, g);
    cui_xml_register_tag(doc, "Uptime", tag_uptime, g);
    cui_xml_register_tag(doc, "TimeHistogram", tag_time_histogram, g);
    cui_xml_register_tag(doc, "ResultCard", tag_result_card, g);
    cui_xml_register_tag(doc, "CanvasFiles", tag_canvas_files, g);
    return doc;
}

/* The Instruments page shows a dose model: a document can hold its text and its
 * arrays as values, so the figures are computed here, exactly as
 * examples/gallery_more.c computes them, and handed to the document once. */
static float gallery_xml_dose(double t) {
    double ke, ka;
    if (t <= 0) return 0;
    ke = log(2.0) / 5.0;
    ka = log(2.0) / 0.25;
    return (float)(95 * ka / (ka - ke) * (exp(-ke * t) - exp(-ka * t)));
}

static void gallery_xml_caffeine(cui_xml *doc) {
    static bool ready;
    static char big[16], peak_mg[16], peak_time[16], peak_text[32], bed_text[32], dot_text[48], peak_at[24];
    static char detail[32], cups[8], curves[3][1400];
    static bool over;
    if (!ready) {
        static const double at_cup[3] = { 8, 10.75, 12.25 };
        double peak = 0, peak_hour = 6, bed = 0;
        int c, k;
        for (k = 0; k < 97; k++) {
            double hour = 6 + 24.0 * k / 96, total = 0;
            for (c = 0; c < 3; c++) {
                float dose = gallery_xml_dose(hour - at_cup[c]);
                char one[24];
                total += dose;
                snprintf(one, sizeof one, k ? ",%.3f" : "%.3f", (double)dose);
                if (strlen(curves[c]) + strlen(one) + 1 < sizeof curves[c]) strcat(curves[c], one);
            }
            if (total > peak) {
                peak = total;
                peak_hour = hour;
            }
        }
        for (c = 0; c < 3; c++) bed += gallery_xml_dose(23 - at_cup[c]);
        over = bed > 50;
        snprintf(big, sizeof big, "%.0f", bed);
        snprintf(peak_mg, sizeof peak_mg, "%.0f", peak);
        snprintf(peak_time, sizeof peak_time, "%02d:%02d", (int)peak_hour,
                 (int)lround((peak_hour - (int)peak_hour) * 60) % 60);
        snprintf(peak_at, sizeof peak_at, "%.6f", peak_hour);
        snprintf(peak_text, sizeof peak_text, "PEAK %.0f MG", peak);
        snprintf(bed_text, sizeof bed_text, "%.0f MG STILL IN YOU", bed);
        snprintf(dot_text, sizeof dot_text, "%s", over ? "OVER THE LINE AT BEDTIME" : "UNDER THE LINE AT BEDTIME");
        snprintf(detail, sizeof detail, "%s", over ? "OVER THE 50 MG LINE" : "UNDER THE 50 MG LINE");
        snprintf(cups, sizeof cups, "3");
        ready = true;
    }
    cui_xml_set_var(doc, "caffeineBig", big);
    cui_xml_set_var(doc, "caffeinePeak", peak_mg);
    cui_xml_set_var(doc, "caffeinePeakTime", peak_time);
    cui_xml_set_var(doc, "caffeinePeakText", peak_text);
    cui_xml_set_var(doc, "caffeinePeakAt", peak_at);
    cui_xml_set_var(doc, "caffeineBedText", bed_text);
    cui_xml_set_var(doc, "caffeineDot", dot_text);
    cui_xml_set_var(doc, "caffeineDetail", detail);
    cui_xml_set_var(doc, "caffeineCups", cups);
    cui_xml_set_var(doc, "caffeineOver", over ? "1" : "0");
    cui_xml_set_var(doc, "caffeineCurve1", curves[0]);
    cui_xml_set_var(doc, "caffeineCurve2", curves[1]);
    cui_xml_set_var(doc, "caffeineCurve3", curves[2]);
}

/* The level meters of the Audio page move with the frame they are drawn in, as
 * examples/gallery_more.c draws them; a document holds text, so the numbers are
 * computed here and handed over as values. */
static void gallery_xml_audio(cui_xml *doc) {
    static const char *const names[6] = { "audioMeter1", "audioMeter2", "audioMeter3",
                                          "audioMeter4", "audioMeter5", "audioMeter6" };
    float level[6][2], master[2];
    int k, c;
    g_audio_frame++;
    for (k = 0; k < 6; k++) {
        char text[40];
        for (c = 0; c < 2; c++) {
            float t = (float)(((double)g_audio_frame + c * 5) / 60.0);
            float beat = fmodf(t * (1.8f + 0.2f * k) + k * 0.31f, 1.0f);
            level[k][c] = (0.3f + 0.1f * k) * (0.2f + 0.8f * expf(-beat * (2.5f + k)));
        }
        snprintf(text, sizeof text, "%.9g,%.9g", (double)level[k][0], (double)level[k][1]);
        cui_xml_set_var(doc, names[k], text);
    }
    master[0] = level[0][0] * 1.4f;
    master[1] = level[4][1] * 1.3f;
    {
        char text[40];
        snprintf(text, sizeof text, "%.9g,%.9g", (double)master[0], (double)master[1]);
        cui_xml_set_var(doc, "audioMaster", text);
    }
}

/* The waveform of the Motion page: peaks made from two sines, as
 * examples/gallery_more.c makes them, handed to the document as values. */
static void gallery_xml_motion(cui_xml *doc) {
    static char peaks_text[96 * 24];
    static bool ready;
    if (!ready) {
        int k;
        size_t at = 0;
        for (k = 0; k < 96; k++) {
            float peak = 0.18f + 0.8f * fabsf(sinf(k * 0.37f) * sinf(k * 0.11f + 1.3f));
            int n = snprintf(peaks_text + at, sizeof peaks_text - at, k ? ",%.9g" : "%.9g", (double)peak);
            if (n < 0 || (size_t)n >= sizeof peaks_text - at) break;
            at += (size_t)n;
        }
        ready = true;
    }
    cui_xml_set_var(doc, "wavePeaks", peaks_text);
}

/* The charts of the More charts page need numbers a document cannot work out:
 * a week of traffic, a year of activity and the scatter's points, computed here
 * exactly as examples/gallery_more.c computes them. */
static void gallery_xml_viz(cui_xml *doc) {
    static char traffic[7 * 24 * 12], days[364 * 12], scatter_a[40 * 24], scatter_b[40 * 24];
    static char sizes_a[40 * 12], sizes_b[40 * 12];
    static bool ready;
    if (!ready) {
        size_t at;
        int d, h, k;
        unsigned s = 7;
        at = 0;
        for (d = 0; d < 7; d++)
            for (h = 0; h < 24; h++) {
                float work = d < 5 ? 1.0f : 0.45f;
                float day = expf(-powf((h - 13.5f) / 4.2f, 2)) * 90 * work +
                            expf(-powf((h - 20.5f) / 2.0f, 2)) * 35;
                float value = day + (float)((d * 31 + h * 17) % 11);
                int n = snprintf(traffic + at, sizeof traffic - at, at ? ",%.9g" : "%.9g", (double)value);
                if (n < 0 || (size_t)n >= sizeof traffic - at) break;
                at += (size_t)n;
            }
        at = 0;
        for (k = 0; k < 364; k++) {
            float base, r, value;
            int wd, n;
            s = s * 1103515245u + 12345u;
            wd = (k + ((cui_date_to_days(cui_date_today()) - 363) % 7 + 10) % 7) % 7;
            base = (wd >= 5 ? 0.6f : 3.2f) * (0.6f + 0.4f * sinf(k / 40.0f) * sinf(k / 40.0f));
            r = (float)((s >> 16) % 1000) / 1000.0f;
            value = r < 0.18f ? 0 : floorf(base * r * 4);
            n = snprintf(days + at, sizeof days - at, at ? ",%.9g" : "%.9g", (double)value);
            if (n < 0 || (size_t)n >= sizeof days - at) break;
            at += (size_t)n;
        }
        at = 0;
        for (k = 0; k < 40; k++) {
            float n = 200 + k * 95.0f;
            float ax = n, ay = 0.08f + n * 0.00006f + (float)((k * 37) % 13) * 0.004f;
            float bx = n * 0.9f + 150, by = 0.15f + n * 0.00011f + (float)((k * 53) % 17) * 0.006f;
            int written = snprintf(scatter_a + at, sizeof scatter_a - at, at ? ";%.9g,%.9g" : "%.9g,%.9g", (double)ax,
                                   (double)ay);
            if (written < 0 || (size_t)written >= sizeof scatter_a - at) break;
            at += (size_t)written;
            written = snprintf(scatter_b + at, sizeof scatter_b - at, at ? ";%.9g,%.9g" : "%.9g,%.9g", (double)bx,
                               (double)by);
            if (written < 0 || (size_t)written >= sizeof scatter_b - at) break;
            at += (size_t)written;
        }
        at = 0;
        for (k = 0; k < 40; k++) {
            int n = snprintf(sizes_a + at, sizeof sizes_a - at, at ? ",%.9g" : "%.9g",
                             (double)(float)(10 + (k * 7) % 40));
            if (n < 0 || (size_t)n >= sizeof sizes_a - at) break;
            at += (size_t)n;
        }
        at = 0;
        for (k = 0; k < 40; k++) {
            int n = snprintf(sizes_b + at, sizeof sizes_b - at, at ? ",%.9g" : "%.9g",
                             (double)(float)(20 + (k * 11) % 50));
            if (n < 0 || (size_t)n >= sizeof sizes_b - at) break;
            at += (size_t)n;
        }
        ready = true;
    }
    cui_xml_set_var(doc, "traffic", traffic);
    cui_xml_set_var(doc, "activity", days);
    cui_xml_set_var(doc, "scatterA", scatter_a);
    cui_xml_set_var(doc, "scatterB", scatter_b);
    cui_xml_set_var(doc, "sizesA", sizes_a);
    cui_xml_set_var(doc, "sizesB", sizes_b);
}

/* The Voice page plays a made-up conversation: the host measures the same
 * "microphone" examples/gallery_more.c measures and hands the levels, the owners
 * and the transcript to the document as values. */
typedef struct { int who; float seconds; bool thinking; const char *text; } gallery_voice_turn;

static const gallery_voice_turn g_voice_script[6] = {
    { 0, 3.2f, false, "What's the weather like tomorrow morning?" }, { 1, 1.4f, true, NULL },
    { 1, 4.2f, false, "Clear and cool, about 9 degrees at eight, warming to 15 by noon." },
    { 0, 2.4f, false, "Should I bring a jacket?" }, { 1, 1.1f, true, NULL },
    { 1, 3.0f, false, "A light one, yes. It's breezy until around ten." },
};

static float gallery_voice_noise(float t, float s) {
    return (sinf(t * 1.7f + s) * 0.5f + sinf(t * 3.1f + s * 2.3f) * 0.3f + sinf(t * 7.3f + s * 0.7f) * 0.2f) * 0.5f +
           0.5f;
}

static float gallery_voice_level(float t, float s) {
    return (sinf(t * 2.6f + s) > -0.45f ? 1.0f : 0.06f) * (0.28f + gallery_voice_noise(t * 2.4f, s) * 0.72f);
}

static int gallery_voice_turn_at(float t, float *into) {
    float total = 0;
    int k;
    for (k = 0; k < 6; k++) total += g_voice_script[k].seconds;
    t = fmodf(t, total);
    for (k = 0; k < 6; k++) {
        if (t < g_voice_script[k].seconds) {
            *into = t;
            return k;
        }
        t -= g_voice_script[k].seconds;
    }
    *into = 0;
    return 0;
}

static int g_voice_frame;
static double g_voice_time;
static double g_voice_next;
static float g_voice_call[240];
static unsigned char g_voice_who[240];
static int g_voice_n;

static void gallery_xml_voice_reset(void) { g_voice_frame = 0; g_voice_time = 0; g_voice_next = 0; g_voice_n = 0; }
static char g_voice_levels[240 * 14], g_voice_owners[240 * 3];
static char g_voice_spoken[160], g_voice_upcoming[160];
static char g_voice_state[16];

static void gallery_xml_voice(cui_xml *doc) {
    double now;
    g_voice_frame++;
    /* the same clock the C side reads: the frame's delta, added up */
    g_voice_time += (double)(1.0f / 60.0f);
    now = g_voice_time;
    char *at = g_voice_levels, *who = g_voice_owners;
    int k, i;
    while (g_voice_next <= now) {
        float t = (float)g_voice_next, into;
        const gallery_voice_turn *turn;
        int index = gallery_voice_turn_at(t, &into);
        turn = &g_voice_script[index];
        if (g_voice_n == 240) {
            memmove(g_voice_call, g_voice_call + 1, sizeof g_voice_call - sizeof g_voice_call[0]);
            memmove(g_voice_who, g_voice_who + 1, sizeof g_voice_who - 1);
            g_voice_n--;
        }
        g_voice_who[g_voice_n] = (unsigned char)turn->who;
        g_voice_call[g_voice_n++] = turn->thinking ? -1.0f
                                     : turn->who == 0 ? 0.8f * gallery_voice_level(t * 1.3f, 2)
                                                      : gallery_voice_level(t * 1.4f, 5);
        g_voice_next += 1.0 / 20.0;
    }
    if (g_voice_next - now > 1) g_voice_next = now;
    for (i = 0; i < g_voice_n; i++) {
        at += sprintf(at, i ? ",%.9g" : "%.9g", (double)g_voice_call[i]);
        who += sprintf(who, i ? ",%d" : "%d", (int)g_voice_who[i]);
    }
    if (!g_voice_n) {
        strcpy(g_voice_levels, "-1");
        strcpy(g_voice_owners, "0");
    }
    /* the transcript of the turn being spoken */
    {
        float into;
        int index = gallery_voice_turn_at((float)now, &into);
        const gallery_voice_turn *cur = &g_voice_script[index];
        const gallery_voice_turn *said = cur->thinking ? &g_voice_script[index - 1] : cur;
        float reveal = cur->thinking ? 1.0f : into / cur->seconds * 1.15f;
        int len = said->text ? (int)strlen(said->text) : 0;
        int shown = reveal >= 1 ? len : (int)(len * reveal);
        snprintf(g_voice_spoken, sizeof g_voice_spoken, "%.*s", shown, said->text ? said->text : "");
        snprintf(g_voice_upcoming, sizeof g_voice_upcoming, "%s", said->text ? said->text + shown : "");
        snprintf(g_voice_state, sizeof g_voice_state, "%s",
                 cur->thinking ? "thinking" : cur->who == 1 ? "speaking" : "listening");
        cui_xml_set_var(doc, "voiceReply", cur->thinking || said->who == 1 ? "1" : "0");
    }
    cui_xml_set_var(doc, "voiceLevels", g_voice_levels);
    cui_xml_set_var(doc, "voiceOwners", g_voice_owners);
    cui_xml_set_var(doc, "voiceTranscript", g_voice_spoken);
    cui_xml_set_var(doc, "voiceUpcoming", g_voice_upcoming);
    cui_xml_set_var(doc, "voiceState", g_voice_state);
    {
        static char message_levels[64 * 14], clip[160 * 14];
        static bool ready;
        if (!ready) {
            char *p = message_levels, *q = clip;
            for (i = 0; i < 64; i++) {
                float level = (0.2f + gallery_voice_noise(i * 0.3f, 0.4f) * 0.8f) *
                              (sinf(i * 0.45f + 0.4f) > -0.7f ? 1.0f : 0.15f);
                p += sprintf(p, i ? ",%.9g" : "%.9g", (double)level);
            }
            for (i = 0; i < 160; i++) {
                float level = (0.2f + gallery_voice_noise(i * 0.22f, 1.3f) * 0.8f) *
                              (sinf(i * 0.12f + 1) > -0.75f ? 1.0f : 0.1f);
                q += sprintf(q, i ? ",%.9g" : "%.9g", (double)level);
            }
            ready = true;
        }
        cui_xml_set_var(doc, "voiceMessage", message_levels);
        cui_xml_set_var(doc, "voiceClip", clip);
    }
    (void)k;
}

void gallery_xml_values(cui_xml *doc, gallery *g) {
    char text[24];
    int page = g->page;
    snprintf(text, sizeof text, "v%d.%d", CUI_VERSION_MAJOR, CUI_VERSION_MINOR);
    cui_xml_set_var(doc, "version", text);
    cui_xml_set_var(doc, "pageTitle", page >= 0 && page < gallery_page_total() ? gallery_page_name(page) : "");
    /* the theme segmented shows the gallery's theme, whatever set it */
    g_mode = g->dark ? 1 : 0;
    gallery_xml_caffeine(doc);
    gallery_xml_audio(doc);
    gallery_xml_motion(doc);
    gallery_xml_viz(doc);
    gallery_xml_voice(doc);
    cui_xml_set_var(doc, "peopleNote", gallery_more_people_note());
    cui_xml_set_var(doc, "chosenPath", gallery_more_chosen_path());
    cui_xml_set_var(doc, "hasChosen", gallery_more_chosen_path()[0] ? "1" : "");
}

void gallery_xml_apply_theme(cui_ctx *ui, gallery *g) {
    int want = (int)(g->dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
    if (g_theme_set != want) {
        cui_set_theme(ui, (cui_theme_mode)want);
        g_theme_set = want;
    }
}
