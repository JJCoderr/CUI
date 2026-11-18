#define _CRT_SECURE_NO_WARNINGS 1

/* examples/hello.c, with its interface in examples/hello.xml.
 *
 *     helloxml                    opens the window
 *     helloxml --shot out.png     draws it offscreen, software renderer
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_HELLO_XML_PATH
#  define CUI_HELLO_XML_PATH "hello.xml"
#endif

static int g_clicks;

static void on_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    if (!strcmp(action, "click")) g_clicks++;
}

static cui_xml *open_document(const char *path) {
    cui_xml *doc = cui_xml_load(path);
    if (!doc) return NULL;
    cui_xml_set_var(doc, "clicks", "0");
    cui_xml_on_any(doc, on_action, NULL);
    return doc;
}

static void frame(cui_xml *doc) {
    char text[24];
    snprintf(text, sizeof text, "%d", g_clicks);
    cui_xml_set_var(doc, "clicks", text);
}

/* The frame the app draws, as cui_app_run calls it. */
static void hello_xml_frame(cui_ctx *ui, void *user) {
    cui_xml *doc = user;
    frame(doc);
    cui_xml_poll(doc);
    cui_xml_render(doc, ui);
    if (cui_xml_quit_requested(doc)) cui_app_quit(NULL);
}

/* The window's own pixels, which only the app can take: it saves them after
 * four frames and quits, as the loop this host used to run did by hand. */
static void hello_xml_ask_window_shot(const char *path) {
#if defined(_WIN32)
    _putenv_s("CUI_SCREENSHOT", path);
    _putenv_s("CUI_SCREENSHOT_FRAMES", "4");
#else
    setenv("CUI_SCREENSHOT", path, 1);
    setenv("CUI_SCREENSHOT_FRAMES", "4", 1);
#endif
}

static int write_shot(cui_xml *doc, const char *out_path) {
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui;
    const cui_draw_data *draw;
    unsigned char *pixels;
    int width = 0, height = 0, i;
    if (!sw) return 1;
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = CUI_THEME_LIGHT });
    if (!ui) {
        cui_sw_destroy(sw);
        return 1;
    }
    cui_xml_window_size(doc, &width, &height);
    cui_set_viewport(ui, (float)width, (float)height, 1);
    cui_input_mouse_leave(ui);
    for (i = 0; i < 6; i++) {
        frame(doc);
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
    const char *path = CUI_HELLO_XML_PATH;
    const char *shot = NULL;
    const char *window_shot = NULL;
    cui_xml *doc;
    int arg = 1;

    if (argc > 2 && !strcmp(argv[1], "--shot")) {
        shot = argv[2];
        arg = 3;
    } else if (argc > 2 && !strcmp(argv[1], "--window-shot")) {
        window_shot = argv[2];
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
        int rc;
        if (window_shot) hello_xml_ask_window_shot(window_shot);
        rc = cui_app_run(&desc, hello_xml_frame, doc);
        cui_xml_free(doc);
        return rc;
    }
}
