/* The gallery of examples/gallery_ui.c and the same gallery described in
 * examples/gallery.xml, drawn with one viewport, theme and accent, frame for
 * frame, then compared: this test says whether a document reproduces the C
 * interface pixel for pixel.
 *
 *   cui_xml_gallery_shots                  every page, both themes, and the
 *                                          accent variants the screenshot
 *                                          tool also draws
 *   cui_xml_gallery_shots out             also writes the pictures
 *   cui_xml_gallery_shots out 3           one page only
 *   cui_xml_gallery_shots out 3 dark      one theme only
 *
 * Exit code 1 when any page differs.
 */
#include <cui/cui.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include "../examples/gallery_ui.h"
#include "../examples/gallery_xml_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_GALLERY_XML_PATH
#  define CUI_GALLERY_XML_PATH "../examples/gallery.xml"
#endif

static const char *const slugs[PAGE_COUNT] = { "overview", "buttons", "inputs", "selection",
                                               "navigation", "overlays", "feedback", "data", "charts" };

/* A file name for every page, the extended ones included: "Actions" becomes
 * "actions", "More developer tools" "more-developer-tools". */
static void page_slug(int page, char *out, size_t cap) {
    if (page >= 0 && page < PAGE_COUNT) {
        snprintf(out, cap, "%s", slugs[page]);
        return;
    }
    {
        const char *name = gallery_page_name(page);
        size_t i = 0;
        for (; name[i] && i + 1 < cap; i++)
            out[i] = (name[i] >= 'A' && name[i] <= 'Z') ? (char)(name[i] + 32) : (name[i] == ' ' ? '-' : name[i]);
        out[i] = 0;
    }
}

/* How tall the window is when a page is drawn: the data page scrolls a long
 * table, the rest fit 1000 points. */
static float page_height(int page) { return page == PAGE_DATA ? 1500.0f : 1000.0f; }

/* The pages examples/gallery.xml describes so far. The extended pages of
 * examples/gallery_more.c are moving into it one at a time; add a page here
 * when its XML draws it, so `ctest` says which remain. */
static const int converted[] = { PAGE_OVERVIEW, PAGE_BUTTONS, PAGE_INPUTS, PAGE_SELECTION, PAGE_NAVIGATION,
                                 PAGE_OVERLAYS, PAGE_FEEDBACK, PAGE_DATA, PAGE_CHARTS, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29 };

typedef struct {
    cui_sw_renderer *sw;
    cui_ctx *ui;
} canvas;

static int g_doc_errors;   /* documents that reported a mistake while they drew */

static bool canvas_open(canvas *c, bool dark, int accent, float width, float height) {
    memset(c, 0, sizeof *c);
    c->sw = cui_sw_create();
    if (!c->sw) return false;
    c->ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(c->sw),
                                        .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT,
                                        .accent = (cui_accent)accent });
    if (!c->ui) {
        cui_sw_destroy(c->sw);
        return false;
    }
    cui_set_viewport(c->ui, width, height, 2);
    cui_input_mouse_leave(c->ui);
    return true;
}

static void canvas_close(canvas *c) {
    cui_destroy(c->ui);
    cui_sw_destroy(c->sw);
}

static void gallery_frames(canvas *c, cui_xml *doc, gallery *g, int count) {
    for (int f = 0; f < count; f++) {
        cui_begin_frame(c->ui, 1.0f / 60.0f);
        gallery_xml_render(doc, c->ui, g);
        cui_end_frame(c->ui);
    }
}

static bool gallery_click(canvas *c, cui_xml *doc, gallery *g, const char *name) {
    const cui_xml_node *node = cui_xml_find(doc, name);
    cui_rect r = cui_get_rect(c->ui, cui_xml_node_id(node));
    if (r.w <= 0 || r.h <= 0) return false;
    cui_input_mouse_move(c->ui, r.x + r.w * 0.5f, r.y + r.h * 0.5f);
    gallery_frames(c, doc, g, 1);
    cui_input_mouse_button(c->ui, CUI_MOUSE_LEFT, true);
    gallery_frames(c, doc, g, 1);
    cui_input_mouse_button(c->ui, CUI_MOUSE_LEFT, false);
    gallery_frames(c, doc, g, 2);
    return true;
}

static bool test_gallery_actions(void) {
    canvas c;
    gallery g;
    bool profile, drawer;
    cui_xml *doc;
    if (!canvas_open(&c, false, 0, 1240, 1000)) return false;
    gallery_init(&g);
    g.page = PAGE_OVERLAYS;
    doc = gallery_xml_open(CUI_GALLERY_XML_PATH, &g);
    if (!doc) {
        canvas_close(&c);
        return false;
    }
    gallery_frames(&c, doc, &g, 3);
    profile = gallery_click(&c, doc, &g, "profile-trigger") && g.dialog_open;
    g.dialog_open = false;
    cui_set_viewport(c.ui, 700, 900, 2);
    gallery_frames(&c, doc, &g, 30);
    drawer = gallery_click(&c, doc, &g, "components-menu") && g.drawer;
    cui_xml_free(doc);
    canvas_close(&c);
    if (!profile || !drawer)
        fprintf(stderr, "Gallery actions: edit profile %s, narrow drawer %s\n",
                profile ? "passed" : "failed", drawer ? "passed" : "failed");
    return profile && drawer;
}

/* Runs the frame loop and hands back the picture, RGBA, malloc'd. */
static unsigned char *canvas_pixels(canvas *c, int *width, int *height) {
    const cui_draw_data *dd = cui_get_draw_data(c->ui);
    unsigned char *pixels;
    *width = dd->width_px;
    *height = dd->height_px;
    pixels = malloc((size_t)*width * (size_t)*height * 4u);
    if (!pixels) return NULL;
    cui_sw_render(c->sw, dd, pixels, *width, *height, *width * 4, CUI_SW_RGBA);
    return pixels;
}

/* examples/gallery_ui.c, drawn by C. */
static unsigned char *draw_c(int page, bool dark, int accent, float width, float height, int *w, int *h) {
    canvas c;
    gallery g;
    int f;
    if (!canvas_open(&c, dark, accent, width, height)) return NULL;
    gallery_init(&g);
    g.page = page;
    g.dark = dark;
    g.accent = accent;
    for (f = 0; f < 90; f++) {
        cui_begin_frame(c.ui, 1.0f / 60.0f);
        gallery_frame(c.ui, &g);
        cui_end_frame(c.ui);
    }
    {
        unsigned char *pixels = canvas_pixels(&c, w, h);
        canvas_close(&c);
        return pixels;
    }
}

/* examples/gallery.xml, drawn by the document. A document of its own for every
 * shot: what an element holds (an open accordion, a scroll offset) must not
 * leak from the page drawn before it. */
static unsigned char *draw_xml(const char *path, int page, bool dark, int accent, float width, float height,
                               int *w, int *h) {
    canvas c;
    static gallery bound;
    cui_xml *doc;
    if (!canvas_open(&c, dark, accent, width, height)) return NULL;
    gallery_init(&bound);
    doc = gallery_xml_open(path, &bound);
    if (!doc) {
        canvas_close(&c);
        return NULL;
    }
    /* a document with a mistake still draws, so the test says so: an unknown tag
     * or an attribute that does not parse would otherwise pass as a picture */
    if (!cui_xml_ok(doc)) {
        g_doc_errors++;
        fprintf(stderr, "%s: %s\n", path, cui_xml_last_error(doc));
    }
    /* the document writes into `bound`, so it is the one that says which page
     * is drawn, what the theme is and which accent is chosen */
    bound.page = page;
    bound.dark = dark;
    bound.accent = accent;
    gallery_frames(&c, doc, &bound, 90);
    {
        unsigned char *pixels = canvas_pixels(&c, w, h);
        canvas_close(&c);
        cui_xml_free(doc);
        return pixels;
    }
}

static int write_png(const char *path, const unsigned char *pixels, int width, int height) {
    if (!cui_write_png(path, pixels, width, height, width * 4)) return 1;
    printf("  wrote %s\n", path);
    return 0;
}

/* One page, one theme, one accent: drawn twice, compared, reported. Returns
 * the number of differences (0 when the two pictures are the same bytes). */
static int compare(const char *path, const char *dir, const char *slug, int page, bool dark, int accent,
                   float width, float height) {
    int wc = 0, hc = 0, wx = 0, hx = 0;
    unsigned char *a, *b;
    int i, diff = 0, first = -1;
    char file[512];
    const char *theme = dark ? "dark" : "light";
    a = draw_c(page, dark, accent, width, height, &wc, &hc);
    b = draw_xml(path, page, dark, accent, width, height, &wx, &hx);
    if (!a || !b) {
        printf("%-10s %-8s %-5s: OUT OF MEMORY\n", slug, theme, "");
        free(a);
        free(b);
        return 1;
    }
    if (wc != wx || hc != hx) {
        printf("%-10s %-6s: size %dx%d against %dx%d\n", slug, theme, wc, hc, wx, hx);
        diff = 1;
    } else {
        for (i = 0; i < wc * hc * 4; i += 4) {
            if (a[i] != b[i] || a[i + 1] != b[i + 1] || a[i + 2] != b[i + 2] || a[i + 3] != b[i + 3]) {
                if (first < 0) first = i / 4;
                diff++;
            }
        }
    }
    if (diff && first >= 0) {
        printf("%-10s %-6s: %d pixels differ (first at %d,%d -> #%02x%02x%02x against #%02x%02x%02x)\n",
               slug, theme, diff, first % wc, first / wc,
               a[first * 4], a[first * 4 + 1], a[first * 4 + 2],
               b[first * 4], b[first * 4 + 1], b[first * 4 + 2]);
    } else {
        printf("%-10s %-6s: identical\n", slug, theme);
    }
    if (dir) {
        snprintf(file, sizeof file, "%s/gallery-%s-%s-c.png", dir, slug, theme);
        write_png(file, a, wc, hc);
        snprintf(file, sizeof file, "%s/gallery-%s-%s-xml.png", dir, slug, theme);
        write_png(file, b, wx, hx);
        if (diff && wc == wx && hc == hx) {
            unsigned char *mark = malloc((size_t)wc * hc * 4u);
            if (mark) {
                int k;
                for (k = 0; k < wc * hc * 4; k += 4) {
                    bool same = a[k] == b[k] && a[k + 1] == b[k + 1] && a[k + 2] == b[k + 2];
                    mark[k] = same ? (unsigned char)255 : (unsigned char)220;
                    mark[k + 1] = same ? (unsigned char)255 : 0;
                    mark[k + 2] = same ? (unsigned char)255 : 0;
                    mark[k + 3] = 255;
                }
                snprintf(file, sizeof file, "%s/gallery-%s-%s-diff.png", dir, slug, theme);
                write_png(file, mark, wc, hc);
                free(mark);
            }
        }
    }
    free(a);
    free(b);
    return diff;
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : NULL;
    int only = argc > 2 ? atoi(argv[2]) : -1;
    int only_theme = -1;
    cui_xml *doc;
    int fails = 0;
    int p;
    char slug[64];

    if (argc > 3) only_theme = !strcmp(argv[3], "dark") ? 1 : 0;

    /* one read first, so a file with a mistake is reported once and loudly */
    doc = cui_xml_load(CUI_GALLERY_XML_PATH);
    if (!doc) {
        fprintf(stderr, "%s: cannot be read\n", CUI_GALLERY_XML_PATH);
        return 1;
    }
    if (!cui_xml_ok(doc)) {
        fprintf(stderr, "%s\n", cui_xml_last_error(doc));
        cui_xml_free(doc);
        return 1;
    }
    cui_xml_free(doc);

    {
        int c;
        for (c = 0; c < (int)(sizeof converted / sizeof converted[0]); c++) {
            int d;
            p = converted[c];
            if (only >= 0 && only != p) continue;
            for (d = 0; d < 2; d++) {
                if (only_theme >= 0 && only_theme != d) continue;
                page_slug(p, slug, sizeof slug);
                fails += compare(CUI_GALLERY_XML_PATH, dir, slug, p, d == 1, 0, 1240.0f, page_height(p)) ? 1 : 0;
            }
        }
    }
    /* the accent variants the screenshot tool draws as well */
    if (only < 0) {
        if (!test_gallery_actions()) fails++;
        snprintf(slug, sizeof slug, "overview-green");
        fails += compare(CUI_GALLERY_XML_PATH, dir, slug, PAGE_OVERVIEW, true, CUI_ACCENT_GREEN, 1240.0f, 1000.0f) ? 1 : 0;
        snprintf(slug, sizeof slug, "selection-blue");
        fails += compare(CUI_GALLERY_XML_PATH, dir, slug, PAGE_SELECTION, false, CUI_ACCENT_BLUE, 1240.0f, 1000.0f) ? 1 : 0;
    }
    printf("%s\n", fails ? "the document and the gallery differ" : "the document draws the gallery");
    /* What is still drawn in C: the extended pages that have not moved into the
     * document yet. They are listed so the gap is visible in the test's output. */
    {
        int p, total = gallery_page_total(), listed = 0;
        for (p = 0; p < total; p++) {
            int c, in = 0;
            for (c = 0; c < (int)(sizeof converted / sizeof converted[0]); c++)
                if (converted[c] == p) in = 1;
            if (in) {
                listed++;
                continue;
            }
            {
                char name[64];
                page_slug(p, name, sizeof name);
                printf("  still drawn in C: page %d (%s)\n", p, name);
            }
        }
        printf("%d of %d pages are drawn by the document\n", listed, total);
    if (g_doc_errors) printf("%d document(s) reported a mistake; see above\n", g_doc_errors);
    }
    return fails || g_doc_errors ? 1 : 0;
}
