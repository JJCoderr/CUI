#define _CRT_SECURE_NO_WARNINGS 1

/* Tempo, the music player of examples/music.c, with its interface in
 * examples/music.xml.
 *
 * The document says where everything goes and what it says; this file keeps
 * what a document cannot: the track table, the screen-printed cover art, the
 * playing equalizer, and the actions. Those three pieces are tags of their own
 * (Cover, Eq, Hover), registered on the document, so the .xml stays the place
 * where the interface lives.
 *
 *     musicxml                    opens the window
 *     musicxml --shot out.png     draws it offscreen with the software renderer
 *     musicxml other.xml          reads another document
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_MUSIC_XML_PATH
#  define CUI_MUSIC_XML_PATH "music.xml"
#endif

/* ---- the data the document shows ---------------------------------------- */

typedef struct { const char *title, *artist, *album; int seconds; int art; } track;

static const track g_tracks[] = {
    { "Low Tide", "Marlow Avenue", "Night Shift", 222, 0 },
    { "Static Bloom", "Velvet Circuit", "Soft Machines", 247, 1 },
    { "Low Orbit", "Kaito Mori", "Satellites", 198, 2 },
    { "Paper Lanterns", "The Quiet Hours", "Ember", 263, 3 },
    { "Glass Harbor", "Ilse Varga", "Tidal", 231, 4 },
    { "After Hours", "Hollis & Grey", "Night Shift", 214, 0 },
    { "Signal / Noise", "Pale Static", "Frequencies", 276, 5 },
};
#define TRACK_COUNT ((int)(sizeof g_tracks / sizeof g_tracks[0]))

/* ---- the app's values: the document binds them --------------------------- */

static int g_playing, g_nav, g_playlist;
static bool g_paused, g_shuffle;
static bool g_liked[TRACK_COUNT] = { true, false, true };
static float g_position = 74, g_volume = 70;
static char g_search[64];
static double g_time;
static cui_ctx *g_ui;

/* ---- cover art, as in examples/music.c ---------------------------------- */

typedef struct { unsigned bg, a, b; } palette;
static const palette palettes[] = {
    { 0xc8553d, 0xf2e8cf, 0x2d3047 },   /* terracotta, cream, ink */
    { 0x2d3047, 0xe0a458, 0xf2e8cf },   /* ink, ochre, cream */
    { 0x5b8e7d, 0xf2e8cf, 0x2d3047 },   /* sage */
    { 0xe0a458, 0x2d3047, 0xc8553d },   /* ochre */
    { 0x1f3a5f, 0x8fb8de, 0xf2e8cf },   /* navy, sky */
    { 0xf2e8cf, 0xc8553d, 0x2d3047 },   /* cream */
};

typedef struct { int art; float radius; bool icon; } art_paint;

static void paint_art(cui_ctx *ui, cui_rect r, void *user) {
    art_paint *a = user;
    const palette *p = &palettes[a->art % 6];
    cui_color bg = CUI_RGB(p->bg), ca = CUI_RGB(p->a), cb = CUI_RGB(p->b);
    float s = r.w;
    cui_draw_rect(ui, r, CUI_CORNERS(a->radius), bg);
    cui_push_clip(ui, r);
    switch (a->art % 6) {
    case 0:   /* a low sun and a small moon */
        cui_draw_circle(ui, (cui_vec2){ r.x + s * 0.62f, r.y + s * 0.70f }, s * 0.36f, ca);
        cui_draw_circle(ui, (cui_vec2){ r.x + s * 0.24f, r.y + s * 0.26f }, s * 0.09f, cb);
        break;
    case 1:   /* three bands */
        for (int k = 0; k < 3; k++)
            cui_draw_rect(ui, (cui_rect){ r.x, r.y + s * (0.22f + k * 0.2f), s, s * 0.09f },
                          CUI_CORNERS(0), k == 1 ? cb : ca);
        break;
    case 2:   /* a quarter disc in the corner, a bar across */
        cui_draw_circle(ui, (cui_vec2){ r.x, r.y + s }, s * 0.62f, ca);
        cui_draw_rect(ui, (cui_rect){ r.x + s * 0.5f, r.y + s * 0.18f, s * 0.34f, s * 0.08f },
                      CUI_CORNERS(0), cb);
        break;
    case 3:   /* rings */
        for (int k = 3; k >= 1; k--)
            cui_draw_arc(ui, (cui_vec2){ r.x + s * 0.5f, r.y + s * 0.5f }, s * 0.12f * (float)k, 0,
                         6.2831853f, s * 0.035f, k == 2 ? cb : ca);
        break;
    case 4:   /* a grid of dots, one out of place */
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++)
                cui_draw_circle(ui, (cui_vec2){ r.x + s * (0.2f + x * 0.2f), r.y + s * (0.2f + y * 0.2f) },
                                s * 0.045f, (x == 2 && y == 1) ? cb : ca);
        break;
    default:  /* half a disc on a line */
        cui_draw_circle(ui, (cui_vec2){ r.x + s * 0.5f, r.y + s * 0.58f }, s * 0.3f, ca);
        cui_draw_rect(ui, (cui_rect){ r.x, r.y + s * 0.58f, s, s * 0.42f }, CUI_CORNERS(0), bg);
        cui_draw_rect(ui, (cui_rect){ r.x + s * 0.1f, r.y + s * 0.58f, s * 0.8f, s * 0.025f },
                      CUI_CORNERS(0), cb);
        break;
    }
    cui_pop_clip(ui);
    if (a->icon) cui_draw_icon(ui, CUI_ICON_MUSIC, r, s * 0.3f, cui_alpha(cb, 0.9f));
}

/* ---- the playing equalizer ---------------------------------------------- */

typedef struct { float t; } eq_paint;

static void paint_eq(cui_ctx *ui, cui_rect r, void *user) {
    eq_paint *e = user;
    for (int i = 0; i < 3; i++) {
        float h = 4 + 8 * (0.5f + 0.5f * sinf(e->t * 6 + i * 1.9f));
        cui_draw_rect(ui, (cui_rect){ r.x + 2 + i * 4.5f, r.y + r.h - 2 - h, 3, h }, CUI_CORNERS(1),
                      CUI_COLOR_ACCENT);
    }
}

/* ---- the row that lights up --------------------------------------------- */

typedef struct { bool hovered, selected; } row_bg;

static void paint_row(cui_ctx *ui, cui_rect r, void *user) {
    row_bg *b = user;
    if (b->selected) cui_draw_rect(ui, r, CUI_CORNERS(10), cui_token_color(ui, CUI_TOKEN_ACTIVE));
    else if (b->hovered) cui_draw_rect(ui, r, CUI_CORNERS(10), cui_token_color(ui, CUI_TOKEN_HOVER));
}

/* ---- attributes, as the tags read them ---------------------------------- */

static float attr_num(cui_xml *doc, cui_xml_node *node, const char *name, float fallback) {
    const char *v = cui_xml_node_attr_value(doc, node, name);
    return *v ? (float)atof(v) : fallback;
}

static cui_size attr_size(cui_xml *doc, cui_xml_node *node, const char *name, cui_size fallback) {
    const char *v = cui_xml_node_attr_value(doc, node, name);
    if (!*v) return fallback;
    if (!strcmp(v, "fill")) return CUI_FILL;
    if (!strcmp(v, "fit")) return CUI_FIT;
    if (strchr(v, '%')) return CUI_PCT((float)atof(v));
    return CUI_PX((float)atof(v));
}

static bool attr_yes(cui_xml *doc, cui_xml_node *node, const char *name) {
    const char *v = cui_xml_node_attr_value(doc, node, name);
    return !strcmp(v, "1") || !strcmp(v, "true") || !strcmp(v, "yes");
}

/* ---- <Cover Art="0" Size="196" Radius="16"/> ---------------------------- */

static bool tag_cover(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    art_paint a;
    cui_size width;
    (void)user;
    a.art = (int)attr_num(doc, node, "art", 0);
    a.radius = attr_num(doc, node, "radius", 0);
    a.icon = false;
    width = attr_size(doc, node, "width", CUI_PX(attr_num(doc, node, "size", 64)));
    {
        cui_size height = attr_size(doc, node, "height", width);
        cui_canvas(ui, cui_xml_node_name(node), width, height, paint_art, &a, sizeof a);
    }
    return true;
}

/* ---- <Eq Size="18x16"/> ------------------------------------------------- */

static bool tag_eq(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    const char *size = cui_xml_node_attr_value(doc, node, "size");
    eq_paint e;
    float w = 18, h = 16;
    (void)user;
    if (*size) sscanf(size, "%fx%f", &w, &h);
    e.t = (float)cui_time(ui);
    cui_canvas(ui, cui_xml_node_name(node), CUI_PX(w), CUI_PX(h), paint_eq, &e, sizeof e);
    return true;
}

/* ---- <Hover Height="56" Selected="{...}" OnClicked="play:0"> ... </Hover> */

static bool tag_hover(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_box box;
    bool *hover;
    row_bg bg;
    cui_canvas_result result;
    const char *action;
    (void)user;
    memset(&box, 0, sizeof box);
    box.id = cui_xml_node_name(node);
    box.dir = CUI_STACK;
    box.width = attr_size(doc, node, "width", CUI_FILL);
    box.height = attr_size(doc, node, "height", CUI_PX(52));
    cui_box_begin(ui, &box);
    hover = cui_state(ui, cui_make_id(ui, "hover"), sizeof(bool));
    bg.hovered = *hover;
    bg.selected = attr_yes(doc, node, "selected");
    result = cui_canvas(ui, "bg", CUI_FILL, CUI_FILL, paint_row, &bg, sizeof bg);
    *hover = result.hovered;
    cui_xml_draw_children(doc, node, ui);
    cui_box_end(ui);
    if (result.clicked) {
        action = cui_xml_node_attr_value(doc, node, "onclicked");
        if (*action) cui_xml_fire(doc, action);
    }
    return true;
}

/* ---- actions ------------------------------------------------------------ */

static void on_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    if (!strncmp(action, "play:", 5)) {
        int i = atoi(action + 5);
        if (i >= 0 && i < TRACK_COUNT) {
            g_playing = i;
            g_position = 0;
            g_paused = false;
        }
    } else if (!strncmp(action, "playlist:", 9)) {
        g_playlist = atoi(action + 9);
    } else if (!strncmp(action, "nav:", 4)) {
        g_nav = atoi(action + 4);
    } else if (!strncmp(action, "like:", 5)) {
        int i = atoi(action + 5);
        if (i >= 0 && i < TRACK_COUNT) g_liked[i] = !g_liked[i];
    } else if (!strcmp(action, "like.now")) {
        g_liked[g_playing] = !g_liked[g_playing];
    } else if (!strcmp(action, "player.pause")) {
        g_paused = !g_paused;
    } else if (!strcmp(action, "player.shuffle")) {
        g_shuffle = !g_shuffle;
    } else if (!strcmp(action, "player.next")) {
        g_playing = (g_playing + 1) % TRACK_COUNT;
        g_position = 0;
    } else if (!strcmp(action, "player.prev")) {
        g_playing = (g_playing + TRACK_COUNT - 1) % TRACK_COUNT;
        g_position = 0;
    }
}

/* ---- the values the document shows as text ------------------------------ */

static void fmt_time(char *out, size_t cap, int seconds) {
    snprintf(out, cap, "%d:%02d", seconds / 60, seconds % 60);
}

static void update_values(cui_xml *doc) {
    char text[32];
    const track *t = &g_tracks[g_playing];
    cui_xml_set_var(doc, "now.title", t->title);
    cui_xml_set_var(doc, "now.artist", t->artist);
    snprintf(text, sizeof text, "%d", t->art);
    cui_xml_set_var(doc, "now.art", text);
    fmt_time(text, sizeof text, (int)g_position);
    cui_xml_set_var(doc, "now.pos", text);
    snprintf(text, sizeof text, "%d", t->seconds);
    cui_xml_set_var(doc, "now.len", text);
    fmt_time(text, sizeof text, t->seconds);
    cui_xml_set_var(doc, "now.lenText", text);
    cui_xml_set_var(doc, "likedNow", g_liked[g_playing] ? "1" : "0");
}

/* ---- wiring ------------------------------------------------------------- */

static cui_xml *open_document(const char *path) {
    cui_xml *doc = cui_xml_load(path);
    if (!doc) return NULL;
    cui_xml_bind_int(doc, "playing", &g_playing);
    cui_xml_bind_int(doc, "nav", &g_nav);
    cui_xml_bind_int(doc, "playlist", &g_playlist);
    cui_xml_bind_bool(doc, "paused", &g_paused);
    cui_xml_bind_bool(doc, "shuffle", &g_shuffle);
    cui_xml_bind_float(doc, "position", &g_position);
    cui_xml_bind_float(doc, "volume", &g_volume);
    cui_xml_bind_text(doc, "search", g_search, sizeof g_search);
    cui_xml_bind_bool(doc, "liked0", &g_liked[0]);
    cui_xml_bind_bool(doc, "liked1", &g_liked[1]);
    cui_xml_bind_bool(doc, "liked2", &g_liked[2]);
    cui_xml_bind_bool(doc, "liked3", &g_liked[3]);
    cui_xml_bind_bool(doc, "liked4", &g_liked[4]);
    cui_xml_bind_bool(doc, "liked5", &g_liked[5]);
    cui_xml_bind_bool(doc, "liked6", &g_liked[6]);
    cui_xml_on_any(doc, on_action, NULL);
    cui_xml_register_tag(doc, "Cover", tag_cover, NULL);
    cui_xml_register_tag(doc, "Eq", tag_eq, NULL);
    cui_xml_register_tag(doc, "Hover", tag_hover, NULL);
    return doc;
}

/* One frame of the clock and the transport, as examples/music.c keeps it. */
static void tick(cui_ctx *ui) {
    double now = cui_time(ui);
    float dt = g_time > 0 ? (float)(now - g_time) : 0;
    if (dt > 0.25f) dt = 0.25f;
    g_time = now;
    if (g_paused) return;
    g_position += dt;
    if (g_position >= g_tracks[g_playing].seconds) {
        g_playing = (g_playing + 1) % TRACK_COUNT;
        g_position = 0;
    }
    cui_request_redraw(ui);
}

/* The frame the app draws, as cui_app_run calls it. */
static void music_xml_frame(cui_ctx *ui, void *user) {
    cui_xml *doc = user;
    g_ui = ui;
    tick(ui);
    update_values(doc);
    cui_xml_render(doc, ui);
    if (cui_xml_quit_requested(doc)) cui_app_quit(NULL);
}

static int write_shot(cui_xml *doc, const char *out_path) {
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui;
    const cui_draw_data *draw;
    unsigned char *pixels;
    int width = 0, height = 0, i;
    if (!sw) return 1;
    {
        /* the theme of the document from the start, so the shot is the window
         * the app would open: a theme change animates, and a picture of the
         * middle of that transition shows neither theme. */
        cui_app_desc d = cui_xml_app_desc(doc);
        ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = d.theme,
                                         .accent = d.accent });
    }
    if (!ui) {
        cui_sw_destroy(sw);
        return 1;
    }
    cui_xml_window_size(doc, &width, &height);
    cui_set_viewport(ui, (float)width, (float)height, 1);
    cui_input_mouse_leave(ui);
    for (i = 0; i < 6; i++) {
        cui_begin_frame(ui, 1.0f / 60.0f);
        tick(ui);
        update_values(doc);
        cui_xml_render(doc, ui);
        cui_end_frame(ui);
    }
    draw = cui_get_draw_data(ui);
    pixels = malloc((size_t)draw->width_px * (size_t)draw->height_px * 4u);
    if (pixels) {
        cui_sw_render(sw, draw, pixels, draw->width_px, draw->height_px, draw->width_px * 4, CUI_SW_RGBA);
        cui_write_png(out_path, pixels, draw->width_px, draw->height_px, draw->width_px * 4);
        printf("%s: %d x %d\n", out_path, draw->width_px, draw->height_px);
        free(pixels);
    }
    cui_destroy(ui);
    cui_sw_destroy(sw);
    return pixels ? 0 : 1;
}

int main(int argc, char **argv) {
    const char *path = CUI_MUSIC_XML_PATH;
    const char *shot = NULL;
    cui_xml *doc;
    int arg = 1;

    if (argc > 2 && !strcmp(argv[1], "--shot")) {
        shot = argv[2];
        arg = 3;
    }
    if (argc > arg) path = argv[arg];

    doc = open_document(path);
    if (!doc) {
        fprintf(stderr, "%s: cannot be read\n", path);
        return 1;
    }
    if (!cui_xml_ok(doc)) fprintf(stderr, "%s\n", cui_xml_last_error(doc));
    if (shot) {
        int rc = write_shot(doc, shot);
        cui_xml_free(doc);
        return rc;
    }
    {
        cui_app_desc desc = cui_xml_app_desc(doc);
        int rc = cui_app_run(&desc, music_xml_frame, doc);
        cui_xml_free(doc);
        return rc;
    }
}
