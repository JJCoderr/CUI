/* The smallest CUI app.
 *
 *     hello                    opens the window
 *     hello --shot out.png     draws it offscreen with the software renderer,
 *                              which is how examples/helloxml.c (the same window
 *                              in XML) is compared with it */
#include <cui/cui.h>
#include <cui/cui_app.h>
#include <cui/cui_render.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bool notifications;
    int clicks;
} hello_state;

static void frame(cui_ctx *ui, void *user) {
    hello_state *s = user;
    cui_column(ui, .pad = 32, .gap = 16, .width = CUI_FILL) {
        cui_heading(ui, "Hello, CUI", 2);
        cui_secondary(ui, "A native window, a few components and no dependencies.");
        cui_switch(ui, "Notifications", &s->notifications);
        cui_row(ui, .gap = 8) {
            if (cui_button(ui, "Click me", .variant = CUI_BUTTON_PRIMARY)) s->clicks++;
            cui_textf(ui, "Clicked %d times", s->clicks);
        }
    }
}

static int write_shot(const char *out_path) {
    hello_state s = { .notifications = true, .clicks = 0 };
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui;
    const cui_draw_data *draw;
    unsigned char *pixels;
    int i;
    if (!sw) return 1;
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = CUI_THEME_LIGHT });
    if (!ui) {
        cui_sw_destroy(sw);
        return 1;
    }
    cui_set_viewport(ui, 420, 300, 1);
    cui_input_mouse_leave(ui);
    for (i = 0; i < 6; i++) {
        cui_begin_frame(ui, 1.0f / 60.0f);
        frame(ui, &s);
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
    static hello_state s = { .notifications = true };
    if (argc > 2 && !strcmp(argv[1], "--shot")) return write_shot(argv[2]);
    return cui_app_run(&(cui_app_desc){ .title = "Hello", .width = 420, .height = 300 }, frame, &s);
}
