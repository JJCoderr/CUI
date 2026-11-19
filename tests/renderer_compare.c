/* Native Win32 backend comparison. Run on an interactive desktop. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include "../src/platform/cui_platform.h"
#include "../examples/gallery_ui.h"

/* This test intentionally compares Vulkan execution paths through the app API. */
bool cui__vk_bench_configure(cui_app *app, bool instancing);
bool cui__vk_bench_wait(cui_app *app);
size_t cui__vk_bench_geometry_bytes(const cui_app *app);

enum { SHOTS = 3 };
static const int pages[SHOTS] = { PAGE_SELECTION, PAGE_INPUTS, PAGE_CHARTS };
static const char *shot_names[SHOTS] = { "selection-light", "inputs-dark", "charts-light" };
static const uint8_t sample_image[] = {
    255, 40, 40, 255, 40, 210, 70, 160,
    30, 50, 255, 120, 240, 150, 0, 255
};

typedef struct { cui_texture image; float shift; } sample;

static void paint_sample(cui_ctx *ui, cui_rect rect, void *data) {
    const sample *s = data;
    cui_draw_image(ui, s->image, (cui_rect){ rect.x + rect.w - 54 - s->shift, rect.y + rect.h - 54, 42, 42 },
                   (cui_rect){ 0, 0, 1, 1 }, CUI_CORNERS(8), CUI_RGB(0xffffff));
}

static void frame(cui_app *app, gallery *g, cui_texture image, float shift) {
    cui_ctx *ui = cui_app_ui(app);
    float w, h, scale;
    int fw, fh;
    cui__plat_metrics(app, &w, &h, &scale, &fw, &fh);
    cui_set_viewport(ui, w, h, scale);
    cui_begin_frame(ui, 1.0f / 60);
    gallery_frame(ui, g);
    sample s = { image, shift };
    cui__float_begin(ui, 0x75000, 1, 0, CUI__PLACE_FULL, 0);
    cui_canvas(ui, "comparison sample", CUI_FILL, CUI_FILL, paint_sample, &s, sizeof s);
    cui__float_end(ui);
    cui_end_frame(ui);
}

static bool drain(cui_app *app) {
    if (cui_app_is_software(app)) return true;
    if (app->desc.renderer == CUI_RENDERER_AUTO) return cui__vk_bench_wait(app);
    glFinish();
    return true;
}

static bool time_frames(cui_app *app, gallery *g, cui_texture image, const char *name, int count, bool full, bool uploads) {
    if (app->sw) cui_sw_set_incremental(app->sw, !full);
    double start = 0;
    for (int i = -60; i < count; i++) {
        if (i == 0) {
            if (!drain(app)) return false;
            start = cui__plat_now();
        }
        cui__plat_wait(app, 0);
        if (uploads) {
            cui_renderer_iface *renderer = &cui_app_ui(app)->renderer;
            uint8_t pixel[] = { (uint8_t)(i + 60), 80, 180, 255 };
            renderer->texture_update(renderer->user, image, 1, 1, 1, 1, pixel, sizeof pixel);
        }
        frame(app, g, image, (float)((i + 60) % 100));
        cui__app_render(app);
        if (!app->running) return false;
    }
    if (!drain(app)) return false;
    double elapsed = cui__plat_now() - start;
    printf("%s%s: %.2f FPS, %.3f ms/frame (%d frames)\n", name, uploads ? " texture updates" : full ? " full redraw" : "",
           count / elapsed, 1000 * elapsed / count, count);
    return true;
}

typedef struct { uint8_t *pixels; int w, h; } shot;

static bool compare(const shot *a, const shot *b, const char *name, const char *reference,
                    const char *scene, double pixel_tolerance) {
    if (a->w != b->w || a->h != b->h) {
        fprintf(stderr, "%s %s dimensions differ from %s\n", name, scene, reference);
        return false;
    }
    size_t pixels = (size_t)a->w * a->h, different = 0, over_two = 0;
    double total = 0;
    int max = 0;
    for (size_t i = 0; i < pixels; i++) {
        int pixel_max = 0;
        for (int j = 0; j < 4; j++) {
            int delta = abs((int)a->pixels[i * 4 + j] - b->pixels[i * 4 + j]);
            total += delta;
            if (delta > pixel_max) pixel_max = delta;
        }
        different += pixel_max != 0;
        over_two += pixel_max > 2;
        if (pixel_max > max) max = pixel_max;
    }
    double mean = total / (4 * pixels);
    double fraction = 100.0 * over_two / pixels;
    printf("%s vs %s %s %dx%d: different %.4f%%, >2 %.4f%%, max %d, mean %.4f/255\n",
           name, reference, scene, a->w, a->h, 100.0 * different / pixels, fraction, max, mean);
    return mean < 0.1 && fraction < pixel_tolerance;
}

static bool check_readback_stride(cui_app *app, const shot *compact) {
    int width = compact->w + 7;
    uint8_t *padded = malloc((size_t)width * compact->h * 4);
    if (!padded) return false;
    bool ok = cui_app_screenshot(app, padded, width, compact->h);
    size_t row = (size_t)compact->w * 4;
    for (int y = 0; ok && y < compact->h; y++)
        ok = memcmp(padded + (size_t)y * width * 4, compact->pixels + (size_t)y * row, row) == 0;
    free(padded);
    return ok;
}

static bool capture(cui_app *app, gallery *g, cui_texture image, const char *dir,
                    const char *name, int i, shot *out) {
    g->page = pages[i];
    g->dark = i == 1;
    cui_set_theme(cui_app_ui(app), g->dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
    for (int n = 0; n < 12; n++) {
        cui__plat_wait(app, 0);
        cui_input_mouse_leave(cui_app_ui(app));
        frame(app, g, image, 0);
        cui__app_render(app);
    }
    const cui_draw_data *dd = cui_get_draw_data(cui_app_ui(app));
    out->w = dd->width_px;
    out->h = dd->height_px;
    if (out->w <= 0 || out->h <= 0) return false;
    /* Readback must see a partial upload queued after the last window frame. */
    cui_renderer_iface *renderer = &cui_app_ui(app)->renderer;
    uint8_t pixel[] = { (uint8_t)(80 + 50 * i), 150, 60, 255 };
    renderer->texture_update(renderer->user, image, 1, 1, 1, 1, pixel, sizeof pixel);
    out->pixels = malloc((size_t)out->w * out->h * 4);
    if (!out->pixels || !cui_app_screenshot(app, out->pixels, out->w, out->h)) return false;
    if (i == 0 && !check_readback_stride(app, out)) return false;
    if (!dir) return true;
    char path[1024];
    int n = snprintf(path, sizeof path, "%s/%s-%s.png", dir, name, shot_names[i]);
    return n > 0 && (size_t)n < sizeof path && cui_write_png(path, out->pixels, out->w, out->h, out->w * 4);
}

static bool check_instances(cui_app *app, gallery *g, cui_texture image) {
    cui_ctx *ui = cui_app_ui(app);
    size_t total_indexed = 0, total_instanced = 0;
    for (int page = 0; page < gallery_page_total(); page++) {
        g->page = page;
        g->dark = page == PAGE_INPUTS;
        cui_set_theme(ui, g->dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
        cui__plat_wait(app, 0);
        cui_input_mouse_leave(ui);
        for (int i = 0; i < 12; i++) frame(app, g, image, 0);
        const cui_draw_data *dd = cui_get_draw_data(ui);
        int w = dd->width_px, h = dd->height_px;
        if (w <= 0 || h <= 0) return false;
        size_t bytes = (size_t)w * h * 4;
        uint8_t *pixels = malloc(bytes * 2);
        if (!pixels) return false;
        shot indexed = { pixels, w, h }, instanced = { pixels + bytes, w, h };
        /* Both reads consume exactly the same completed UI frame. */
        bool ok = cui__vk_bench_configure(app, false) && cui_app_screenshot(app, indexed.pixels, w, h);
        size_t before = cui__vk_bench_geometry_bytes(app);
        ok = ok && cui__vk_bench_configure(app, true) && cui_app_screenshot(app, instanced.pixels, w, h);
        size_t after = cui__vk_bench_geometry_bytes(app);
        ok = ok && compare(&indexed, &instanced, "adaptive", "indexed", gallery_page_name(page), 0.1);
        free(pixels);
        if (!ok) return false;
        printf("geometry %s: %zu -> %zu bytes\n", gallery_page_name(page), before, after);
        total_indexed += before;
        total_instanced += after;
    }
    printf("geometry total: %zu -> %zu bytes\n", total_indexed, total_instanced);
    return total_indexed > total_instanced;
}

static int compare_instances(int frames) {
    cui_app_desc desc = {0};
    desc.title = "CUI Vulkan instancing comparison";
    desc.width = 1240;
    desc.height = 820;
    desc.accent = CUI_ACCENT_VIOLET;
    desc.no_vsync = true;
    cui_app *app = cui_app_create(&desc);
    if (!cui__vk_bench_configure(app, true)) {
        fprintf(stderr, "Vulkan unavailable\n");
        cui_app_destroy(app);
        return 1;
    }
    cui_ctx *ui = cui_app_ui(app);
    cui_texture image = cui_image_create(ui, 2, 2, sample_image);
    gallery g;
    gallery_init(&g);
    bool ok = image && check_instances(app, &g, image);
    int instruments = 0;
    while (instruments < gallery_page_total() && strcmp(gallery_page_name(instruments), "Instruments")) instruments++;
    if (instruments == gallery_page_total()) ok = false;
    const int workloads[] = { PAGE_SELECTION, PAGE_CHARTS, instruments, PAGE_SELECTION };
    for (int i = 0; ok && i < 4; i++) {
        gallery_init(&g);
        g.page = workloads[i];
        cui_set_theme(ui, CUI_THEME_LIGHT);
        for (int trial = 0; ok && trial < 6; trial++) {
            bool enabled = ((trial + 1) / 2) % 2 != 0;
            cui__vk_bench_configure(app, enabled);
            char name[128];
            snprintf(name, sizeof name, "%s %s", gallery_page_name(g.page), enabled ? "adaptive" : "indexed");
            ok = time_frames(app, &g, image, name, frames, false, i == 3);
        }
    }
    if (image) cui_image_destroy(ui, image);
    cui_app_destroy(app);
    return ok ? 0 : 1;
}

int main(int argc, char **argv) {
    int frames = argc > 2 ? atoi(argv[2]) : 300;
    if (frames <= 0) return 2;
    if (argc > 3 && strcmp(argv[3], "instances") == 0) return compare_instances(frames);
    const char *dir = argc > 1 ? argv[1] : NULL;
    bool uploads = argc > 3 && strcmp(argv[3], "uploads") == 0;
    const cui_renderer_kind kinds[] = { CUI_RENDERER_OPENGL, CUI_RENDERER_AUTO, CUI_RENDERER_SOFTWARE };
    const char *names[] = { "opengl", "vulkan", "software" };
    shot reference[SHOTS] = {0};
    bool ok = true;
    for (int backend = 0; backend < 3; backend++) {
        cui_app_desc desc = {0};
        desc.title = "CUI renderer comparison";
        desc.width = 1240;
        desc.height = 820;
        desc.accent = CUI_ACCENT_VIOLET;
        desc.no_vsync = true;
        desc.renderer = kinds[backend];
        cui_app *app = cui_app_create(&desc);
        if (!app || (kinds[backend] == CUI_RENDERER_AUTO ? !cui__vk_bench_configure(app, true) :
            cui_app_is_software(app) != (kinds[backend] == CUI_RENDERER_SOFTWARE))) {
            fprintf(stderr, "%s renderer unavailable\n", names[backend]);
            cui_app_destroy(app);
            ok = false;
            break;
        }
        /* Keep focus and pointer differences out of the static captures. */
        cui__plat_wait(app, 0);
        cui_input_focus(cui_app_ui(app), true);
        gallery g;
        gallery_init(&g);
        cui_texture image = cui_image_create(cui_app_ui(app), 2, 2, sample_image);
        if (!image) ok = false;
        for (int i = 0; image && i < SHOTS; i++) {
            shot current = {0};
            if (!capture(app, &g, image, dir, names[backend], i, &current)) {
                fprintf(stderr, "could not capture %s %s\n", names[backend], shot_names[i]);
                free(current.pixels);
                ok = false;
                break;
            }
            if (backend == 0) reference[i] = current;
            else {
                double tolerance = cui_app_is_software(app) ? 0.25 : 0.1;
                if (!compare(&reference[i], &current, names[backend], "OpenGL", shot_names[i], tolerance)) ok = false;
                free(current.pixels);
            }
        }
        g.page = PAGE_SELECTION;
        for (int trial = 0; image && trial < 3; trial++)
            if (!time_frames(app, &g, image, names[backend], frames, false, uploads)) ok = false;
        if (cui_app_is_software(app) && !uploads)
            for (int trial = 0; image && trial < 3; trial++)
                if (!time_frames(app, &g, image, names[backend], frames, true, false)) ok = false;
        if (kinds[backend] == CUI_RENDERER_AUTO) {
            cui_app_set_size(app, 1000, 690);
            cui__plat_wait(app, 0);
            frame(app, &g, image, 0);
            cui__app_render(app);
            if (!app->presented_once || !drain(app)) {
                fprintf(stderr, "Vulkan swapchain resize failed\n");
                ok = false;
            }
        }
        if (image) cui_image_destroy(cui_app_ui(app), image);
        cui_app_destroy(app);
        if (backend == 0 && !ok) break;
    }
    for (int i = 0; i < SHOTS; i++) free(reference[i].pixels);
    return ok ? 0 : 1;
}
