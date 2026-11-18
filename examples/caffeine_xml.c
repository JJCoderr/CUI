#define _CRT_SECURE_NO_WARNINGS 1

/* examples/caffeine.c, with its interface in examples/caffeine.xml.
 *
 * The model is what a document cannot say: one curve per cup, the peak, what
 * is left at bedtime, the latest time the last cup could have been drunk, and
 * the two annotations of the chart. It is computed here the way the C example
 * computes it, and the picture is drawn from the same numbers: <Level> is the
 * dot chart, <Cups> the event track, both registered on the document.
 *
 *     caffeinexml                    opens the window
 *     caffeinexml --shot out.png     draws it offscreen, software renderer
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_CAFFEINE_XML_PATH
#  define CUI_CAFFEINE_XML_PATH "caffeine.xml"
#endif

#define MAX_CUPS   12
#define SAMPLES    145
#define DAY_START  6.0
#define DAY_END    30.0
#define BEDTIME    23.0
#define LIMIT_MG   50.0f
#define CUP_MG     95.0
#define HALF_LIFE  5.0

typedef struct {
    double cups[MAX_CUPS];
    int count, selected;
    float curve[MAX_CUPS][SAMPLES];
} caffeine_state;

static caffeine_state g;

/* what the document shows as text, computed once a frame */
static char g_summary[64], g_peak[16], g_peak_at[16], g_at_bed[16], g_bed_line[32], g_verdict[64];
static char g_note_peak[64], g_note_bed[64];
static bool g_over;

/* ---- the model, as examples/caffeine.c has it --------------------------- */

static double cup_level(double t) {
    if (t <= 0) return 0;
    double ke = log(2.0) / HALF_LIFE, ka = log(2.0) / 0.25;
    return CUP_MG * ka / (ka - ke) * (exp(-ke * t) - exp(-ka * t));
}

static double level_at(const double *cups, int n, double at) {
    double s = 0;
    for (int i = 0; i < n; i++) s += cup_level(at - cups[i]);
    return s;
}

static const char *clock_text(char *buf, size_t cap, double h) {
    int m = (int)lround(h * 60) % (24 * 60);
    snprintf(buf, cap, "%02d:%02d", m / 60, m % 60);
    return buf;
}

static void compute(void) {
    char t2[16];
    double peak = 0, peak_at = DAY_START;
    for (int s = 0; s < SAMPLES; s++) {
        double at = DAY_START + (DAY_END - DAY_START) * s / (SAMPLES - 1), total = 0;
        for (int c = 0; c < g.count; c++) {
            g.curve[c][s] = (float)cup_level(at - g.cups[c]);
            total += g.curve[c][s];
        }
        if (total > peak) {
            peak = total;
            peak_at = at;
        }
    }
    double at_bed = level_at(g.cups, g.count, BEDTIME);
    snprintf(g_summary, sizeof g_summary, "%d CUPS \xc2\xb7 %.0f MG \xc2\xb7 HALF-LIFE %.0fH", g.count,
             g.count * CUP_MG, HALF_LIFE);
    if (g.count == 0) {
        snprintf(g_verdict, sizeof g_verdict, "NO CUPS YET");
        snprintf(g_peak, sizeof g_peak, "\xe2\x80\x94");
        g_peak_at[0] = 0;
    } else {
        snprintf(g_peak, sizeof g_peak, "%.0f", peak);
        clock_text(g_peak_at, sizeof g_peak_at, peak_at);
        if (level_at(g.cups, g.count - 1, BEDTIME) > LIMIT_MG) {
            snprintf(g_verdict, sizeof g_verdict, "OVER THE LINE EVEN WITHOUT THE LAST CUP");
        } else {
            double lo = DAY_START, hi = BEDTIME;
            for (int k = 0; k < 40; k++) {
                double mid = (lo + hi) / 2, cups[MAX_CUPS];
                for (int c = 0; c < g.count - 1; c++) cups[c] = g.cups[c];
                cups[g.count - 1] = mid;
                if (level_at(cups, g.count, BEDTIME) <= LIMIT_MG) lo = mid;
                else hi = mid;
            }
            snprintf(g_verdict, sizeof g_verdict, "LAST CUP BY %s", clock_text(t2, sizeof t2, floor(lo * 4) / 4));
        }
    }
    snprintf(g_at_bed, sizeof g_at_bed, "%.0f", at_bed);
    g_over = at_bed > LIMIT_MG;
    snprintf(g_bed_line, sizeof g_bed_line, "%s", g_over ? "OVER THE LINE" : "UNDER THE LINE");
    snprintf(g_note_peak, sizeof g_note_peak, "PEAK %.0f MG", peak);
    snprintf(g_note_bed, sizeof g_note_bed, "%.0f MG STILL IN YOU", at_bed);
}

/* ---- <Level>: the dot-matrix chart with its annotations ---------------- */

static bool tag_level(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_dot_series series[MAX_CUPS];
    cui_chart_annotation notes[3];
    cui_dot_chart_result r;
    int c;
    (void)doc;
    (void)user;
    for (c = 0; c < g.count; c++) {
        series[c] = (cui_dot_series){ g.curve[c], NULL, c == g.selected };
    }
    notes[0] = (cui_chart_annotation){ CUI_ANNOTATION_Y, 0, LIMIT_MG, "50", NULL, false };
    notes[1] = (cui_chart_annotation){ CUI_ANNOTATION_X, BEDTIME, 0, "BED 23:00", g_note_bed, false };
    notes[2] = (cui_chart_annotation){ CUI_ANNOTATION_X, 0, 0, g_note_peak, g_peak_at, false };
    /* the peak's x follows the model: recompute it here, as the C example does */
    {
        double peak = 0, peak_at = DAY_START;
        for (int s = 0; s < SAMPLES; s++) {
            double at = DAY_START + (DAY_END - DAY_START) * s / (SAMPLES - 1), total = 0;
            for (c = 0; c < g.count; c++) total += g.curve[c][s];
            if (total > peak) {
                peak = total;
                peak_at = at;
            }
        }
        notes[2].x = peak_at;
    }
    r = cui_dot_chart(ui, cui_xml_node_name(node), series, g.count, SAMPLES, .height = 300, .x_min = DAY_START,
                      .x_max = DAY_END, .x_units = CUI_AXIS_HOURS, .min = 0, .max = 260, .unit = "MG",
                      .no_axis = true, .annotations = notes, .annotation_count = g.count ? 3 : 1);
    (void)r;
    return true;
}

/* ---- <Cups>: the event track under the chart --------------------------- */

static bool tag_cups(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)doc;
    (void)user;
    cui_event_track(ui, cui_xml_node_name(node), g.cups, &g.count, MAX_CUPS, DAY_START, DAY_END, .step = 0.25,
                    .units = CUI_AXIS_HOURS, .label_step = 6, .selected = &g.selected);
    return true;
}

/* ---- wiring ------------------------------------------------------------- */

static cui_xml *open_document(const char *path) {
    cui_xml *doc = cui_xml_load(path);
    if (!doc) return NULL;
    cui_xml_set_var(doc, "summary", "");
    cui_xml_set_var(doc, "peak", "");
    cui_xml_set_var(doc, "peakAt", "");
    cui_xml_set_var(doc, "atbed", "");
    cui_xml_set_var(doc, "bedLine", "");
    cui_xml_set_var(doc, "verdict", "");
    cui_xml_set_var(doc, "over", "0");
    cui_xml_set_var(doc, "danger", "0");
    cui_xml_register_tag(doc, "Level", tag_level, NULL);
    cui_xml_register_tag(doc, "Cups", tag_cups, NULL);
    return doc;
}

static void values(cui_xml *doc) {
    compute();
    cui_xml_set_var(doc, "summary", g_summary);
    cui_xml_set_var(doc, "peak", g_peak);
    cui_xml_set_var(doc, "peakAt", g_peak_at);
    cui_xml_set_var(doc, "atbed", g_at_bed);
    cui_xml_set_var(doc, "bedLine", g_bed_line);
    cui_xml_set_var(doc, "verdict", g_verdict);
    cui_xml_set_var(doc, "over", g_over ? "1" : "0");
    cui_xml_set_var(doc, "danger", (g.count && g_verdict[0] == 'O') ? "1" : "0");
}

/* The frame the app draws, as cui_app_run calls it. */
static void caffeine_xml_frame(cui_ctx *ui, void *user) {
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
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = CUI_THEME_DARK,
                                     .accent = CUI_ACCENT_BLUE });
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
    const char *path = CUI_CAFFEINE_XML_PATH;
    const char *shot = NULL;
    cui_xml *doc;
    int arg = 1;

    g.cups[0] = 8.0;
    g.cups[1] = 10.75;
    g.cups[2] = 12.25;
    g.count = 3;
    g.selected = 2;

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
        int rc = cui_app_run(&desc, caffeine_xml_frame, doc);
        cui_xml_free(doc);
        return rc;
    }
}

