#define _CRT_SECURE_NO_WARNINGS 1

/* examples/settings.c, with its interface in examples/settings.xml.
 *
 * The account is the app's data, as in the C example: one struct with the
 * profile, the choices, the open section and what is unsaved. The document
 * binds those fields and draws the sections; here are the few things it cannot
 * say -- the name and the sentence of the open section, the error under the
 * user name, whether the delete button in the dialog is enabled, what a change
 * does to the window (the theme, the accent, the text size, the reduced motion
 * setting) and the toasts.
 *
 *     settingsxml                    opens the window
 *     settingsxml --shot out.png     draws it offscreen, software renderer
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_SETTINGS_XML_PATH
#  define CUI_SETTINGS_XML_PATH "settings.xml"
#endif

typedef struct {
    int section;
    char first[64], last[64], email[96], username[48], bio[400], website[96];
    int timezone, language;
    bool email_comments, email_mentions, email_digest, push_all, push_mentions, desktop;
    int theme, density;
    int accent;
    float font_scale;
    bool reduced_motion;
    bool confirm_delete;
    char confirm_text[64];
    bool dirty;
} settings_xml_state;

static settings_xml_state g;

/* the sections, in the order the document and the C example name them */
static const char *section_names[] = { "Profile", "Notifications", "Appearance", "Account" };
static const char *section_descs[] = { "How others see you.", "Choose what you hear about.",
                                       "Make it yours.", "Security and data." };

/* a phone: the sections become tabs, and the padding gets tighter */
static bool g_narrow;

/* a toast waits for the frame it is queued in, which is where a document has no
 * element for it: 0 none, 1 saved, 2 the account scheduled for deletion */
static int g_toast;

/* ---- actions ------------------------------------------------------------ */

static void settings_xml_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    (void)node;
    (void)user;
    if (!strncmp(action, "section:", 8)) {
        int i = atoi(action + 8);
        if (i >= 0 && i < 4) g.section = i;
    } else if (!strcmp(action, "edit")) {
        g.dirty = true;
    } else if (!strcmp(action, "discard")) {
        g.dirty = false;
    } else if (!strcmp(action, "save")) {
        g.dirty = false;
        g_toast = 1;
    } else if (!strcmp(action, "ask-delete")) {
        g.confirm_delete = true;
        g.confirm_text[0] = 0;
        /* a Dialog drawn closed stays out of the frames that follow until it is
         * shown again, so opening it from a bound value says so as well */
        cui_xml_node_show(cui_xml_find(doc, "confirm"), true);
    } else if (!strcmp(action, "cancel-delete")) {
        g.confirm_delete = false;
    } else if (!strcmp(action, "delete-confirmed")) {
        g.confirm_delete = false;
        g_toast = 2;
    }
}

/* ---- wiring ------------------------------------------------------------- */

static cui_xml *settings_xml_document(const char *path) {
    cui_xml *doc = cui_xml_load(path);
    if (!doc) return NULL;
    cui_xml_bind_text(doc, "first", g.first, sizeof g.first);
    cui_xml_bind_text(doc, "last", g.last, sizeof g.last);
    cui_xml_bind_text(doc, "email", g.email, sizeof g.email);
    cui_xml_bind_text(doc, "username", g.username, sizeof g.username);
    cui_xml_bind_text(doc, "website", g.website, sizeof g.website);
    cui_xml_bind_text(doc, "bio", g.bio, sizeof g.bio);
    cui_xml_bind_text(doc, "confirmText", g.confirm_text, sizeof g.confirm_text);
    cui_xml_bind_int(doc, "section", &g.section);
    cui_xml_bind_int(doc, "timezone", &g.timezone);
    cui_xml_bind_int(doc, "language", &g.language);
    cui_xml_bind_int(doc, "theme", &g.theme);
    cui_xml_bind_int(doc, "density", &g.density);
    cui_xml_bind_int(doc, "accent", &g.accent);
    cui_xml_bind_float(doc, "fontScale", &g.font_scale);
    cui_xml_bind_bool(doc, "emailComments", &g.email_comments);
    cui_xml_bind_bool(doc, "emailMentions", &g.email_mentions);
    cui_xml_bind_bool(doc, "emailDigest", &g.email_digest);
    cui_xml_bind_bool(doc, "pushAll", &g.push_all);
    cui_xml_bind_bool(doc, "pushMentions", &g.push_mentions);
    cui_xml_bind_bool(doc, "desktop", &g.desktop);
    cui_xml_bind_bool(doc, "reducedMotion", &g.reduced_motion);
    cui_xml_bind_bool(doc, "confirmDelete", &g.confirm_delete);
    cui_xml_bind_bool(doc, "dirty", &g.dirty);
    cui_xml_bind_bool(doc, "narrow", &g_narrow);
    cui_xml_set_var(doc, "sectionName", section_names[0]);
    cui_xml_set_var(doc, "sectionDesc", section_descs[0]);
    cui_xml_set_var(doc, "userError", "");
    cui_xml_set_var(doc, "deleteDisabled", "1");
    cui_xml_on_any(doc, settings_xml_action, NULL);
    return doc;
}

/* What the document shows as text, computed once a frame: the name of the open
 * section, the sentence under it, the error under the user name, whether the
 * dialog's delete button may fire, and how wide the window is. */
static void settings_xml_values(cui_xml *doc, cui_ctx *ui) {
    int section = g.section;
    if (section < 0 || section > 3) section = 0;
    g_narrow = ui && cui_viewport_size(ui).x < 760;
    cui_xml_set_var(doc, "sectionName", section_names[section]);
    cui_xml_set_var(doc, "sectionDesc", section_descs[section]);
    cui_xml_set_var(doc, "userError", strchr(g.username, ' ') ? "Usernames cannot contain spaces." : "");
    cui_xml_set_var(doc, "deleteDisabled", strcmp(g.confirm_text, "DELETE") != 0 ? "1" : "0");
}

/* The four choices of the appearance section act on the window in the C
 * example; a document holds the value, so it is applied here when it moved. */
static void settings_xml_choices(cui_ctx *ui) {
    static int theme = CUI_THEME_LIGHT;
    static int accent = CUI_ACCENT_NEUTRAL;
    static float scale = 1.0f;
    static bool motion = false;
    if (g.theme != theme) {
        theme = g.theme;
        cui_set_theme(ui, (cui_theme_mode)theme);
    }
    if (g.accent != accent) {
        accent = g.accent;
        cui_set_accent(ui, (cui_accent)accent);
    }
    if (g.font_scale != scale) {
        scale = g.font_scale;
        cui_theme_get(ui)->font_scale = scale;
    }
    if (g.reduced_motion != motion) {
        motion = g.reduced_motion;
        cui_set_reduced_motion(ui, motion);
    }
}

/* The two toasts the C example raises, drawn in the frame their action ran in. */
static void settings_xml_toasts(cui_ctx *ui) {
    if (g_toast == 1) cui_toast(ui, "Settings saved", .tone = CUI_TONE_SUCCESS);
    else if (g_toast == 2)
        cui_toast(ui, "Account scheduled for deletion",
                  .description = "You have 14 days to change your mind.", .tone = CUI_TONE_DANGER);
    g_toast = 0;
}

/* The account `settings --shot` draws, and the one the app opens with: the
 * profile filled in, Berlin time, two kinds of mail and the mention push on,
 * text at its normal size, nothing unsaved. examples/settings.c seeds the same
 * values. */
static void settings_xml_seed(settings_xml_state *s) {
    memset(s, 0, sizeof *s);
    snprintf(s->first, sizeof s->first, "Maya");
    snprintf(s->last, sizeof s->last, "Okafor");
    snprintf(s->email, sizeof s->email, "maya@fieldwork.studio");
    snprintf(s->username, sizeof s->username, "maya.okafor");
    snprintf(s->website, sizeof s->website, "https://fieldwork.studio");
    snprintf(s->bio, sizeof s->bio, "Product designer in Berlin. Currently on onboarding and the settings pages.");
    s->timezone = 3;
    s->email_comments = s->email_mentions = true;
    s->push_mentions = true;
    s->font_scale = 1.0f;
}

/* The frame the app draws, as cui_app_run calls it. */
static void settings_xml_frame(cui_ctx *ui, void *user) {
    cui_xml *doc = user;
    settings_xml_values(doc, ui);
    cui_xml_poll(doc);
    settings_xml_choices(ui);
    cui_xml_render(doc, ui);
    settings_xml_toasts(ui);
    if (cui_xml_quit_requested(doc)) cui_app_quit(NULL);
}

/* `settingsxml --shot out.png`: the same window drawn offscreen with the
 * software renderer, with the viewport, the theme and the six frames of
 * examples/settings.c, so the two pictures can be compared. */
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
        settings_xml_values(doc, ui);
        cui_begin_frame(ui, 1.0f / 60.0f);
        settings_xml_choices(ui);
        cui_xml_render(doc, ui);
        settings_xml_toasts(ui);
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
    const char *path = CUI_SETTINGS_XML_PATH;
    const char *shot = NULL;
    cui_xml *doc;
    int arg = 1;

    settings_xml_seed(&g);
    if (argc > 2 && !strcmp(argv[1], "--shot")) {
        shot = argv[2];
        arg = 3;
    }
    if (argc > arg) path = argv[arg];
    doc = settings_xml_document(path);
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
        int rc = cui_app_run(&desc, settings_xml_frame, doc);
        cui_xml_free(doc);
        return rc;
    }
}
