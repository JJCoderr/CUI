#define _CRT_SECURE_NO_WARNINGS 1

/* examples/mail.c, with its interface in examples/mail.xml.
 *
 * The inbox is the app's data, as in the C example: the six messages with
 * their sender, subject, preview, time, body and label, the four folders, the
 * three labels, the search, the open message and the compose fields. The
 * document holds the window, the two splits, the folders pane, the list's
 * header, the reading pane's toolbar, the reader and the dialog; what a
 * document cannot say is here: <Messages>, the rows of the list, because a
 * list that is searched, selected and painted by a canvas is the app's data,
 * and <Drawer>, the sheet a narrow window opens the folders in.
 *
 *     mailxml                    opens the window
 *     mailxml --shot out.png     draws it offscreen, software renderer
 *     mailxml other.xml          reads another document
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_MAIL_XML_PATH
#  define CUI_MAIL_XML_PATH "mail.xml"
#endif

/* ---- the app's state ---------------------------------------------------- */

typedef struct {
    const char *from, *email, *subject, *preview, *time, *body;
    int folder;            /* 0 inbox, 1 starred, 2 sent, 3 archive */
    bool unread, starred;
    int label;             /* -1 none, else tone */
} mail_message;

static mail_message g_messages[] = {
    { "Jonas Berg", "jonas@northlight.io", "Review of the onboarding flow", "I went through all six screens on a 13 inch laptop. Step three scrolls before the button is visible.", "9:41",
      "Hi Maya,\n\nI went through all six screens on a 13 inch laptop. Step three scrolls before the button is visible, and the skip link reads like an error.\n\nCould we look at it together on Thursday at 10?\n\nJonas", 0, true, true, CUI_TONE_INFO },
    { "Priya Nair", "priya@harbor.dev", "Invoice 2024-118", "The October invoice is attached. Payment terms are 30 days as usual.", "8:15",
      "Hi,\n\nThe October invoice is attached. Payment terms are 30 days as usual. Let me know if the purchase order number needs to change.\n\nPriya", 0, true, false, -1 },
    { "Tomas Rivera", "tomas@kiln.app", "Export is slow for large projects", "A 900 file project takes about four minutes to export on my machine.", "Yesterday",
      "A 900 file project takes about four minutes to export on my machine, most of it in the thumbnail step. I attached a trace.\n\nTomas", 0, false, true, CUI_TONE_WARNING },
    { "Lena Fischer", "lena@fischer.dev", "Re: Release notes draft", "Looks good. I tightened the second paragraph and fixed two links.", "Yesterday",
      "Looks good. I tightened the second paragraph and fixed two links that pointed at the old docs.\n\nLena", 0, false, false, CUI_TONE_SUCCESS },
    { "Aiko Tanaka", "aiko@tanaka.design", "Icons for the settings page", "Here are the twelve icons at 16 and 20 px, outline only.", "Mon",
      "Here are the twelve icons at 16 and 20 px, outline only, on the same 1.5 px stroke as the rest of the set.\n\nAiko", 0, false, false, CUI_TONE_VIOLET },
    { "Sam Whitfield", "sam@whitfield.co", "Office closed on Friday", "The building is closed for maintenance. Work from home if you can.", "Mon",
      "The building is closed for maintenance on Friday. Work from home if you can; the server room stays reachable over the VPN.\n\nSam", 3, false, true, -1 },
};
#define MAIL_MESSAGE_COUNT ((int)(sizeof g_messages / sizeof g_messages[0]))

static const char *const g_folder_names[] = { "Inbox", "Starred", "Sent", "Archive" };

typedef struct {
    int folder, selected;
    char query[64];        /* what the Search holds */
    bool compose;
    char to[96], subject[128], body[1024];
    int panes;             /* 3, 2 or 1, from the window width */
    bool reading;          /* one pane: the message is open */
    bool drawer;           /* folders in a drawer below three panes */
} mail_state;

static mail_state g;

/* The frame being drawn: the two actions that toast need the context, and an
 * action reports through the document, not through the frame. */
static cui_ctx *g_frame_ui;

/* ---- the state `mail --shot` draws -------------------------------------- */

/* The inbox the app opens with: the inbox, the first message open. The split
 * ratios are in the document, which owns them. examples/mail.c seeds the same
 * message as read. */
static void seed(mail_state *s) {
    memset(s, 0, sizeof *s);
    s->folder = 0;
    s->selected = 0;
    g_messages[0].unread = false;
}

/* ---- the list's filter, as examples/mail.c has it ----------------------- */

static bool mail_match(const mail_message *m, const char *q) {
    char hay[1024], needle[64];
    if (!q[0]) return true;
    snprintf(hay, sizeof hay, "%s %s %s", m->from, m->subject, m->preview);
    snprintf(needle, sizeof needle, "%s", q);
    for (char *p = hay; *p; p++) *p = (char)tolower((unsigned char)*p);
    for (char *p = needle; *p; p++) *p = (char)tolower((unsigned char)*p);
    return strstr(hay, needle) != NULL;
}

static bool mail_in_folder(const mail_message *m, int folder) {
    if (folder == 1) return m->starred;
    return m->folder == folder;
}

static int mail_unread_in(int folder) {
    int unread = 0;
    for (int i = 0; i < MAIL_MESSAGE_COUNT; i++)
        if (mail_in_folder(&g_messages[i], folder) && g_messages[i].unread) unread++;
    return unread;
}

/* the tone of a label as the document writes it */
static const char *mail_tone_name(int tone) {
    static const char *const names[] = { "neutral", "info", "success", "warning", "danger", "violet", "teal", "rose", "accent" };
    if (tone < 0 || tone >= (int)(sizeof names / sizeof names[0])) return "neutral";
    return names[tone];
}

/* ---- attributes, as the tags read them ---------------------------------- */

static bool mail_word_eq(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static float mail_attr_num(cui_xml *doc, cui_xml_node *node, const char *name, float fallback) {
    const char *v = cui_xml_node_attr_value(doc, node, name);
    return v && *v ? (float)atof(v) : fallback;
}

/* ---- the rows of the list ---------------------------------------------- */

/* examples/mail.c's row_bg and paint_row: a quiet fill when the row is
 * selected or hovered, a hairline between the others. */
typedef struct { bool hovered, selected, last; } mail_row_bg;

static void paint_mail_row(cui_ctx *ui, cui_rect r, void *user) {
    mail_row_bg *b = user;
    if (b->selected) cui_draw_rect(ui, r, CUI_CORNERS(10), cui_token_color(ui, CUI_TOKEN_ACTIVE));
    else if (b->hovered) cui_draw_rect(ui, r, CUI_CORNERS(10), cui_token_color(ui, CUI_TOKEN_HOVER));
    else if (!b->last) cui_draw_rect(ui, (cui_rect){ r.x + 26, r.y + r.h - 1, r.w - 38, 1 }, CUI_CORNERS(0), cui_token_color(ui, CUI_TOKEN_BORDER_SUBTLE));
}

/* ---- <Messages/>: the rows of the open folder --------------------------- */

/* Copied from examples/mail.c rather than described: which rows there are,
 * what they say, which one is open and what a row's menu does are the app's
 * data. The row is a canvas painted behind the name, subject and preview, and
 * a context menu on right click. */
static bool tag_messages(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    int shown = 0;
    (void)doc;
    (void)node;
    (void)user;
    for (int i = 0; i < MAIL_MESSAGE_COUNT; i++) {
        mail_message *m = &g_messages[i];
        int next;
        if (!mail_in_folder(m, g.folder) || !mail_match(m, g.query)) continue;
        shown++;
        cui_push_id_int(ui, i);
        next = i + 1;
        while (next < MAIL_MESSAGE_COUNT && (!mail_in_folder(&g_messages[next], g.folder) || !mail_match(&g_messages[next], g.query))) next++;
        cui_box(ui, .dir = CUI_STACK, .width = CUI_FILL) {
            bool *hover = cui_state(ui, cui_make_id(ui, "bg"), sizeof(bool));
            mail_row_bg b = { *hover, g.selected == i, next >= MAIL_MESSAGE_COUNT || g.selected == next };
            cui_canvas_result res = cui_canvas(ui, "bg", CUI_FILL, CUI_FILL, paint_mail_row, &b, sizeof b);
            *hover = res.hovered;
            if (res.clicked) { g.selected = i; m->unread = false; g.reading = true; }
            /* every row keeps the dot's gutter, so names line up read or unread */
            cui_row(ui, .width = CUI_FILL, .pad_y = 12, .pad_right = 12, .align = CUI_ALIGN_START) {
                cui_box(ui, .width = CUI_PX(26), .pad_top = 6, .align = CUI_ALIGN_CENTER) {
                    if (m->unread) cui_box(ui, .width = CUI_PX(7), .height = CUI_PX(7), .radius = 4, .bg = CUI_COLOR_ACCENT) {}
                }
                cui_column(ui, .gap = 3, .width = CUI_FILL) {
                    cui_row(ui, .gap = 8, .width = CUI_FILL) {
                        cui_text(ui, m->from, .font = m->unread ? CUI_FONT_MEDIUM : CUI_FONT_BODY, .nowrap = true, .width = CUI_FILL);
                        cui_text(ui, m->time, .size = 12, .color = CUI_COLOR_TEXT_MUTED, .nowrap = true);
                    }
                    cui_text(ui, m->subject, .font = CUI_FONT_MEDIUM, .size = 12.5f, .nowrap = true);
                    cui_text(ui, m->preview, .size = 12, .color = CUI_COLOR_TEXT_SECONDARY, .nowrap = true);
                }
            }
        }
        cui_context_menu(ui, "row-menu") {
            if (cui_menu_item(ui, m->unread ? "Mark as read" : "Mark as unread", .icon = CUI_ICON_MAIL)) m->unread = !m->unread;
            if (cui_menu_item(ui, m->starred ? "Unstar" : "Star", .icon = CUI_ICON_STAR)) m->starred = !m->starred;
            if (cui_menu_item(ui, "Archive", .icon = CUI_ICON_ARCHIVE)) m->folder = 3;
        }
        cui_pop_id(ui);
    }
    if (!shown) cui_empty_state(ui, "No messages", .icon = CUI_ICON_INBOX, .description = "You are all caught up.");
    return true;
}

/* ---- <Drawer Text="Mail" Side="left" Size="280"> ------------------------ */

static bool tag_drawer(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    static const char *const sides[] = { "right", "left", "bottom", "top" };
    cui_sheet_opts o;
    char title[64];
    const char *side;
    int i, kind = CUI_SIDE_LEFT;
    (void)user;
    memset(&o, 0, sizeof o);
    o.size = mail_attr_num(doc, node, "size", 400);
    side = cui_xml_node_attr_value(doc, node, "side");
    for (i = 0; i < 4; i++)
        if (side[0] && mail_word_eq(side, sides[i])) kind = i;
    o.side = (cui_side)kind;
    snprintf(title, sizeof title, "%s", cui_xml_node_attr_value(doc, node, "text"));
    if (cui_sheet_begin(ui, title, &g.drawer, &o)) {
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
    if (!strcmp(action, "compose")) {
        g.compose = true;
        g.drawer = false;
    } else if (!strncmp(action, "folder:", 7)) {
        int f = atoi(action + 7);
        if (f >= 0 && f < 4) {
            g.folder = f;
            g.selected = -1;
            g.drawer = g.reading = false;
        }
    } else if (!strcmp(action, "drawer")) {
        g.drawer = true;
    } else if (!strcmp(action, "reading-off")) {
        g.reading = false;
    } else if (!strcmp(action, "archive")) {
        if (g.selected >= 0) {
            g_messages[g.selected].folder = 3;
            g.selected = -1;
            if (g_frame_ui) cui_toast(g_frame_ui, "Archived", .action = "Undo");
        }
    } else if (!strcmp(action, "star")) {
        if (g.selected >= 0) g_messages[g.selected].starred = !g_messages[g.selected].starred;
    } else if (!strcmp(action, "reply")) {
        if (g.selected >= 0) {
            const mail_message *m = &g_messages[g.selected];
            g.compose = true;
            snprintf(g.to, sizeof g.to, "%s", m->email);
            snprintf(g.subject, sizeof g.subject, "Re: %s", m->subject);
        }
    } else if (!strcmp(action, "discard")) {
        g.compose = false;
        g.to[0] = g.subject[0] = g.body[0] = 0;
    } else if (!strcmp(action, "send")) {
        g.compose = false;
        if (g_frame_ui)
            cui_toast(g_frame_ui, "Message sent", .description = g.subject[0] ? g.subject : "No subject",
                      .tone = CUI_TONE_SUCCESS);
        g.to[0] = g.subject[0] = g.body[0] = 0;
    }
}

/* ---- the values the document shows as text ------------------------------ */

/* What the frame knows before the document is drawn: the pane count of the
 * window's width, the open folder, its unread count and the open message.
 * examples/mail.c computes the same in its frame. */
static void values(cui_xml *doc, cui_ctx *ui) {
    char number[32];
    float vw;
    int unread;
    const mail_message *m;
    if (!ui) return;
    vw = cui_viewport_size(ui).x;
    g.panes = vw < 720 ? 1 : vw < 1100 ? 2 : 3;
    if (g.panes == 3) g.drawer = false;
    snprintf(number, sizeof number, "%d", g.panes);
    cui_xml_set_var(doc, "panes", number);
    cui_xml_set_var(doc, "narrow", g.panes < 3 ? "1" : "0");
    cui_xml_set_var(doc, "three", g.panes == 3 ? "1" : "0");
    cui_xml_set_var(doc, "two", g.panes == 2 ? "1" : "0");
    /* one pane: the open message and the list take turns */
    cui_xml_set_var(doc, "oneReader", g.panes == 1 && g.reading && g.selected >= 0 ? "1" : "0");
    cui_xml_set_var(doc, "oneList", g.panes == 1 && !(g.reading && g.selected >= 0) ? "1" : "0");
    unread = mail_unread_in(0);
    snprintf(number, sizeof number, "%d", unread);
    cui_xml_set_var(doc, "inboxBadge", unread ? number : "");
    snprintf(number, sizeof number, "%d", g.folder);
    cui_xml_set_var(doc, "folder", number);
    cui_xml_set_var(doc, "folderName", g_folder_names[g.folder]);
    cui_xml_set_var(doc, "has", g.selected >= 0 ? "1" : "0");
    cui_xml_set_var(doc, "none", g.selected < 0 ? "1" : "0");
    if (g.selected < 0) return;
    m = &g_messages[g.selected];
    cui_xml_set_var(doc, "starred", m->starred ? "1" : "0");
    cui_xml_set_var(doc, "starTooltip", m->starred ? "Unstar" : "Star");
    cui_xml_set_var(doc, "starColor", m->starred ? "warning" : "transparent");
    cui_xml_set_var(doc, "hasLabel", m->label >= 0 ? "1" : "0");
    cui_xml_set_var(doc, "labelText", m->label < 0 ? "" : m->label == CUI_TONE_INFO ? "Work"
                                     : m->label == CUI_TONE_VIOLET ? "Personal"
                                     : m->label == CUI_TONE_WARNING ? "Urgent" : "Merged");
    cui_xml_set_var(doc, "labelTone", mail_tone_name(m->label));
    cui_xml_set_var(doc, "sender", m->from);
    cui_xml_set_var(doc, "senderEmail", m->email);
    cui_xml_set_var(doc, "subjectText", m->subject);
    cui_xml_set_var(doc, "timeText", m->time);
    cui_xml_set_var(doc, "bodyText", m->body);
}

/* ---- wiring ------------------------------------------------------------- */

static cui_xml *open_document(const char *path) {
    cui_xml *doc = cui_xml_load(path);
    if (!doc) return NULL;
    /* the fields the document holds, and the ones {"..."} reads */
    cui_xml_bind_text(doc, "query", g.query, sizeof g.query);
    cui_xml_bind_bool(doc, "compose", &g.compose);
    cui_xml_bind_text(doc, "to", g.to, sizeof g.to);
    cui_xml_bind_text(doc, "subject", g.subject, sizeof g.subject);
    cui_xml_bind_text(doc, "body", g.body, sizeof g.body);
    cui_xml_set_var(doc, "panes", "3");
    cui_xml_set_var(doc, "narrow", "0");
    cui_xml_set_var(doc, "three", "1");
    cui_xml_set_var(doc, "two", "0");
    cui_xml_set_var(doc, "oneReader", "0");
    cui_xml_set_var(doc, "oneList", "0");
    cui_xml_set_var(doc, "inboxBadge", "");
    cui_xml_set_var(doc, "folder", "0");
    cui_xml_set_var(doc, "folderName", g_folder_names[0]);
    cui_xml_set_var(doc, "has", "1");
    cui_xml_set_var(doc, "none", "0");
    cui_xml_set_var(doc, "starred", "0");
    cui_xml_set_var(doc, "starTooltip", "Star");
    cui_xml_set_var(doc, "starColor", "transparent");
    cui_xml_set_var(doc, "hasLabel", "0");
    cui_xml_set_var(doc, "labelText", "");
    cui_xml_set_var(doc, "labelTone", "neutral");
    cui_xml_set_var(doc, "sender", "");
    cui_xml_set_var(doc, "senderEmail", "");
    cui_xml_set_var(doc, "subjectText", "");
    cui_xml_set_var(doc, "timeText", "");
    cui_xml_set_var(doc, "bodyText", "");
    cui_xml_on_any(doc, on_action, NULL);
    cui_xml_register_tag(doc, "Messages", tag_messages, NULL);
    cui_xml_register_tag(doc, "Drawer", tag_drawer, NULL);
    return doc;
}

/* One frame: the values, then the window. The mail app has no clock of its
 * own, so the shot and the app run this and nothing else. */
static void frame(cui_ctx *ui, cui_xml *doc) {
    g_frame_ui = ui;
    values(doc, ui);
    cui_xml_render(doc, ui);
}

/* The frame the app draws, as cui_app_run calls it. */
static void mail_xml_frame(cui_ctx *ui, void *user) {
    cui_xml *doc = user;
    frame(ui, doc);
    cui_xml_poll(doc);
    if (cui_xml_quit_requested(doc)) cui_app_quit(NULL);
}

/* `mailxml --shot out.png`: the same window drawn offscreen with the software
 * renderer, with the viewport, the theme, the accent and the six frames of
 * examples/mail.c, so the two pictures can be compared. */
static int write_shot(cui_xml *doc, const char *out_path) {
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui;
    const cui_draw_data *draw;
    unsigned char *pixels;
    int width = 0, height = 0, i;
    if (!sw) return 1;
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = CUI_THEME_LIGHT,
                                     .accent = CUI_ACCENT_BLUE });
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
    const char *path = CUI_MAIL_XML_PATH;
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
        int rc = cui_app_run(&desc, mail_xml_frame, doc);
        cui_xml_free(doc);
        return rc;
    }
}
