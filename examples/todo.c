/* A to-do list: the app that docs/tutorial.md builds step by step. It shows the
 * things most apps need: your own data as the state, a list that grows, a filter,
 * a confirmation dialog and saving to a file. */
#include <cui/cui.h>
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 200

typedef struct { char title[96]; bool done; } task;

typedef struct {
    task tasks[MAX_TASKS];
    int count;
    char draft[96];        /* what is being typed into the field */
    int filter;            /* 0 all, 1 open, 2 done */
    bool confirm_clear;    /* the "Clear finished tasks?" dialog is open */
    char path[512];        /* the file the list is saved in; empty: it is not saved */
} app_state;

/* one task a line: "x Buy milk" when done, "- Buy milk" when not */
static void save(const app_state *a) {
    if (!a->path[0]) return;
    FILE *f = fopen(a->path, "w");
    if (!f) return;
    for (int i = 0; i < a->count; i++) fprintf(f, "%c %s\n", a->tasks[i].done ? 'x' : '-', a->tasks[i].title);
    fclose(f);
}

static void load(app_state *a) {
    FILE *f = fopen(a->path, "r");
    if (!f) return;
    char line[128];
    while (a->count < MAX_TASKS && fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) < 3) continue;
        task *t = &a->tasks[a->count++];
        t->done = line[0] == 'x';
        snprintf(t->title, sizeof t->title, "%.95s", line + 2);
    }
    fclose(f);
}

static bool add_task(app_state *a) {
    if (!a->draft[0] || a->count >= MAX_TASKS) return false;
    task *t = &a->tasks[a->count++];
    snprintf(t->title, sizeof t->title, "%s", a->draft);
    t->done = false;
    a->draft[0] = 0;
    return true;
}

static void frame(cui_ctx *ui, void *user) {
    app_state *a = user;
    bool changed = false;
    int done = 0;
    for (int i = 0; i < a->count; i++) done += a->tasks[i].done;

    cui_column(ui, .width = CUI_FILL, .height = CUI_FILL, .align = CUI_ALIGN_CENTER, .bg = CUI_COLOR_BACKGROUND) {
        cui_column(ui, .width = CUI_FILL, .max_width = 560, .height = CUI_FILL, .pad = 24, .gap = 16) {
            cui_row(ui, .gap = 12, .align = CUI_ALIGN_END) {
                cui_heading(ui, "To do", 2);
                cui_spacer(ui);
                cui_textf(ui, "%d open, %d done", a->count - done, done);
            }

            /* add: Enter in the field or the button */
            cui_row(ui, .gap = 8) {
                int typed = cui_input(ui, "new-task", a->draft, sizeof a->draft, .placeholder = "Add a task", .width = CUI_FILL);
                bool add = cui_button(ui, "Add", .variant = CUI_BUTTON_PRIMARY, .icon = CUI_ICON_PLUS, .disabled = !a->draft[0]);
                if ((typed & CUI_SUBMITTED) || add) changed |= add_task(a);
            }

            cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER) {
                static const char *filters[] = { "All", "Open", "Done" };
                cui_segmented(ui, "filter", filters, 3, &a->filter);
                cui_spacer(ui);
                if (cui_button(ui, "Clear done", .variant = CUI_BUTTON_GHOST, .disabled = done == 0)) a->confirm_clear = true;
            }

            /* the list: each row gets an id scope, so two tasks can have the same title */
            int remove = -1, shown = 0;
            cui_scroll(ui, .id = "tasks", .width = CUI_FILL, .height = CUI_FILL, .gap = 2) {
                for (int i = 0; i < a->count; i++) {
                    task *t = &a->tasks[i];
                    if ((a->filter == 1 && t->done) || (a->filter == 2 && !t->done)) continue;
                    shown++;
                    cui_push_id_int(ui, i);
                    cui_row(ui, .gap = 8, .align = CUI_ALIGN_CENTER, .width = CUI_FILL) {
                        if (cui_checkbox(ui, t->title, &t->done)) changed = true;
                        cui_spacer(ui);
                        if (cui_icon_button(ui, CUI_ICON_X, .tooltip = "Delete", .variant = CUI_BUTTON_GHOST, .size = CUI_SIZE_SM)) remove = i;
                    }
                    cui_pop_id(ui);
                }
                if (!shown)
                    cui_empty_state(ui, a->count ? "Nothing here" : "No tasks yet", .icon = CUI_ICON_LIST_CHECKS,
                                    .description = a->count ? "No task matches this filter." : "Type one above and press Enter.");
            }
            if (remove >= 0) {   /* after the loop, so the rows built this frame stay in step with the array */
                memmove(&a->tasks[remove], &a->tasks[remove + 1], sizeof(task) * (size_t)(a->count - remove - 1));
                a->count--;
                changed = true;
            }
        }
    }

    /* a dialog is drawn while its bool is true; closing it sets the bool to false */
    cui_dialog(ui, "Clear finished tasks?", &a->confirm_clear, .description = "They are removed from the list and from the file.", .width = 380) {
        cui_dialog_footer(ui) {
            if (cui_button(ui, "Cancel")) a->confirm_clear = false;
            if (cui_button(ui, "Clear", .variant = CUI_BUTTON_DANGER)) {
                int kept = 0;
                for (int i = 0; i < a->count; i++) if (!a->tasks[i].done) a->tasks[kept++] = a->tasks[i];
                a->count = kept;
                a->confirm_clear = false;
                changed = true;
                cui_toast(ui, "Finished tasks cleared");
            }
        }
    }

    if (changed) save(a);
}

/* The list `todo --shot` draws: four tasks, two of them done, so the picture
 * shows the rows, the counter and both states. examples/todoxml.c starts from
 * the same four. */
static void seed(app_state *a) {
    static const task seed_tasks[4] = {
        { "Buy milk", true },
        { "Write the release notes", false },
        { "Call the bank", true },
        { "Ship 1.0", false },
    };
    memcpy(a->tasks, seed_tasks, sizeof seed_tasks);
    a->count = 4;
}

/* `todo --shot out.png`: the same window drawn offscreen with the software
 * renderer, for a machine without one -- and to compare with examples/todoxml.c,
 * the same app described in XML. It saves nothing. */
static int write_shot(const char *out_path) {
    static app_state a;
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui;
    const cui_draw_data *draw;
    unsigned char *pixels;
    int i;
    if (!sw) return 1;
    seed(&a);
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = CUI_THEME_LIGHT });
    if (!ui) {
        cui_sw_destroy(sw);
        return 1;
    }
    cui_set_viewport(ui, 520, 640, 1);
    cui_input_mouse_leave(ui);
    for (i = 0; i < 6; i++) {
        cui_begin_frame(ui, 1.0f / 60.0f);
        frame(ui, &a);
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
    static app_state a;
    if (argc > 2 && !strcmp(argv[1], "--shot")) return write_shot(argv[2]);
    snprintf(a.path, sizeof a.path, "todo.txt");   /* beside where the app is started */
    load(&a);
    return cui_app_run(&(cui_app_desc){ .title = "To do", .width = 520, .height = 640 }, frame, &a);
}
