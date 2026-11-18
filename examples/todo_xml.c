#define _CRT_SECURE_NO_WARNINGS 1

/* examples/todo.c, with its interface in examples/todo.xml.
 *
 * The tasks are the app's data, as in the C example: four of them, one bool
 * per row, and the counter the header shows. The document holds the layout,
 * the field, the filter and the rows.
 *
 *     todoxml                    opens the window
 *     todoxml --shot out.png     draws it offscreen, software renderer
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_TODO_XML_PATH
#  define CUI_TODO_XML_PATH "todo.xml"
#endif

#define TASK_COUNT 4

/* ---- the data the document shows, as examples/todo.c has it -------------- */

typedef struct {
    const char *title;
    bool done;
} todo_task;

static todo_task g_tasks[TASK_COUNT] = {
    { "Buy milk", true },
    { "Write the release notes", false },
    { "Call the bank", true },
    { "Ship 1.0", false },
};

static char g_draft[96];
static int g_filter;
static int g_opencount, g_donecount;

static void count_tasks(void) {
    int i, done = 0;
    for (i = 0; i < TASK_COUNT; i++) done += g_tasks[i].done ? 1 : 0;
    g_donecount = done;
    g_opencount = TASK_COUNT - done;
}

/* ---- actions ------------------------------------------------------------ */

static void on_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    if (!strcmp(action, "add")) {
        g_draft[0] = 0;
        count_tasks();
    } else if (!strcmp(action, "clear")) {
        int i;
        for (i = 0; i < TASK_COUNT; i++) g_tasks[i].done = false;
        count_tasks();
    } else if (!strncmp(action, "remove:", 7)) {
        int i = atoi(action + 7);
        if (i >= 0 && i < TASK_COUNT) g_tasks[i].done = false;
        count_tasks();
    }
}

/* ---- wiring ------------------------------------------------------------- */

static cui_xml *open_document(const char *path) {
    cui_xml *doc = cui_xml_load(path);
    if (!doc) return NULL;
    cui_xml_bind_text(doc, "draft", g_draft, sizeof g_draft);
    cui_xml_bind_int(doc, "filter", &g_filter);
    cui_xml_bind_int(doc, "donecount", &g_donecount);
    cui_xml_bind_bool(doc, "task0", &g_tasks[0].done);
    cui_xml_bind_bool(doc, "task1", &g_tasks[1].done);
    cui_xml_bind_bool(doc, "task2", &g_tasks[2].done);
    cui_xml_bind_bool(doc, "task3", &g_tasks[3].done);
    cui_xml_on_any(doc, on_action, NULL);
    return doc;
}

static void values(cui_xml *doc) {
    char text[16];
    count_tasks();
    snprintf(text, sizeof text, "%d", g_opencount);
    cui_xml_set_var(doc, "open", text);
    snprintf(text, sizeof text, "%d", g_donecount);
    cui_xml_set_var(doc, "done", text);
}

/* The frame the app draws, as cui_app_run calls it. */
static void todo_xml_frame(cui_ctx *ui, void *user) {
    cui_xml *doc = user;
    values(doc);
    cui_xml_poll(doc);
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
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = CUI_THEME_LIGHT });
    if (!ui) {
        cui_sw_destroy(sw);
        return 1;
    }
    cui_xml_window_size(doc, &width, &height);
    cui_set_viewport(ui, (float)width, (float)height, 1);
    cui_input_mouse_leave(ui);
    for (i = 0; i < 6; i++) {
        values(doc);
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
    const char *path = CUI_TODO_XML_PATH;
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
        int rc = cui_app_run(&desc, todo_xml_frame, doc);
        cui_xml_free(doc);
        return rc;
    }
}
