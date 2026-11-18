/* The XML demo: the window is described in examples/xml_demo.xml.
 *
 * This file knows nothing about the layout. It binds four values, takes the
 * actions the document fires, and lets the document follow its file.
 *
 *     xmlui                         reads examples/xml_demo.xml
 *     xmlui path/to/other.xml       reads that file
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_XML_DEMO_PATH
#  define CUI_XML_DEMO_PATH "xml_demo.xml"
#endif

static cui_ctx *g_ui;
static int g_clicks;

/* The values the document writes, and reads back through {"..."}. */
static bool g_notify = true;
static bool g_shared = true;
static float g_scale = 1.0f;
static int g_tab = 0;

static void on_xml_error(const char *path, const cui_xml_error *error, void *user) {
    (void)user;
    fprintf(stderr, "%s:%d:%d: %s\n", path, error->line, error->column, error->message);
}

static void on_reload(cui_xml *doc, bool ok, void *user) {
    (void)doc;
    (void)user;
    fprintf(stderr, "reload: %s\n", ok ? "the interface follows the file"
                                       : "the file has a mistake, the interface is unchanged");
}

static void toast(const char *title, const char *description, cui_tone tone) {
    cui_toast_opts o;
    memset(&o, 0, sizeof o);
    o.description = description;
    o.tone = tone;
    cui_toast_ex(g_ui, title, &o);
}

static void on_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    (void)user;
    g_clicks++;
    if (!strcmp(action, "file.save")) {
        char text[160];
        snprintf(text, sizeof text, "%s, scale %.1fx, notifications %s",
                 cui_xml_get_text(doc, "folder"), (double)g_scale, g_notify ? "on" : "off");
        toast("Saved", text, CUI_TONE_SUCCESS);
    } else if (!strcmp(action, "file.browse")) {
        toast("Browse", "cui_app_file_dialog opens the system's picker here", CUI_TONE_NEUTRAL);
    } else if (!strcmp(action, "file.open")) {
        toast("Open", "The table row under the pointer knows its own element", CUI_TONE_INFO);
    } else if (!strcmp(action, "row.picked")) {
        char text[64];
        snprintf(text, sizeof text, "row %d was clicked", cui_xml_node_index(node));
        toast("Table", text, CUI_TONE_VIOLET);
    } else {
        toast(action, "This name has no handler: cui_xml_on_any received it", CUI_TONE_NEUTRAL);
    }
}

/* The frame the app draws, as cui_app_run calls it. The toast helper needs a
 * context, so the running one is kept here as the loop this host used to run
 * did by hand. */
static void xml_demo_frame(cui_ctx *ui, void *user) {
    char count[24];
    cui_xml *doc = user;
    g_ui = ui;
    cui_xml_poll(doc);                     /* reads the file again when it changed */
    snprintf(count, sizeof count, "%d", g_clicks);
    cui_xml_set_var(doc, "clicks", count); /* what {clicks} shows in the status bar */
    cui_xml_render(doc, ui);
    if (cui_xml_quit_requested(doc)) cui_app_quit(NULL);
}

/* `xmlui --shot out.png [file.xml]`: draws the document offscreen with the
 * software renderer and writes the picture, so a machine without a window (a
 * build agent, a terminal over ssh) can see what the XML describes. */
static int write_shot(cui_xml *doc, const char *out_path) {
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui;
    const cui_draw_data *draw;
    unsigned char *pixels;
    int width = 0, height = 0, i;
    if (!sw) return 1;
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw) });
    if (!ui) {
        cui_sw_destroy(sw);
        return 1;
    }
    cui_xml_window_size(doc, &width, &height);
    cui_set_viewport(ui, (float)width, (float)height, 1);
    cui_input_mouse_leave(ui);
    cui_xml_set_var(doc, "clicks", "3");
    for (i = 0; i < 4; i++) {          /* a few frames: layout settles, things animate in */
        cui_begin_frame(ui, 1.0f / 60.0f);
        cui_xml_render(doc, ui);
        cui_end_frame(ui);
    }
    draw = cui_get_draw_data(ui);
    pixels = malloc((size_t)draw->width_px * (size_t)draw->height_px * 4u);
    if (pixels) {
        cui_sw_render(sw, draw, pixels, draw->width_px, draw->height_px, draw->width_px * 4,
                      CUI_SW_RGBA);
        cui_write_png(out_path, pixels, draw->width_px, draw->height_px, draw->width_px * 4);
        free(pixels);
        printf("%s: %d x %d\n", out_path, draw->width_px, draw->height_px);
    }
    cui_destroy(ui);
    cui_sw_destroy(sw);
    return pixels ? 0 : 1;
}

int main(int argc, char **argv) {
    const char *path = CUI_XML_DEMO_PATH;
    const char *shot = NULL;
    cui_xml *doc;
    cui_app_desc desc;
    int arg = 1;

    if (argc > 2 && (!strcmp(argv[1], "--shot") || !strcmp(argv[1], "-s"))) {
        shot = argv[2];
        arg = 3;
    }
    if (argc > arg) path = argv[arg];

    doc = cui_xml_load(path);
    if (!doc) {
        fprintf(stderr, "%s: cannot be read\n", path);
        return 1;
    }
    cui_xml_set_error_callback(doc, on_xml_error, NULL);
    cui_xml_set_reload_callback(doc, on_reload, NULL);
    if (!cui_xml_ok(doc)) fprintf(stderr, "%s\n", cui_xml_last_error(doc));

    /* The document writes these; the app owns them. */
    cui_xml_bind_bool(doc, "notify", &g_notify);
    cui_xml_bind_bool(doc, "shared", &g_shared);
    cui_xml_bind_float(doc, "scale", &g_scale);
    cui_xml_bind_int(doc, "tab", &g_tab);
    cui_xml_on_any(doc, on_action, NULL);

    if (shot) {
        int rc = write_shot(doc, shot);
        cui_xml_free(doc);
        return rc;
    }

    desc = cui_xml_app_desc(doc);              /* Title, Width, Height, Theme */
    {
        int rc = cui_app_run(&desc, xml_demo_frame, doc);
        cui_xml_free(doc);
        return rc;
    }
}
