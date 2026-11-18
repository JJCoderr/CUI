#define _CRT_SECURE_NO_WARNINGS 1

/* examples/finance.c, with its interface in examples/finance.xml.
 *
 * The figures are the app's data, as in the C example: the six transactions
 * with their merchant, category, icon, tone and amount, and the painted card
 * the account is on. The document holds the window, the top bar, the three rows
 * of cards, the two charts, the four budgets, the three goals and the card
 * panel; what a document cannot say is here: <Txns>, the rows of the table,
 * because an amount is formatted and a row is trimmed by the window's width,
 * and <CardArt>, the payment card, because a canvas is app paint. The command
 * palette is drawn after the document: its commands are the app's data too.
 *
 *     financexml                    opens the window
 *     financexml --shot out.png     draws it offscreen, software renderer
 *     financexml other.xml          reads another document
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_FINANCE_XML_PATH
#  define CUI_FINANCE_XML_PATH "finance.xml"
#endif

/* ---- the transactions the table shows ----------------------------------- */

typedef struct { const char *merchant, *category, *icon, *when; float amount; cui_tone tone; } finance_txn;

static const finance_txn g_txns[] = {
    { "Salary", "Income", CUI_ICON_BRIEFCASE, "Today, 09:12", 6200.00f, CUI_TONE_SUCCESS },
    { "Blue Bottle", "Coffee", CUI_ICON_GIFT, "Today, 08:40", -6.50f, CUI_TONE_WARNING },
    { "Fieldnotes Rent", "Housing", CUI_ICON_BUILDING_2, "Yesterday", -1850.00f, CUI_TONE_INFO },
    { "Night Market", "Groceries", CUI_ICON_SHOPPING_CART, "Yesterday", -84.20f, CUI_TONE_TEAL },
    { "Cloud Storage", "Subscriptions", CUI_ICON_CLOUD, "Nov 3", -9.99f, CUI_TONE_VIOLET },
    { "Transfer from Sam", "Transfer", CUI_ICON_ARROW_DOWN, "Nov 2", 120.00f, CUI_TONE_SUCCESS },
};
#define FINANCE_TXN_COUNT ((int)(sizeof g_txns / sizeof g_txns[0]))

/* ---- the app's values: the document binds them --------------------------- */

typedef struct {
    int tab, range;
    bool frozen, palette;
    char search[64];
    bool narrow;     /* the window is too narrow for the tabs beside the brand */
} finance_state;

static finance_state g;

/* ---- the state `finance --shot` draws ----------------------------------- */

/* The overview the app opens with: the third range chosen, the first tab, the
 * card unfrozen. examples/finance.c seeds the same values. */
static void seed(finance_state *s) {
    memset(s, 0, sizeof *s);
    s->range = 2;
}

/* ---- the payment card: examples/finance.c's paint_card ------------------ */

typedef struct { bool frozen; } finance_card_paint;

static void paint_finance_card(cui_ctx *ui, cui_rect r, void *user) {
    finance_card_paint *c = user;
    cui_corners rad = CUI_CORNERS(18);
    cui_draw_shadow(ui, r, rad, 32, -6, (cui_vec2){ 0, 16 }, CUI_RGBA(0x00000080));
    cui_draw_rect_gradient(ui, r, rad, CUI_RGB(0x1f6f5c), CUI_RGB(0x0b2a24));
    cui_push_clip(ui, r);
    /* printed rings, like the guilloche on a real card; no glow */
    cui_draw_arc(ui, (cui_vec2){ r.x + r.w * 0.92f, r.y + r.h * 1.05f }, r.h * 0.75f, 0, 6.2831853f, 1, CUI_RGBA(0xffffff1c));
    cui_draw_arc(ui, (cui_vec2){ r.x + r.w * 0.92f, r.y + r.h * 1.05f }, r.h * 0.55f, 0, 6.2831853f, 1, CUI_RGBA(0xffffff14));
    cui_pop_clip(ui);
    cui_draw_rect_border(ui, r, rad, CUI_TRANSPARENT, CUI_RGBA(0xffffff22), 1);

    cui_color white = CUI_RGB(0xffffff), soft = CUI_RGBA(0xffffffb0);
    cui_draw_text(ui, CUI_FONT_DISPLAY, 17, (cui_vec2){ r.x + 24, r.y + 22 }, white, "Halden", -1);
    cui_draw_icon(ui, CUI_ICON_WIFI, (cui_rect){ r.x + r.w - 48, r.y + 16, 28, 28 }, 18, soft);
    /* chip */
    cui_rect chip = { r.x + 24, r.y + r.h * 0.40f, 40, 30 };
    cui_draw_rect_gradient(ui, chip, CUI_CORNERS(6), CUI_RGB(0xf3d58a), CUI_RGB(0xb8913f));
    cui_draw_line(ui, (cui_vec2){ chip.x, chip.y + 15 }, (cui_vec2){ chip.x + 40, chip.y + 15 }, 1, CUI_RGBA(0x00000040));
    cui_draw_line(ui, (cui_vec2){ chip.x + 14, chip.y }, (cui_vec2){ chip.x + 14, chip.y + 30 }, 1, CUI_RGBA(0x00000040));
    cui_draw_line(ui, (cui_vec2){ chip.x + 26, chip.y }, (cui_vec2){ chip.x + 26, chip.y + 30 }, 1, CUI_RGBA(0x00000040));
    cui_draw_text(ui, CUI_FONT_MONO, 17, (cui_vec2){ r.x + 24, r.y + r.h - 64 }, white, "4821  ••••  ••••  0937", -1);
    cui_draw_text(ui, CUI_FONT_BODY, 12, (cui_vec2){ r.x + 24, r.y + r.h - 34 }, soft, "MARA QUINN", -1);
    cui_draw_text(ui, CUI_FONT_BODY, 12, (cui_vec2){ r.x + r.w * 0.5f, r.y + r.h - 34 }, soft, "09 / 29", -1);
    cui_draw_text(ui, CUI_FONT_MEDIUM, 13, (cui_vec2){ r.x + r.w - 66, r.y + r.h - 40 }, white, "DEBIT", -1);
    if (c->frozen) {
        cui_draw_rect(ui, r, rad, CUI_RGBA(0x0a0f0ec0));
        cui_draw_icon(ui, CUI_ICON_LOCK, r, 28, white);
    }
}

/* ---- <CardArt/>: the payment card --------------------------------------- */

static bool finance_tag_card_art(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    finance_card_paint c = { g.frozen };
    (void)doc;
    (void)node;
    (void)user;
    (void)cui_canvas(ui, "card", CUI_FILL, CUI_PX(200), paint_finance_card, &c, sizeof c);
    return true;
}

/* ---- the tinted square an icon sits in ---------------------------------- */

/* examples/finance.c's tone_icon with its table: the token a tone names, at 14%
 * behind the icon, and the icon in that token too. */
static const cui_token finance_tone_tokens[] = {
    CUI_TOKEN_TEXT_SECONDARY, CUI_TOKEN_INFO, CUI_TOKEN_SUCCESS, CUI_TOKEN_WARNING,
    CUI_TOKEN_DANGER, CUI_TOKEN_SERIES_2, CUI_TOKEN_SERIES_3, CUI_TOKEN_SERIES_4,
    CUI_TOKEN_ACCENT
};

static void finance_tone_icon(cui_ctx *ui, const char *icon, cui_tone tone) {
    cui_color c = cui_token_color(ui, finance_tone_tokens[tone]);
    cui_box(ui, .width = CUI_PX(38), .height = CUI_PX(38), .radius = 12, .bg = cui_alpha(c, 0.14f),
            .dir = CUI_STACK, .align = CUI_ALIGN_CENTER, .justify = CUI_JUSTIFY_CENTER) {
        cui_icon(ui, icon, 16, c);
    }
}

/* ---- <Txns/>: the rows of the table ------------------------------------- */

/* Copied from examples/finance.c rather than described: which rows there are,
 * what they say, how an amount is formatted and which of them loses its
 * category on a narrow window are the app's data. */
static bool finance_tag_txns(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    int i;
    (void)doc;
    (void)node;
    (void)user;
    for (i = 0; i < FINANCE_TXN_COUNT; i++) {
        const finance_txn *t = &g_txns[i];
        char amount[32];
        cui_push_id_int(ui, i);
        cui_row(ui, .width = CUI_FILL, .gap = 14, .pad_y = 7, .align = CUI_ALIGN_CENTER) {
            finance_tone_icon(ui, t->icon, t->tone);
            cui_column(ui, .gap = 1, .width = CUI_FILL) {
                cui_text(ui, t->merchant, .font = CUI_FONT_MEDIUM, .nowrap = true);
                cui_text(ui, t->when, .size = 12, .color = CUI_COLOR_TEXT_MUTED, .nowrap = true);
            }
            /* a phone keeps the merchant and the amount */
            if (cui_viewport_size(ui).x >= 560) cui_badge(ui, t->category);
            snprintf(amount, sizeof amount, "%s$%.2f", t->amount < 0 ? "-" : "+",
                     (double)(t->amount < 0 ? -t->amount : t->amount));
            cui_text(ui, amount, .font = CUI_FONT_MEDIUM, .tabular = true, .width = CUI_PX(110),
                     .align = CUI_TEXT_RIGHT,
                     .color = t->amount > 0 ? CUI_COLOR_SUCCESS : CUI_COLOR_FOREGROUND);
        }
        cui_pop_id(ui);
    }
    return true;
}

/* ---- actions ------------------------------------------------------------ */

static void finance_on_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    if (!strcmp(action, "palette")) g.palette = true;
}

/* ---- the values the document shows as text ------------------------------ */

/* What a frame knows before the document is drawn: whether the window is narrow
 * enough that the tabs go on a line of their own. examples/finance.c computes
 * the same in topbar. */
static void values(cui_xml *doc, cui_ctx *ui) {
    if (!ui) return;
    g.narrow = cui_viewport_size(ui).x < 900;
    cui_xml_set_var(doc, "narrow", g.narrow ? "1" : "0");
}

/* ---- wiring ------------------------------------------------------------- */

static cui_xml *open_document(const char *path) {
    cui_xml *doc = cui_xml_load(path);
    if (!doc) return NULL;
    /* the fields the document holds, and the ones {"..."} reads */
    cui_xml_bind_int(doc, "nav", &g.tab);
    cui_xml_bind_int(doc, "range", &g.range);
    cui_xml_bind_bool(doc, "frozen", &g.frozen);
    cui_xml_bind_text(doc, "search", g.search, sizeof g.search);
    cui_xml_set_var(doc, "narrow", "0");
    cui_xml_on_any(doc, finance_on_action, NULL);
    cui_xml_register_tag(doc, "CardArt", finance_tag_card_art, NULL);
    cui_xml_register_tag(doc, "Txns", finance_tag_txns, NULL);
    return doc;
}

/* The commands of the palette are the app's data and a palette has no tag, so
 * it is drawn after the document, in the frame its shortcut opened it. */
static void palette(cui_ctx *ui) {
    static const cui_command cmds[] = {
        { "Send money", CUI_ICON_SEND, "Actions", NULL },
        { "Freeze card", CUI_ICON_LOCK, "Actions", NULL },
        { "Download statement", CUI_ICON_DOWNLOAD, "Actions", NULL },
    };
    int c = cui_command_palette(ui, &g.palette, cmds, 3);
    if (c == 1) g.frozen = !g.frozen;
    else if (c >= 0) cui_toast(ui, cmds[c].title, .tone = CUI_TONE_SUCCESS);
}

/* One frame: the values, the window and what floats over it. The finance app
 * has no clock of its own, so the shot and the app run this and nothing else. */
static void frame(cui_ctx *ui, cui_xml *doc) {
    values(doc, ui);
    cui_xml_render(doc, ui);
    palette(ui);
}

/* The frame the app draws, as cui_app_run calls it. */
static void finance_xml_frame(cui_ctx *ui, void *user) {
    cui_xml *doc = user;
    frame(ui, doc);
    cui_xml_poll(doc);
    if (cui_xml_quit_requested(doc)) cui_app_quit(NULL);
}

/* `financexml --shot out.png`: the same window drawn offscreen with the software
 * renderer, with the viewport, the theme, the accent and the six frames of
 * examples/finance.c, so the two pictures can be compared. */
static int write_shot(cui_xml *doc, const char *out_path) {
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui;
    const cui_draw_data *draw;
    unsigned char *pixels;
    int width = 0, height = 0, i;
    if (!sw) return 1;
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = CUI_THEME_DARK,
                                     .accent = CUI_ACCENT_GREEN });
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
    const char *path = CUI_FINANCE_XML_PATH;
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
        int rc = cui_app_run(&desc, finance_xml_frame, doc);
        cui_xml_free(doc);
        return rc;
    }
}
