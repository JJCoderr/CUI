#define _CRT_SECURE_NO_WARNINGS 1

/* examples/tracker.c, with its interface in examples/tracker.xml.
 *
 * The board is the app's data, as in the C example: the ten issues with their
 * key, title, owner, due date, status, priority and two labels, the selected
 * issue, the comment field and the subscribed bell. The document holds the
 * window, the sidebar, the list's header, its tabs, the detail pane's
 * properties, activity and comment row, and the two sheets; what a document
 * cannot say is here: <Issues>, the four groups of rows, because which group
 * holds which issue, the count beside a group and the painted marks of a row
 * are the app's data; <StatusGlyph> and <PriorityGlyph>, the two canvases the
 * detail pane shows, because a canvas is app paint; and <Drawer> and
 * <DetailSheet>, the two sheets a narrow window opens. The command palette is
 * drawn after the document: its commands are the app's data too.
 *
 *     trackerxml                    opens the window
 *     trackerxml --shot out.png     draws it offscreen, software renderer
 *     trackerxml other.xml          reads another document
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_TRACKER_XML_PATH
#  define CUI_TRACKER_XML_PATH "tracker.xml"
#endif

enum { TRK_BACKLOG, TRK_TODO, TRK_PROGRESS, TRK_REVIEW, TRK_DONE, TRK_STATUS_COUNT };
enum { TRK_PRI_NONE, TRK_PRI_LOW, TRK_PRI_MEDIUM, TRK_PRI_HIGH, TRK_PRI_URGENT };

/* ---- the issues the list shows, as examples/tracker.c has them ------------ */

typedef struct {
    const char *key, *title, *assignee, *date;
    int status, priority;
    const char *labels[2];
    cui_tone label_tones[2];
} tracker_issue;

static const tracker_issue g_issues[] = {
    { "ENG-214", "Text input drops the last character on fast typing", "Maya Okafor", "Nov 6", TRK_PROGRESS, TRK_PRI_URGENT, { "Bug", "Input" }, { CUI_TONE_DANGER, CUI_TONE_NEUTRAL } },
    { "ENG-209", "Spring animations for popover placement flips", "Jonas Berg", "Nov 8", TRK_PROGRESS, TRK_PRI_HIGH, { "Motion", NULL }, { CUI_TONE_VIOLET } },
    { "ENG-205", "Software renderer: SIMD path for text quads", "Lena Fischer", "Nov 12", TRK_PROGRESS, TRK_PRI_MEDIUM, { "Perf", NULL }, { CUI_TONE_TEAL } },
    { "ENG-198", "Keyboard navigation for the color picker", "Priya Nair", "Nov 7", TRK_REVIEW, TRK_PRI_HIGH, { "A11y", "Input" }, { CUI_TONE_INFO, CUI_TONE_NEUTRAL } },
    { "ENG-196", "Wayland backend through libdecor", "Tomas Rivera", "Nov 9", TRK_REVIEW, TRK_PRI_MEDIUM, { "Platform", NULL }, { CUI_TONE_ROSE } },
    { "ENG-221", "Inline IME composition on macOS", "Aiko Tanaka", "Nov 14", TRK_TODO, TRK_PRI_HIGH, { "Platform", "Input" }, { CUI_TONE_ROSE, CUI_TONE_NEUTRAL } },
    { "ENG-219", "Virtual table with sticky header", "Sam Whitfield", "Nov 18", TRK_TODO, TRK_PRI_MEDIUM, { "Data", NULL }, { CUI_TONE_TEAL } },
    { "ENG-217", "Docs: theming cookbook", "Maya Okafor", "Nov 20", TRK_TODO, TRK_PRI_LOW, { "Docs", NULL }, { CUI_TONE_NEUTRAL } },
    { "ENG-188", "Ship 1.0 release notes", "Jonas Berg", "Nov 1", TRK_DONE, TRK_PRI_MEDIUM, { "Docs", NULL }, { CUI_TONE_NEUTRAL } },
    { "ENG-185", "Gliding highlight in segmented controls", "Priya Nair", "Oct 30", TRK_DONE, TRK_PRI_LOW, { "Motion", NULL }, { CUI_TONE_VIOLET } },
};
#define TRACKER_ISSUE_COUNT ((int)(sizeof g_issues / sizeof g_issues[0]))

static const char *const g_status_names[TRK_STATUS_COUNT] = { "Backlog", "Todo", "In progress", "In review", "Done" };
static const char *const g_priority_names[] = { "No priority", "Low", "Medium", "High", "Urgent" };

/* the names a document writes for a tone, in the order of cui_tone */
static const char *const g_tone_names[] = { "neutral", "info", "success", "warning", "danger",
                                            "violet", "teal", "rose", "accent" };

/* ---- the app's values: the document binds them --------------------------- */

typedef struct {
    int nav, tab, selected;
    bool palette, subscribed, drawer, sheet;
    int panes;              /* 3, 2 or 1, from the window width */
    char comment[256];
} tracker_state;

static tracker_state g;

/* ---- the state `tracker --shot` draws ----------------------------------- */

/* The board the app opens with: the Engine team in the sidebar, the live issue
 * first, three panes. examples/tracker.c seeds the same values. */
static void seed(tracker_state *s) {
    memset(s, 0, sizeof *s);
    s->nav = 6;
}

/* ---- the painted marks: examples/tracker.c's glyphs --------------------- */

typedef struct { int status; } tracker_status_glyph;

static void tracker_paint_status(cui_ctx *ui, cui_rect r, void *user) {
    tracker_status_glyph *g = user;
    cui_vec2 c = { r.x + r.w / 2, r.y + r.h / 2 };
    float rad = 6.5f;
    const float tau = 6.2831853f;
    cui_color muted = cui_token_color(ui, CUI_TOKEN_TEXT_MUTED);
    cui_color warn = cui_token_color(ui, CUI_TOKEN_WARNING);
    cui_color accent = cui_token_color(ui, CUI_TOKEN_ACCENT);
    switch (g->status) {
    case TRK_BACKLOG:   /* dashed ring */
        for (int i = 0; i < 8; i++)
            cui_draw_arc(ui, c, rad, tau * i / 8 + 0.12f, tau * (i + 1) / 8 - 0.12f, 1.5f, muted);
        break;
    case TRK_TODO:
        cui_draw_arc(ui, c, rad, 0, tau, 1.5f, cui_token_color(ui, CUI_TOKEN_TEXT_SECONDARY));
        break;
    case TRK_PROGRESS:  /* ring with a half pie */
    case TRK_REVIEW: {
        cui_color col = g->status == TRK_PROGRESS ? warn : accent;
        float frac = g->status == TRK_PROGRESS ? 0.5f : 0.75f;
        cui_draw_arc(ui, c, rad, 0, tau, 1.5f, col);
        cui_vec2 pts[34];
        int n = 0;
        pts[n++] = c;
        for (int i = 0; i <= 32; i++) {
            float a = -tau / 4 + tau * frac * i / 32;
            pts[n++] = (cui_vec2){ c.x + cosf(a) * (rad - 3), c.y + sinf(a) * (rad - 3) };
        }
        /* the pie is not convex for frac > 0.5: draw it as two halves */
        if (frac <= 0.5f) cui_draw_polygon(ui, pts, n, col);
        else {
            cui_draw_polygon(ui, pts, 18, col);
            cui_vec2 rest[18];
            rest[0] = c;
            for (int i = 0; i < 17; i++) rest[i + 1] = pts[17 + i];
            cui_draw_polygon(ui, rest, 18, col);
        }
        break;
    }
    case TRK_DONE:
        cui_draw_circle(ui, c, rad + 0.75f, accent);
        cui_draw_icon(ui, CUI_ICON_CHECK, r, 10, cui_token_color(ui, CUI_TOKEN_ACCENT_FOREGROUND));
        break;
    }
}

static void tracker_status_icon(cui_ctx *ui, const char *id, int status) {
    tracker_status_glyph glyph = { status };
    cui_canvas(ui, id, CUI_PX(16), CUI_PX(16), tracker_paint_status, &glyph, sizeof glyph);
}

typedef struct { int priority; } tracker_priority_glyph;

static void tracker_paint_priority(cui_ctx *ui, cui_rect r, void *user) {
    tracker_priority_glyph *g = user;
    cui_color on = cui_token_color(ui, CUI_TOKEN_TEXT_SECONDARY);
    cui_color off = cui_alpha(on, 0.3f);
    if (g->priority == TRK_PRI_URGENT) {
        cui_rect b = { r.x + 1, r.y + 1, r.w - 2, r.h - 2 };
        cui_draw_rect(ui, b, CUI_CORNERS(3.5f), cui_token_color(ui, CUI_TOKEN_WARNING));
        cui_draw_rect(ui, (cui_rect){ b.x + b.w / 2 - 0.9f, b.y + 3, 1.8f, 5.5f }, CUI_CORNERS(0.9f), CUI_RGB(0x1a1a1a));
        cui_draw_rect(ui, (cui_rect){ b.x + b.w / 2 - 0.9f, b.y + b.h - 4.6f, 1.8f, 1.8f }, CUI_CORNERS(0.9f), CUI_RGB(0x1a1a1a));
        return;
    }
    if (g->priority == TRK_PRI_NONE) {
        for (int i = 0; i < 3; i++)
            cui_draw_rect(ui, (cui_rect){ r.x + 2 + i * 4.5f, r.y + r.h / 2 - 0.75f, 3, 1.5f }, CUI_CORNERS(0.75f), off);
        return;
    }
    for (int i = 0; i < 3; i++) {
        float h = 4 + i * 3.5f;
        cui_draw_rect(ui, (cui_rect){ r.x + 2 + i * 4.5f, r.y + r.h - 2 - h, 3, h }, CUI_CORNERS(1), i < g->priority ? on : off);
    }
}

static void tracker_priority_icon(cui_ctx *ui, const char *id, int priority) {
    tracker_priority_glyph glyph = { priority };
    cui_canvas(ui, id, CUI_PX(16), CUI_PX(16), tracker_paint_priority, &glyph, sizeof glyph);
}

/* A full-width hover/selection background behind a row of components. */
typedef struct { bool hovered, selected; float radius; } tracker_row_bg;

static void tracker_paint_row(cui_ctx *ui, cui_rect r, void *user) {
    tracker_row_bg *b = user;
    if (b->selected) cui_draw_rect(ui, r, CUI_CORNERS(b->radius), cui_token_color(ui, CUI_TOKEN_ACTIVE));
    else if (b->hovered) cui_draw_rect(ui, r, CUI_CORNERS(b->radius), cui_token_color(ui, CUI_TOKEN_HOVER));
}

static bool tracker_row_background(cui_ctx *ui, const char *id, bool selected, float radius) {
    cui_id rid = cui_make_id(ui, id);
    bool *hover = cui_state(ui, rid, sizeof(bool));
    tracker_row_bg b = { *hover, selected, radius };
    cui_canvas_result res = cui_canvas(ui, id, CUI_FILL, CUI_FILL, tracker_paint_row, &b, sizeof b);
    *hover = res.hovered;
    return res.clicked;
}

/* ---- one issue: examples/tracker.c's issue_row -------------------------- */

static void tracker_issue_row(cui_ctx *ui, int i) {
    const tracker_issue *it = &g_issues[i];
    cui_push_id_int(ui, i);
    cui_box(ui, .dir = CUI_STACK, .width = CUI_FILL, .height = CUI_PX(44)) {
        if (tracker_row_background(ui, "bg", g.selected == i, 8)) {
            g.selected = i;
            g.sheet = g.panes < 3;
        }
        cui_row(ui, .width = CUI_FILL, .height = CUI_FILL, .pad_x = 12, .gap = 12, .align = CUI_ALIGN_CENTER) {
            tracker_priority_icon(ui, "prio", it->priority);
            cui_text(ui, it->key, .font = CUI_FONT_MONO, .size = 12, .color = CUI_COLOR_TEXT_MUTED, .width = CUI_PX(62), .nowrap = true);
            tracker_status_icon(ui, "status", it->status);
            cui_text(ui, it->title, .width = CUI_FILL, .nowrap = true);
            if (g.panes > 1) {   /* a phone keeps the title, the status and who has it */
                for (int l = 0; l < 2; l++)
                    if (it->labels[l]) cui_badge(ui, it->labels[l], .tone = it->label_tones[l]);
                cui_text(ui, it->date, .size = 12, .color = CUI_COLOR_TEXT_MUTED, .width = CUI_PX(46), .align = CUI_TEXT_RIGHT, .nowrap = true);
            }
            cui_avatar(ui, it->assignee, .size = 22);
        }
    }
    cui_pop_id(ui);
}

/* ---- small readers, as examples/mail_xml.c has them --------------------- */

static bool tracker_word_eq(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static float tracker_attr_num(cui_xml *doc, cui_xml_node *node, const char *name, float fallback) {
    const char *value = cui_xml_node_attr_value(doc, node, name);
    return value && *value ? (float)atof(value) : fallback;
}

/* ---- <Issues/>: the four groups of rows --------------------------------- */

/* Copied from examples/tracker.c rather than described: which group holds
 * which issue, the order of the groups, the count beside a group's name and
 * the two painted marks of a row are the app's data, and a row is a canvas
 * painted behind its text so it can show that it is selected. */
static bool tracker_tag_issues(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    static const int order[] = { TRK_PROGRESS, TRK_REVIEW, TRK_TODO, TRK_DONE };
    int g_idx, i;
    (void)doc;
    (void)node;
    (void)user;
    for (g_idx = 0; g_idx < 4; g_idx++) {
        int st = order[g_idx], count = 0;
        for (i = 0; i < TRACKER_ISSUE_COUNT; i++) count += g_issues[i].status == st;
        cui_push_id_int(ui, 100 + st);
        cui_row(ui, .width = CUI_FILL, .pad_x = 12, .pad_top = g_idx ? 14 : 4, .pad_bottom = 6, .gap = 10, .align = CUI_ALIGN_CENTER) {
            char n[8];
            tracker_status_icon(ui, "group", st);
            cui_text(ui, g_status_names[st], .font = CUI_FONT_MEDIUM, .size = 13, .nowrap = true);
            snprintf(n, sizeof n, "%d", count);
            cui_text(ui, n, .size = 13, .color = CUI_COLOR_TEXT_MUTED);
            cui_spacer(ui);
            cui_icon_button(ui, CUI_ICON_PLUS, .size = CUI_SIZE_SM, .tooltip = "Add issue");
        }
        cui_pop_id(ui);
        for (i = 0; i < TRACKER_ISSUE_COUNT; i++)
            if (g_issues[i].status == st) tracker_issue_row(ui, i);
    }
    return true;
}

/* ---- <StatusGlyph Status="0"/> and <PriorityGlyph Priority="4"/> -------- */

static bool tracker_tag_status_glyph(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)user;
    tracker_status_icon(ui, cui_xml_node_name(node), (int)tracker_attr_num(doc, node, "status", 0));
    return true;
}

static bool tracker_tag_priority_glyph(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)user;
    tracker_priority_icon(ui, cui_xml_node_name(node), (int)tracker_attr_num(doc, node, "priority", 0));
    return true;
}

/* ---- <Drawer Text="Orbit" Side="left" Size="280"> ----------------------- */

static bool tracker_tag_drawer(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    static const char *const sides[] = { "right", "left", "bottom", "top" };
    cui_sheet_opts o;
    char title[64];
    const char *side;
    int i, kind = CUI_SIDE_LEFT;
    (void)user;
    memset(&o, 0, sizeof o);
    o.size = tracker_attr_num(doc, node, "size", 400);
    side = cui_xml_node_attr_value(doc, node, "side");
    for (i = 0; i < 4; i++)
        if (side[0] && tracker_word_eq(side, sides[i])) kind = i;
    o.side = (cui_side)kind;
    snprintf(title, sizeof title, "%s", cui_xml_node_attr_value(doc, node, "text"));
    if (cui_sheet_begin(ui, title, &g.drawer, &o)) {
        cui_xml_draw_children(doc, node, ui);
        cui_sheet_end(ui);
    }
    return true;
}

/* ---- <DetailSheet>: the issue open in a sheet --------------------------- */

/* examples/tracker.c makes the sheet as wide as the window on a phone and 420
 * points otherwise, and calls it by the key of the open issue. */
static bool tracker_tag_detail_sheet(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_sheet_opts o;
    float vw = cui_viewport_size(ui).x;
    (void)user;
    memset(&o, 0, sizeof o);
    o.size = g.panes == 1 ? vw : 420;
    if (cui_sheet_begin(ui, g_issues[g.selected].key, &g.sheet, &o)) {
        cui_xml_draw_children(doc, node, ui);
        cui_sheet_end(ui);
    }
    return true;
}

/* ---- actions ------------------------------------------------------------ */

/* The sidebar's places set the page; the two groups above the teams also close
 * the drawer they were clicked in, as the C example does. */
static void tracker_on_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    if (!strcmp(action, "palette")) {
        g.palette = true;
    } else if (!strcmp(action, "drawer")) {
        g.drawer = true;
    } else if (!strcmp(action, "subscribe")) {
        g.subscribed = !g.subscribed;
    } else if (!strncmp(action, "nav:", 4)) {
        int i = atoi(action + 4);
        if (i >= 0 && i < 9) {
            g.nav = i;
            if (i < 6) g.drawer = false;
        }
    }
}

/* ---- the values the document shows as text ------------------------------ */

/* The open issue: what is left of the detail pane once its layout is in the
 * document. examples/tracker.c reads the same fields of the same row. */
static void detail_values(cui_xml *doc) {
    const tracker_issue *it;
    char number[32];
    if (g.selected < 0) g.selected = 0;
    if (g.selected >= TRACKER_ISSUE_COUNT) g.selected = TRACKER_ISSUE_COUNT - 1;
    it = &g_issues[g.selected];
    snprintf(number, sizeof number, "%d", it->status);
    cui_xml_set_var(doc, "issueStatus", number);
    snprintf(number, sizeof number, "%d", it->priority);
    cui_xml_set_var(doc, "issuePriority", number);
    cui_xml_set_var(doc, "issueStatusName", g_status_names[it->status]);
    cui_xml_set_var(doc, "issuePriorityName", g_priority_names[it->priority]);
    cui_xml_set_var(doc, "issueKey", it->key);
    cui_xml_set_var(doc, "issueTitle", it->title);
    cui_xml_set_var(doc, "issueAssignee", it->assignee);
    cui_xml_set_var(doc, "issueDate", it->date);
    cui_xml_set_var(doc, "issueLabel1", it->labels[0] ? it->labels[0] : "");
    cui_xml_set_var(doc, "issueLabel1Tone", g_tone_names[it->label_tones[0]]);
    cui_xml_set_var(doc, "issueLabel2", it->labels[1] ? it->labels[1] : "");
    cui_xml_set_var(doc, "issueLabel2Tone", g_tone_names[it->label_tones[1]]);
}

/* What a frame knows before the document is drawn: the pane count of the
 * window's width (and the drawer and the sheet that answer closes) and the
 * open issue. examples/tracker.c computes the same in its frame. */
static void values(cui_xml *doc, cui_ctx *ui) {
    float vw;
    if (!ui) return;
    vw = cui_viewport_size(ui).x;
    g.panes = vw < 760 ? 1 : vw < 1200 ? 2 : 3;
    if (g.panes > 1) g.drawer = false;
    if (g.panes == 3) g.sheet = false;
    cui_xml_set_var(doc, "wide", g.panes > 1 ? "1" : "0");
    cui_xml_set_var(doc, "phone", g.panes == 1 ? "1" : "0");
    cui_xml_set_var(doc, "three", g.panes == 3 ? "1" : "0");
    detail_values(doc);
}

/* ---- wiring ------------------------------------------------------------- */

static cui_xml *open_document(const char *path) {
    cui_xml *doc = cui_xml_load(path);
    if (!doc) return NULL;
    /* the fields the document holds, and the ones {"..."} reads */
    cui_xml_bind_int(doc, "nav", &g.nav);
    cui_xml_bind_int(doc, "tab", &g.tab);
    cui_xml_bind_bool(doc, "subscribed", &g.subscribed);
    cui_xml_bind_text(doc, "comment", g.comment, sizeof g.comment);
    cui_xml_set_var(doc, "wide", "1");
    cui_xml_set_var(doc, "phone", "0");
    cui_xml_set_var(doc, "three", "1");
    detail_values(doc);
    cui_xml_on_any(doc, tracker_on_action, NULL);
    cui_xml_register_tag(doc, "Issues", tracker_tag_issues, NULL);
    cui_xml_register_tag(doc, "StatusGlyph", tracker_tag_status_glyph, NULL);
    cui_xml_register_tag(doc, "PriorityGlyph", tracker_tag_priority_glyph, NULL);
    cui_xml_register_tag(doc, "Drawer", tracker_tag_drawer, NULL);
    cui_xml_register_tag(doc, "DetailSheet", tracker_tag_detail_sheet, NULL);
    return doc;
}

/* The commands of the palette are the app's data and a palette has no tag, so
 * it is drawn after the document, in the frame its shortcut opened it. */
static void palette(cui_ctx *ui) {
    static const cui_command cmds[] = {
        { "Create issue", CUI_ICON_PLUS, "Issues", "C" },
        { "Assign to me", CUI_ICON_USER, "Issues", "I" },
        { "Change status", CUI_ICON_CIRCLE_DOT, "Issues", "S" },
        { "Go to inbox", CUI_ICON_INBOX, "Navigation", "G I" },
        { "Go to cycles", CUI_ICON_REFRESH_CW, "Navigation", "G C" },
    };
    int c = cui_command_palette(ui, &g.palette, cmds, 5);
    if (c >= 0) cui_toast(ui, cmds[c].title, .tone = CUI_TONE_SUCCESS);
}

/* One frame: the values, the window and what floats over it. The tracker has
 * no clock of its own, so the shot and the app run this and nothing else. */
static void frame(cui_ctx *ui, cui_xml *doc) {
    values(doc, ui);
    cui_xml_render(doc, ui);
    palette(ui);
}

/* `trackerxml --shot out.png`: the same window drawn offscreen with the software
 * renderer, with the viewport, the theme, the accent and the six frames of
 * examples/tracker.c, so the two pictures can be compared. */
/* The frame the app draws, as cui_app_run calls it. */
static void tracker_xml_frame(cui_ctx *ui, void *user) {
    cui_xml *doc = user;
    frame(ui, doc);
    cui_xml_poll(doc);
    if (cui_xml_quit_requested(doc)) cui_app_quit(NULL);
}

static int write_shot(cui_xml *doc, const char *out_path) {
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui;
    const cui_draw_data *draw;
    unsigned char *pixels;
    int width = 0, height = 0, i;
    if (!sw) return 1;
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = CUI_THEME_DARK,
                                     .accent = CUI_ACCENT_NEUTRAL });
    if (!ui) {
        cui_sw_destroy(sw);
        return 1;
    }
    cui_xml_window_size(doc, &width, &height);
    cui_set_viewport(ui, (float)width, (float)height, 1);
    cui_input_mouse_leave(ui);
    for (i = 0; i < 6; i++) {
        cui_begin_frame(ui, 1.0f / 60.0f);
        frame(ui, doc);
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
    const char *path = CUI_TRACKER_XML_PATH;
    const char *shot = NULL;
    cui_xml *doc;
    int arg = 1;

    seed(&g);
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
        int rc = cui_app_run(&desc, tracker_xml_frame, doc);
        cui_xml_free(doc);
        return rc;
    }
}
