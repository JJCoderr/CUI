#define _CRT_SECURE_NO_WARNINGS 1

/* The component gallery of examples/gallery_ui.c, described in
 * examples/gallery.xml.
 *
 *     galleryxml                       opens the window the document describes
 *     galleryxml --shot out.png        draws it offscreen, software renderer
 *     galleryxml --page 3 --shot o.png the same, on another page
 *
 * Everything about the layout, the type and the colors is in the document; the
 * C side keeps the values the controls hold and the actions they fire.
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gallery_xml_ui.h"

#ifndef CUI_GALLERY_XML_PATH
#  define CUI_GALLERY_XML_PATH "gallery.xml"
#endif

/* the values the document writes and reads back, kept for the window's life */
static gallery g;

static void on_xml_error(const char *path, const cui_xml_error *error, void *user) {
    (void)user;
    fprintf(stderr, "%s:%d:%d: %s\n", path, error->line, error->column, error->message);
}

/* What the frame callback works on: the document and the gallery behind it. */
typedef struct {
    cui_xml *doc;
    gallery *g;
} gallery_xml_view;

/* The frame the app draws, as cui_app_run calls it. */
static void gallery_xml_frame(cui_ctx *ui, void *user) {
    gallery_xml_view *v = user;
    gallery_xml_values(v->doc, v->g);
    gallery_xml_apply_theme(ui, v->g);
    cui_xml_poll(v->doc);
    cui_xml_render(v->doc, ui);
    if (cui_xml_quit_requested(v->doc)) cui_app_quit(NULL);
}

/* The window's own pixels, which only the app can take: it saves them after
 * four frames and quits, as the loop this host used to run did by hand. */
static void gallery_xml_ask_window_shot(const char *path) {
#if defined(_WIN32)
    _putenv_s("CUI_SCREENSHOT", path);
    _putenv_s("CUI_SCREENSHOT_FRAMES", "4");
#else
    setenv("CUI_SCREENSHOT", path, 1);
    setenv("CUI_SCREENSHOT_FRAMES", "4", 1);
#endif
}

static int write_shot(cui_xml *doc, gallery *g, const char *out_path, bool dark, int width, int height, int frames) {
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui;
    const cui_draw_data *draw;
    unsigned char *pixels;
    int i;
    if (!sw) return 1;
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw),
                                     .theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT,
                                     .accent = (cui_accent)g->accent });
    if (!ui) {
        cui_sw_destroy(sw);
        return 1;
    }
    cui_set_viewport(ui, (float)width, (float)height, 2);
    cui_input_mouse_leave(ui);
    for (i = 0; i < frames; i++) {
        gallery_xml_values(doc, g);
        cui_begin_frame(ui, 1.0f / 60.0f);
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
    const char *path = CUI_GALLERY_XML_PATH;
    const char *shot = NULL;
    const char *window_shot = NULL;
    int page = -1, frames = 90, width = 1240, height = 1000;
    bool dark = false;
    cui_xml *doc;
    int arg = 1;

    for (; arg < argc; arg++) {
        if (!strcmp(argv[arg], "--shot") && arg + 1 < argc) shot = argv[++arg];
        else if (!strcmp(argv[arg], "--window-shot") && arg + 1 < argc) window_shot = argv[++arg];
        else if (!strcmp(argv[arg], "--page") && arg + 1 < argc) page = atoi(argv[++arg]);
        else if (!strcmp(argv[arg], "--frames") && arg + 1 < argc) frames = atoi(argv[++arg]);
        else if (!strcmp(argv[arg], "--width") && arg + 1 < argc) width = atoi(argv[++arg]);
        else if (!strcmp(argv[arg], "--height") && arg + 1 < argc) height = atoi(argv[++arg]);
        else if (!strcmp(argv[arg], "--dark")) dark = true;
        else if (!strcmp(argv[arg], "--light")) dark = false;
        else if (argv[arg][0] != '-') path = argv[arg];
    }

    gallery_init(&g);
    if (page >= 0 && page < gallery_page_total()) g.page = page;
    g.dark = dark;

    doc = gallery_xml_open(path, &g);
    if (!doc) {
        fprintf(stderr, "%s: cannot be read\n", path);
        return 1;
    }
    cui_xml_set_error_callback(doc, on_xml_error, NULL);
    if (!cui_xml_ok(doc)) fprintf(stderr, "%s\n", cui_xml_last_error(doc));

    if (shot) {
        int rc = write_shot(doc, &g, shot, dark, width, height, frames);
        cui_xml_free(doc);
        return rc;
    }
    {
        cui_app_desc desc = cui_xml_app_desc(doc);
        gallery_xml_view view;
        int rc;
        desc.theme = g.dark ? CUI_THEME_DARK : CUI_THEME_LIGHT;
        desc.accent = (cui_accent)g.accent;
        view.doc = doc;
        view.g = &g;
        if (window_shot) gallery_xml_ask_window_shot(window_shot);
        rc = cui_app_run(&desc, gallery_xml_frame, &view);
        cui_xml_free(doc);
        return rc;
    }
}
