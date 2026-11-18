#define _CRT_SECURE_NO_WARNINGS 1

/* examples/notes.c, with its interface in examples/notes.xml.
 *
 * The notes are the app's data, as in the C example: four of them, the first
 * pinned, with their tag and the two fields of the editor. The document holds
 * the split, the list's header, the search, the filter, the open note and the
 * dialog; the rows of the list are <Notes>, registered here, because a list
 * that grows, is pinned and is searched is the app's data, not a document's.
 *
 *     notesxml                    opens the window
 *     notesxml --shot out.png     draws it offscreen, software renderer
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_NOTES_XML_PATH
#  define CUI_NOTES_XML_PATH "notes.xml"
#endif

#define MAX_NOTES 64

typedef struct {
    char title[96];
    char body[2048];
    bool pinned;
    int tag;
} note;

typedef struct {
    note notes[MAX_NOTES];
    int count;
    int selected;
    char query[64];        /* what the Search holds */
    int filter;            /* 0 all, then the tag plus one */
    bool confirm_delete;   /* the "Delete this note?" dialog is open */
} notes_state;

static notes_state g;

/* the editor's two fields: the document binds them, and the text of the note
 * that is open is copied in and out of them, which is what the C example gets
 * for free by pointing the fields at the note itself */
static char g_title[96];
static char g_body[2048];

/* what the document shows as text, computed once a frame */
static char g_status[64];

static const char *tags[] = { "Personal", "Work", "Ideas" };

/* ---- the notes, as examples/notes.c has them ---------------------------- */

static bool matches(const note *n, const char *q) {
    char hay[2200], needle[64];
    if (!q[0]) return true;
    snprintf(hay, sizeof hay, "%s %s", n->title, n->body);
    snprintf(needle, sizeof needle, "%s", q);
    for (char *p = hay; *p; p++) *p = (char)tolower((unsigned char)*p);
    for (char *p = needle; *p; p++) *p = (char)tolower((unsigned char)*p);
    return strstr(hay, needle) != NULL;
}

static void add_note(notes_state *a, const char *title, const char *body, int tag) {
    note *n;
    if (a->count >= MAX_NOTES) return;
    n = &a->notes[a->count];
    memset(n, 0, sizeof *n);
    snprintf(n->title, sizeof n->title, "%s", title);
    snprintf(n->body, sizeof n->body, "%s", body);
    n->tag = tag;
    a->selected = a->count++;
}

static void delete_selected(notes_state *a) {
    if (a->selected < 0 || a->selected >= a->count) return;
    memmove(&a->notes[a->selected], &a->notes[a->selected + 1],
            sizeof(note) * (size_t)(a->count - a->selected - 1));
    a->count--;
    if (a->selected >= a->count) a->selected = a->count - 1;
}

/* the note that is open goes into the fields, and the fields go back into it */
static void show_selection(void) {
    if (g.selected < 0 || g.selected >= g.count) return;
    snprintf(g_title, sizeof g_title, "%s", g.notes[g.selected].title);
    snprintf(g_body, sizeof g_body, "%s", g.notes[g.selected].body);
}

static void take_editor_text(void) {
    if (g.selected < 0 || g.selected >= g.count) return;
    snprintf(g.notes[g.selected].title, sizeof g.notes[g.selected].title, "%s", g_title);
    snprintf(g.notes[g.selected].body, sizeof g.notes[g.selected].body, "%s", g_body);
}

/* the one action of the menu that the app itself does: the note as a text file */
static void export_note(void) {
    char path[1024], name[128];
    const note *n;
    FILE *f;
    if (g.selected < 0 || g.selected >= g.count) return;
    n = &g.notes[g.selected];
    snprintf(name, sizeof name, "%s.txt", n->title[0] ? n->title : "note");
    if (!cui_app_file_dialog(NULL, &(cui_file_dialog_opts){ .kind = CUI_FILE_SAVE, .title = "Export note",
                                                           .filter_name = "Text", .filter = "txt",
                                                           .default_name = name },
                             path, sizeof path))
        return;
    f = fopen(path, "wb");
    /* a toast says the file was written; a document has no tag for one */
    if (f) { fprintf(f, "%s\n\n%s\n", n->title, n->body); fclose(f); }
}

/* ---- <Notes>: the rows of the list -------------------------------------- */

static bool tag_notes(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    int shown = 0;
    (void)doc;
    (void)node;
    (void)user;
    /* the pinned ones first, then the rest: the two passes of the C example */
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < g.count; i++) {
            note *n = &g.notes[i];
            char preview[160];
            int w = 0;
            if ((pass == 0) != n->pinned) continue;
            if (g.filter > 0 && n->tag != g.filter - 1) continue;
            if (!matches(n, g.query)) continue;
            /* one line of the note: its lines joined with a dot, list markers dropped */
            if (!n->body[0]) w = snprintf(preview, sizeof preview, "No content");
            for (const char *p = n->body; *p && w < (int)sizeof preview - 8;) {
                const char *e = strchr(p, '\n');
                int len = e ? (int)(e - p) : (int)strlen(p);
                const char *q = p;
                while (len > 0 && (*q == '-' || *q == '*' || *q == ' ')) { q++; len--; }
                if (len > 0) w += snprintf(preview + w, sizeof preview - (size_t)w, "%s%.*s",
                                           w ? "  \xc2\xb7  " : "", len, q);
                if (!e) break;
                p = e + 1;
            }
            cui_push_id_int(ui, i);
            if (cui_list_item(ui, n->title[0] ? n->title : "Untitled", .subtitle = preview,
                              .selected = g.selected == i,
                              .icon = n->pinned ? CUI_ICON_BOOKMARK : CUI_ICON_FILE_TEXT)) {
                g.selected = i;
                show_selection();
            }
            cui_context_menu(ui, "note-menu") {
                if (cui_menu_item(ui, n->pinned ? "Unpin" : "Pin to top", .icon = CUI_ICON_BOOKMARK))
                    n->pinned = !n->pinned;
                if (cui_menu_item(ui, "Duplicate", .icon = CUI_ICON_COPY)) {
                    add_note(&g, n->title, n->body, n->tag);
                    show_selection();
                }
                cui_menu_separator(ui);
                if (cui_menu_item(ui, "Delete", .icon = CUI_ICON_TRASH_2, .danger = true)) {
                    g.selected = i;
                    g.confirm_delete = true;
                }
            }
            cui_pop_id(ui);
            shown++;
        }
    }
    if (shown == 0)
        cui_empty_state(ui, "No notes found", .icon = CUI_ICON_SEARCH,
                        .description = "Try another search or create a note.");
    return true;
}

/* ---- actions ------------------------------------------------------------ */

static void on_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    if (!strcmp(action, "new")) {
        add_note(&g, "Untitled", "", g.filter > 0 ? g.filter - 1 : 0);
        show_selection();
    } else if (!strcmp(action, "pin")) {
        if (g.selected >= 0 && g.selected < g.count) g.notes[g.selected].pinned = !g.notes[g.selected].pinned;
    } else if (!strncmp(action, "tag:", 4)) {
        int t = atoi(action + 4);
        if (g.selected >= 0 && g.selected < g.count && t >= 0 && t < 3) g.notes[g.selected].tag = t;
    } else if (!strcmp(action, "export")) {
        export_note();
    } else if (!strcmp(action, "delete")) {
        g.confirm_delete = true;
    } else if (!strcmp(action, "cancel-delete")) {
        g.confirm_delete = false;
    } else if (!strcmp(action, "delete-confirmed")) {
        delete_selected(&g);
        show_selection();
        g.confirm_delete = false;
    }
}

/* ---- wiring ------------------------------------------------------------- */

static cui_xml *open_document(const char *path) {
    cui_xml *doc = cui_xml_load(path);
    if (!doc) return NULL;
    cui_xml_set_var(doc, "tagName", "");
    cui_xml_set_var(doc, "tagTone", "neutral");
    cui_xml_set_var(doc, "pinLabel", "");
    cui_xml_set_var(doc, "pinTooltip", "Pin");
    cui_xml_set_var(doc, "pinColor", "transparent");
    cui_xml_set_var(doc, "pinned", "0");
    cui_xml_set_var(doc, "tagIs0", "0");
    cui_xml_set_var(doc, "tagIs1", "0");
    cui_xml_set_var(doc, "tagIs2", "0");
    cui_xml_set_var(doc, "status", "");
    cui_xml_bind_text(doc, "query", g.query, sizeof g.query);
    cui_xml_bind_int(doc, "filter", &g.filter);
    cui_xml_bind_text(doc, "title", g_title, sizeof g_title);
    cui_xml_bind_text(doc, "body", g_body, sizeof g_body);
    cui_xml_bind_bool(doc, "confirmDelete", &g.confirm_delete);
    cui_xml_register_tag(doc, "Notes", tag_notes, NULL);
    cui_xml_on_any(doc, on_action, NULL);
    show_selection();
    return doc;
}

static void values(cui_xml *doc) {
    note *n;
    int words = 0;
    bool in_word = false;
    take_editor_text();
    if (g.selected < 0 || g.selected >= g.count) return;
    n = &g.notes[g.selected];
    cui_xml_set_var(doc, "tagName", tags[n->tag]);
    cui_xml_set_var(doc, "tagTone", n->tag == 1 ? "info" : n->tag == 2 ? "violet" : "neutral");
    cui_xml_set_var(doc, "pinLabel", n->pinned ? "Pinned" : "");
    cui_xml_set_var(doc, "pinTooltip", n->pinned ? "Unpin" : "Pin");
    cui_xml_set_var(doc, "pinColor", n->pinned ? "accent" : "transparent");
    cui_xml_set_var(doc, "pinned", n->pinned ? "1" : "0");
    cui_xml_set_var(doc, "tagIs0", n->tag == 0 ? "1" : "0");
    cui_xml_set_var(doc, "tagIs1", n->tag == 1 ? "1" : "0");
    cui_xml_set_var(doc, "tagIs2", n->tag == 2 ? "1" : "0");
    /* a word has a letter or digit in it, so the dashes of a list do not count */
    for (const char *p = n->body; *p; p++) {
        bool w = !isspace((unsigned char)*p);
        if (!w) { in_word = false; continue; }
        if (!in_word && isalnum((unsigned char)*p)) { words++; in_word = true; }
    }
    snprintf(g_status, sizeof g_status, "%d words  \xc2\xb7  %d characters", words, (int)strlen(n->body));
    cui_xml_set_var(doc, "status", g_status);
}

/* The frame the app draws, as cui_app_run calls it. */
static void notes_xml_frame(cui_ctx *ui, void *user) {
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
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw), .theme = CUI_THEME_LIGHT,
                                     .accent = CUI_ACCENT_AMBER });
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
    const char *path = CUI_NOTES_XML_PATH;
    const char *shot = NULL;
    cui_xml *doc;
    int arg = 1;

    /* the notes `notes --shot` draws: four, the first pinned, the first open */
    add_note(&g, "Launch checklist", "Ship the 1.0 release.\n\n- Final pass on docs\n- Record the demo video\n- Write the announcement post\n- Celebrate", 1);
    add_note(&g, "Reading list", "The Design of Everyday Things\nA Pattern Language\nThe Art of Computer Programming", 0);
    add_note(&g, "App ideas", "A calm habit tracker with springy check marks. A tiny music player that lives in the menu bar.", 2);
    add_note(&g, "Meeting notes", "Discussed the roadmap for Q1. Focus on accessibility, keyboard support and docs.", 1);
    g.notes[0].pinned = true;
    g.selected = 0;

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
        int rc = cui_app_run(&desc, notes_xml_frame, doc);
        cui_xml_free(doc);
        return rc;
    }
}
