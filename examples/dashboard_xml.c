#define _CRT_SECURE_NO_WARNINGS 1

/* examples/dashboard.c, with its interface in examples/dashboard.xml.
 *
 * The state is the app's data, as in the C example: the open page, the chosen
 * range, the search, the table's order and its selected row. The document binds
 * those and holds the sidebar, the top bar, the figures, the charts and the
 * table's card. What a document cannot say is here: the orders the table shows
 * (and the search that filters them), the figures of a range with the sentences
 * under them, the theme the account toggles, the command palette and its
 * toasts. Two pieces are tags of their own -- <Orders>, which is the table with
 * the app's rows, and <NumberRoll>, the four figures that roll when the range
 * changes -- and the drawer of a narrow window is <Drawer>.
 *
 *     dashboardxml                    opens the window
 *     dashboardxml --shot out.png     draws it offscreen, software renderer
 *     dashboardxml other.xml          reads another document
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_DASHBOARD_XML_PATH
#  define CUI_DASHBOARD_XML_PATH "dashboard.xml"
#endif

/* ---- the orders the table shows ----------------------------------------- */

typedef struct { const char *id, *customer, *email, *date, *status; int tone; float amount; } order;

static const order g_orders[] = {
    { "#3021", "Nora Lindqvist", "nora@lindqvist.se", "Nov 5", "Paid", CUI_TONE_SUCCESS, 184.20f },
    { "#3020", "Kwame Asante", "kwame@asante.co", "Nov 5", "Pending", CUI_TONE_WARNING, 42.75f },
    { "#3019", "Hana Novak", "hana@novak.cz", "Nov 4", "Paid", CUI_TONE_SUCCESS, 318.40f },
    { "#3018", "Felix Moreau", "felix@moreau.fr", "Nov 4", "Refunded", CUI_TONE_DANGER, 96.00f },
    { "#3017", "Iris Coleman", "iris@coleman.org", "Nov 3", "Paid", CUI_TONE_SUCCESS, 471.15f },
    { "#3016", "Ravi Shah", "ravi@shah.in", "Nov 3", "Paid", CUI_TONE_SUCCESS, 1236.80f },
};
#define ORDER_COUNT ((int)(sizeof g_orders / sizeof g_orders[0]))

/* ---- the app's values: the document binds them --------------------------- */

typedef struct {
    int nav;
    int range;
    char search[64];
    int page;
    cui_table_sort sort;
    int selected;
    bool dark;
    bool palette;
    bool drawer;     /* the navigation drawer on a narrow window */
    bool narrow;
    bool two;        /* two figures per line instead of four in a strip */
} dashboard_xml_state;

static dashboard_xml_state g;

/* the four figures of the three ranges, as examples/dashboard.c has them */
static const char *const g_revenue[] = { "$15,830", "$64,210", "$702,480" };
static const char *const g_order_count[] = { "318", "1,286", "14,903" };
static const char *const g_conversion[] = { "4.3%", "4.1%", "3.8%" };
static const char *const g_refunds[] = { "1.5%", "1.7%", "2.0%" };
static const char *const g_since[] = { "on last week", "on October", "on last year" };

/* what the document shows as text, computed once a frame */
static char g_change[2][48];

/* ---- the state `dashboard --shot` draws --------------------------------- */

static void seed(dashboard_xml_state *s) {
    memset(s, 0, sizeof *s);
    s->sort.column = 0;
    s->sort.descending = true;
    s->selected = 0;
    s->range = 1;
}

/* ---- attributes, as the tags read them ---------------------------------- */

static float dash_num(cui_xml *doc, cui_xml_node *node, const char *name, float fallback) {
    const char *v = cui_xml_node_attr_value(doc, node, name);
    return v && *v ? (float)atof(v) : fallback;
}

/* ---- <Orders/>: the table with the app's rows --------------------------- */

static bool tag_orders(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_table_column cols[] = {
        { "Order", CUI_PX(90), CUI_TEXT_LEFT, true },
        { "Customer", CUI_FILL, CUI_TEXT_LEFT, true },
        { "Date", CUI_PX(90), CUI_TEXT_LEFT, false },
        { "Status", CUI_PX(120), CUI_TEXT_LEFT, true },
        { "Amount", CUI_PX(110), CUI_TEXT_RIGHT, true },
    };
    int i;
    (void)doc;
    (void)node;
    (void)user;
    cui_table(ui, "orders", cols, 5, .sort = &g.sort, .selectable = true) {
        for (i = 0; i < ORDER_COUNT; i++) {
            const order *o = &g_orders[i];
            if (g.search[0] && !strstr(o->customer, g.search) && !strstr(o->id, g.search)) continue;
            if (cui_table_row(ui, g.selected == i)) g.selected = i;
            cui_table_cell_text(ui, o->id);
            cui_table_cell(ui);
            cui_avatar(ui, o->customer, .size = 28);
            cui_column(ui, .gap = 0, .width = CUI_FILL) {
                cui_text(ui, o->customer, .font = CUI_FONT_MEDIUM, .nowrap = true);
                cui_text(ui, o->email, .size = 12, .color = CUI_COLOR_TEXT_SECONDARY, .nowrap = true);
            }
            cui_table_cell_text(ui, o->date);
            cui_table_cell(ui);
            cui_badge(ui, o->status, .tone = (cui_tone)o->tone, .dot = true);
            cui_table_cell_textf(ui, "$%.2f", (double)o->amount);
        }
    }
    return true;
}

/* ---- <NumberRoll Label="Revenue" Value="{revenue}"/> -------------------- */

static bool tag_number_roll(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    static const char *const fonts[] = { "body", "medium", "display", "mono" };
    const char *label = cui_xml_node_attr_value(doc, node, "label");
    const char *value = cui_xml_node_attr_value(doc, node, "value");
    const char *font = cui_xml_node_attr_value(doc, node, "font");
    int kind = 0, i;
    (void)user;
    for (i = 0; i < 4; i++)
        if (font && !strcmp(font, fonts[i])) kind = i;
    /* the label is the roll's id, the way examples/dashboard.c passes it */
    cui_number_roll(ui, label, value, .font = (cui_font)kind, .size = dash_num(doc, node, "size", 0));
    return true;
}

/* ---- <Drawer Text="Larkspur Supply" Side="left" Size="280"> ------------- */

static bool tag_drawer(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    static const char *const sides[] = { "right", "left", "bottom", "top" };
    cui_sheet_opts o;
    const char *side = cui_xml_node_attr_value(doc, node, "side");
    int kind = CUI_SIDE_LEFT, i;
    (void)user;
    for (i = 0; i < 4; i++)
        if (side && !strcmp(side, sides[i])) kind = i;
    memset(&o, 0, sizeof o);
    o.side = (cui_side)kind;
    o.size = dash_num(doc, node, "size", 400);
    if (cui_sheet_begin(ui, cui_xml_node_attr_value(doc, node, "text"), &g.drawer, &o)) {
        cui_xml_draw_children(doc, node, ui);
        cui_sheet_end(ui);
    }
    return true;
}

/* ---- actions ------------------------------------------------------------ */

static void on_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    if (!strncmp(action, "nav:", 4)) {
        int i = atoi(action + 4);
        if (i >= 0 && i < 6) {
            g.nav = i;
            g.drawer = false;
        }
    } else if (!strcmp(action, "palette")) {
        g.palette = true;
    } else if (!strcmp(action, "drawer")) {
        g.drawer = true;
    } else if (!strcmp(action, "toggle-theme")) {
        g.dark = !g.dark;
    }
}

/* ---- the values the document shows as text ------------------------------ */

/* The figures of the chosen range, the two sentences under them, and the three
 * facts of the window: a narrow one, a strip too narrow for four figures, and a
 * dark theme. examples/dashboard.c computes them in its frame. */
static void values(cui_xml *doc, cui_ctx *ui) {
    int k = g.range;
    float room;
    if (k < 0 || k > 2) k = 1;
    g.narrow = ui && cui_viewport_size(ui).x < 760;
    room = (ui ? cui_viewport_size(ui).x : 0) - (g.narrow ? 32 : 297);
    g.two = room < 720;
    snprintf(g_change[0], sizeof g_change[0], "%s %s", k == 2 ? "+31.0%" : k ? "+12.4%" : "+3.1%", g_since[k]);
    snprintf(g_change[1], sizeof g_change[1], "%s %s", k == 2 ? "+22.6%" : k ? "+7.3%" : "+1.9%", g_since[k]);
    cui_xml_set_var(doc, "narrow", g.narrow ? "1" : "0");
    cui_xml_set_var(doc, "two", g.two ? "1" : "0");
    cui_xml_set_var(doc, "dark", g.dark ? "1" : "0");
    cui_xml_set_var(doc, "revenue", g_revenue[k]);
    cui_xml_set_var(doc, "orders", g_order_count[k]);
    cui_xml_set_var(doc, "conversion", g_conversion[k]);
    cui_xml_set_var(doc, "refunds", g_refunds[k]);
    cui_xml_set_var(doc, "change1", g_change[0]);
    cui_xml_set_var(doc, "change2", g_change[1]);
    cui_xml_set_var(doc, "conversionChange", k == 2 ? "+0.6 pts" : k ? "+0.4 pts" : "+0.2 pts");
    cui_xml_set_var(doc, "refundChange",
                    k == 2 ? "-0.3 pts, fewer returns" : k ? "-0.6 pts, fewer returns" : "-0.2 pts, fewer returns");
}

/* ---- wiring ------------------------------------------------------------- */

static cui_xml *open_document(const char *path) {
    cui_xml *doc = cui_xml_load(path);
    if (!doc) return NULL;
    cui_xml_bind_int(doc, "nav", &g.nav);
    cui_xml_bind_int(doc, "range", &g.range);
    cui_xml_bind_text(doc, "search", g.search, sizeof g.search);
    cui_xml_bind_int(doc, "page", &g.page);
    cui_xml_set_var(doc, "narrow", "0");
    cui_xml_set_var(doc, "two", "0");
    cui_xml_set_var(doc, "dark", "0");
    cui_xml_set_var(doc, "revenue", g_revenue[1]);
    cui_xml_set_var(doc, "orders", g_order_count[1]);
    cui_xml_set_var(doc, "conversion", g_conversion[1]);
    cui_xml_set_var(doc, "refunds", g_refunds[1]);
    cui_xml_set_var(doc, "change1", "");
    cui_xml_set_var(doc, "change2", "");
    cui_xml_set_var(doc, "conversionChange", "");
    cui_xml_set_var(doc, "refundChange", "");
    cui_xml_on_any(doc, on_action, NULL);
    cui_xml_register_tag(doc, "Orders", tag_orders, NULL);
    cui_xml_register_tag(doc, "NumberRoll", tag_number_roll, NULL);
    cui_xml_register_tag(doc, "Drawer", tag_drawer, NULL);
    return doc;
}

/* The theme the account toggles lives in the document, so it is applied here
 * when it moved, which is what the button in examples/dashboard.c does. */
static void choices(cui_ctx *ui) {
    static bool dark = false;
    if (g.dark == dark) return;
    dark = g.dark;
    cui_set_theme(ui, dark ? CUI_THEME_DARK : CUI_THEME_LIGHT);
}

/* The commands of the palette are the app's data and a palette has no tag, so
 * it is drawn after the document, in the frame its shortcut opened it. */
static void palette(cui_ctx *ui) {
    static const cui_command cmds[] = {
        { "Create order", CUI_ICON_PLUS, "Actions", "Ctrl N" },
        { "Add customer", CUI_ICON_USER_PLUS, "Actions", NULL },
        { "Export report", CUI_ICON_DOWNLOAD, "Actions", NULL },
        { "Go to analytics", CUI_ICON_CHART_BAR, "Navigation", NULL },
    };
    int c = cui_command_palette(ui, &g.palette, cmds, 4);
    if (c >= 0) cui_toast(ui, cmds[c].title, .tone = CUI_TONE_SUCCESS);
}

/* One frame: the values, the window and what floats over it. The dashboard has
 * no clock of its own, so the shot and the app run this and nothing else. */
static void frame(cui_ctx *ui, cui_xml *doc) {
    values(doc, ui);
    if (!g.narrow) g.drawer = false;
    choices(ui);
    cui_xml_render(doc, ui);
    palette(ui);
}

/* The frame the app draws, as cui_app_run calls it. */
static void dashboard_xml_frame(cui_ctx *ui, void *user) {
    cui_xml *doc = user;
    frame(ui, doc);
    cui_xml_poll(doc);
    if (cui_xml_quit_requested(doc)) cui_app_quit(NULL);
}

/* `dashboardxml --shot out.png`: the same window drawn offscreen with the
 * software renderer, with the viewport, the theme, the accent and the six
 * frames of examples/dashboard.c, so the two pictures can be compared. */
static int write_shot(cui_xml *doc, const char *out_path) {
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui;
    const cui_draw_data *draw;
    unsigned char *pixels;
    int width = 0, height = 0, i;
    if (!sw) return 1;
    {
        /* the theme of the document from the start, so the shot is the window
         * the app would open: a theme change animates, and a picture of the
         * middle of that transition shows neither theme. */
        cui_app_desc d = cui_xml_app_desc(doc);
        ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = d.theme,
                                         .accent = d.accent });
    }
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
    const char *path = CUI_DASHBOARD_XML_PATH;
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
        int rc = cui_app_run(&desc, dashboard_xml_frame, doc);
        cui_xml_free(doc);
        return rc;
    }
}
