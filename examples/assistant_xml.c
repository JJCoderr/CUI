#define _CRT_SECURE_NO_WARNINGS 1

/* examples/assistant.c, with its interface in examples/assistant.xml.
 *
 * The conversation is the app's data, as in the C example: the seven chats of
 * the sidebar, the two files waiting in the composer, the models the picker
 * offers, the sources a citation opens, the plan's four steps and the scripted
 * answer. The document holds the window, the sidebar, the top row, the message
 * list, the composer's footer and the workspace panel; what a document cannot
 * say is here: <Answer> (an answer's reasoning, tool call, text, citations and
 * actions, which stream while one plays), <ChatMessage>, <ChatComposer>,
 * <ModelPicker>, <ContextMeter>, <Plan>, <PromptSuggestions>, <UserMenu> and
 * <Drawer>.
 *
 *     assistantxml                    opens the window
 *     assistantxml --shot out.png     draws it offscreen, software renderer
 *     assistantxml other.xml          reads another document
 */
#include <cui/cui_app.h>
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CUI_ASSISTANT_XML_PATH
#  define CUI_ASSISTANT_XML_PATH "assistant.xml"
#endif

/* ---- the app's state ----------------------------------------------------- */

typedef struct {
    int chat;
    char draft[2048];
    char asked[2048];
    int model;
    int vote;
    double started;          /* when the current answer began, < 0 idle */
    bool asked_once;
    const char *files[4];
    int file_count;
    int removed;
    bool narrow;            /* a phone-sized window */
    bool drawer;            /* the chat list on a narrow window */
} assistant_state;

static assistant_state g;

/* The models, the sources and the answer are the same data examples/assistant.c
 * has; the tool call's request and reply are not attributes a document can
 * carry without quoting JSON inside XML, so they live here too. */
static const cui_model g_models[] = {
    { "Atlas", "Longer answers, reads files and code", "New", CUI_ICON_LAYERS },
    { "Atlas Fast", "Short answers in about a second", NULL, CUI_ICON_ZAP },
    { "Atlas Mini", "Runs on your own machine", NULL, CUI_ICON_BOX },
};

static const cui_source g_sources[] = {
    { "Immediate mode GUIs and why they keep coming back", "https://fieldnotes.dev/immediate-mode", "Retained and immediate mode toolkits and the trade offs between them." },
    { "Notes on subpixel text positioning", "https://glyphlab.io/notes/subpixel", "Shaping, hinting and subpixel positioning explained with examples." },
    { "Spring physics for interface motion", "https://motionbook.dev/springs", "Why spring physics feel more natural than cubic beziers for interface motion." },
};

static const char *g_answer =
    "Mostly because they do very little per frame. The interface is rebuilt from your data each frame, laid out "
    "in one pass and drawn in a handful of batches, so there is no tree of widget objects to keep in sync.\n\n"
    "Text is the expensive part, which is why glyphs are rasterized once into an atlas and reused. And motion "
    "feels quick when it is driven by springs that respond to input at once, instead of fixed length curves.";

static const cui_viz_item g_parts[] = { { "Instructions", 3200 }, { "Files", 41800 }, { "Messages", 68400 } };

static const cui_prompt_suggestion g_sugg[] = {
    { CUI_ICON_CHART_BAR, "Compare with last quarter", "Revenue, churn and costs" },
    { CUI_ICON_LIST_CHECKS, "Turn this into a checklist", NULL },
};

static const char *const g_tool_input = "{\"query\": \"immediate mode gui performance\", \"limit\": 5}";
static const char *const g_tool_output = "{\"results\": 3, \"took_ms\": 1380}";

/* ---- the state `assistant --shot` draws ---------------------------------- */

static void seed(assistant_state *s) {
    memset(s, 0, sizeof *s);
    s->started = -1;
    s->removed = -1;
    s->files[0] = "quarterly-report.pdf";
    s->files[1] = "metrics.csv";
    s->file_count = 2;
    snprintf(s->draft, sizeof s->draft, "Now compare these numbers with last quarter.");
}

/* ---- attributes, as the tags read them ---------------------------------- */

static bool xml_word_eq(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static float xml_num(cui_xml *doc, cui_xml_node *node, const char *name, float fallback) {
    const char *v = cui_xml_node_attr_value(doc, node, name);
    return v && *v ? (float)atof(v) : fallback;
}

static bool xml_flag(cui_xml *doc, cui_xml_node *node, const char *name, bool fallback) {
    const char *v = cui_xml_node_attr_value(doc, node, name);
    if (!v || !*v) return fallback;
    if (xml_word_eq(v, "true") || xml_word_eq(v, "yes") || xml_word_eq(v, "on") || !strcmp(v, "1")) return true;
    if (xml_word_eq(v, "false") || xml_word_eq(v, "no") || xml_word_eq(v, "off") || !strcmp(v, "0")) return false;
    return fallback;
}

/* ---- the answer, the way examples/assistant.c draws it ------------------ */

/* One answer: the reasoning that folds away, the tool call it made, the text,
 * the citations and the buttons under it. `t` counts the seconds since the
 * answer began; `live` streams it, so at rest both programs draw the same
 * pieces with the same values. Copied from examples/assistant.c rather than
 * described: an answer is the app's data. */
static void answer_block(cui_ctx *ui, const char *key, float t, bool live) {
    cui_push_id(ui, key);
    bool thinking = live && t < 2.6f;
    cui_reasoning(ui, "think", .thinking = thinking, .seconds = live ? (thinking ? t : 2.6f) : 6) {
        cui_text(ui, "This is about perceived speed. Two parts: how little work a frame does, and how fast input reaches the screen.",
                 .size = 13.5f, .color = CUI_COLOR_TEXT_SECONDARY);
        cui_text(ui, "Find sources on retained against immediate mode, text rendering cost and interface motion.", .size = 13.5f, .color = CUI_COLOR_TEXT_SECONDARY);
    }
    if (!live || t > 1.2f) {
        cui_task_status st = live && t < 3.6f ? CUI_TASK_RUNNING : CUI_TASK_DONE;
        cui_tool_call(ui, "search", "web_search", .icon = CUI_ICON_GLOBE, .detail = "immediate mode gui performance", .status = st,
                      .seconds = st == CUI_TASK_DONE ? 1.4f : t - 1.2f,
                      .input = g_tool_input,
                      .output = g_tool_output);
    }
    if (live && t > 2.6f && t < 4.2f) cui_agent_status(ui, "Reading 3 sources", .status = CUI_TASK_RUNNING, .seconds = t - 2.6f);
    if (!live || t > 3.6f) {
        if (live) cui_typewriter(ui, "answer", g_answer, .size = 15, .speed = 140, .caret = true);
        else cui_text(ui, g_answer, .size = 15);
        if (!live || t > 6.5f) {
            cui_row(ui, .gap = 6, .align = CUI_ALIGN_CENTER) {
                for (int k = 0; k < 3; k++) cui_citation(ui, k + 1, &g_sources[k]);
            }
            cui_source_list(ui, "sources", g_sources, 3);
            int act = cui_response_actions(ui, "actions", .copy_text = g_answer, .vote = &g.vote);
            if (act & CUI_RESPONSE_REGENERATE) g.started = cui_time(ui);
            if (act & CUI_RESPONSE_SHARE) cui_toast(ui, "Link copied", .tone = CUI_TONE_SUCCESS);
        }
    }
    cui_pop_id(ui);
}

/* ---- <Answer Name="first" Live="false"/> -------------------------------- */

static bool tag_answer(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    char key[64];
    bool live;
    float t;
    (void)user;
    live = xml_flag(doc, node, "live", false);
    snprintf(key, sizeof key, "%s", cui_xml_node_name(node));
    t = g.started >= 0 ? (float)(cui_time(ui) - g.started) : 99;
    answer_block(ui, key[0] ? key : "answer", t, live);
    return true;
}

/* ---- <ChatMessage Text="..." Mine="true"/> ------------------------------ */

static bool tag_chat_message(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_chat_opts o;
    const char *text;
    (void)user;
    memset(&o, 0, sizeof o);
    o.mine = xml_flag(doc, node, "mine", false);
    text = cui_xml_node_attr_value(doc, node, "text");
    if (!text[0]) text = cui_xml_node_text(doc, node);
    cui_chat_message_ex(ui, text, &o);
    return true;
}

/* ---- <ModelPicker Ghost="true" Placement="bottomstart"/> ---------------- */

static bool tag_model_picker(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    static const char *const places[] = { "bottom", "bottomstart", "bottomend", "top", "topstart", "topend",
                                         "rightstart", "leftstart", "right", "left" };
    cui_model_picker_opts o;
    const char *place;
    int i, kind = CUI_PLACE_BOTTOM_START;
    (void)user;
    memset(&o, 0, sizeof o);
    o.ghost = xml_flag(doc, node, "ghost", false);
    place = cui_xml_node_attr_value(doc, node, "placement");
    for (i = 0; i < 10; i++)
        if (place[0] && xml_word_eq(place, places[i])) kind = i;
    o.placement = (cui_placement)kind;
    cui_model_picker_ex(ui, "model", g_models, 3, &g.model, &o);
    return true;
}

/* ---- <ContextMeter Used="113400" Limit="200000"/> ----------------------- */

static bool tag_context_meter(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_context_opts o;
    (void)user;
    memset(&o, 0, sizeof o);
    o.parts = g_parts;
    o.part_count = 3;
    cui_context_meter_ex(ui, "ctx", xml_num(doc, node, "used", 0), xml_num(doc, node, "limit", 0), &o);
    return true;
}

/* ---- <Plan Title="Plan"/> ----------------------------------------------- */

static bool tag_plan(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_agent_plan_opts o;
    cui_plan_step steps[4];
    char title[48];
    float t;
    (void)user;
    t = g.asked_once && g.started >= 0 ? (float)(cui_time(ui) - g.started) : 99;
    memset(&o, 0, sizeof o);
    snprintf(title, sizeof title, "%s", cui_xml_node_attr_value(doc, node, "title"));
    o.title = title[0] ? title : NULL;
    steps[0] = (cui_plan_step){ "Search for sources", "3 results", t > 3.6f ? CUI_TASK_DONE : CUI_TASK_RUNNING };
    steps[1] = (cui_plan_step){ "Read the sources", t > 4.2f ? "3 of 3" : "Reading",
                                t > 4.2f ? CUI_TASK_DONE : t > 2.6f ? CUI_TASK_RUNNING : CUI_TASK_PENDING };
    steps[2] = (cui_plan_step){ "Write the answer", NULL,
                                t > 7.5f ? CUI_TASK_DONE : t > 3.6f ? CUI_TASK_RUNNING : CUI_TASK_PENDING };
    steps[3] = (cui_plan_step){ "Check the citations", NULL, t > 7.5f ? CUI_TASK_DONE : CUI_TASK_PENDING };
    cui_agent_plan_ex(ui, "plan", steps, 4, &o);
    return true;
}

/* ---- <PromptSuggestions Columns="1"/> ----------------------------------- */

static bool tag_prompt_suggestions(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_prompt_suggestions_opts o;
    int pick;
    (void)user;
    memset(&o, 0, sizeof o);
    o.columns = (int)xml_num(doc, node, "columns", 0);
    pick = cui_prompt_suggestions_ex(ui, "sugg", g_sugg, 2, &o);
    if (pick >= 0) snprintf(g.draft, sizeof g.draft, "%s", g_sugg[pick].title);
    return true;
}

/* ---- <ChatComposer Placeholder="Ask a follow up" Busy="{busy}"/> -------- */

static bool tag_chat_composer(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_composer_opts o;
    char placeholder[64];
    int ev;
    (void)user;
    memset(&o, 0, sizeof o);
    o.busy = xml_flag(doc, node, "busy", false);
    o.attachments = g.files;
    o.attachment_count = g.file_count;
    o.removed = &g.removed;
    o.models = g_models;
    o.model_count = 3;
    o.model = &g.model;
    snprintf(placeholder, sizeof placeholder, "%s", cui_xml_node_attr_value(doc, node, "placeholder"));
    o.placeholder = placeholder[0] ? placeholder : NULL;
    ev = cui_chat_composer_ex(ui, "composer", g.draft, sizeof g.draft, &o);
    if (ev & CUI_COMPOSER_SEND) {
        snprintf(g.asked, sizeof g.asked, "%s", g.draft);
        g.draft[0] = 0;
        g.asked_once = true;
        g.started = cui_time(ui);
    }
    if (ev & CUI_COMPOSER_STOP) g.started = cui_time(ui) - 99;
    if ((ev & CUI_COMPOSER_REMOVE) && g.removed >= 0 && g.removed < g.file_count) {
        for (int k = g.removed; k < g.file_count - 1; k++) g.files[k] = g.files[k + 1];
        g.file_count--;
    }
    if (ev & CUI_COMPOSER_ATTACH) cui_toast(ui, "Drop files on the window to attach them");
    return true;
}

/* ---- <UserMenu Text="Mara Quinn" Detail="mara@muse.dev" Status="online"> */

static bool tag_user_menu(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    cui_user_menu_opts o;
    char name[64], detail[96], status[24];
    (void)user;
    memset(&o, 0, sizeof o);
    o.compact = xml_flag(doc, node, "compact", false);
    snprintf(detail, sizeof detail, "%s", cui_xml_node_attr_value(doc, node, "detail"));
    snprintf(status, sizeof status, "%s", cui_xml_node_attr_value(doc, node, "status"));
    o.detail = detail[0] ? detail : NULL;
    o.status = status[0] ? status : NULL;
    snprintf(name, sizeof name, "%s", cui_xml_node_attr_value(doc, node, "text"));
    if (cui_user_menu_begin(ui, name, &o)) {
        cui_xml_draw_children(doc, node, ui);
        cui_user_menu_end(ui);
    }
    return true;
}

/* ---- <Drawer Text="Muse" Side="left" Size="300"> ------------------------ */

static bool tag_drawer(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    static const char *const sides[] = { "right", "left", "bottom", "top" };
    cui_sheet_opts o;
    char title[64];
    const char *side;
    int i, kind = CUI_SIDE_LEFT;
    (void)user;
    memset(&o, 0, sizeof o);
    o.size = xml_num(doc, node, "size", 400);
    side = cui_xml_node_attr_value(doc, node, "side");
    for (i = 0; i < 4; i++)
        if (side[0] && xml_word_eq(side, sides[i])) kind = i;
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
    if (!strcmp(action, "new-chat")) {
        g.asked_once = false;
        g.started = -1;
        g.draft[0] = 0;
    } else if (!strncmp(action, "chat:", 5)) {
        int i = atoi(action + 5);
        if (i >= 0 && i < 7) g.chat = i;
        g.drawer = false;
    } else if (!strcmp(action, "drawer")) {
        g.drawer = true;
    }
}

/* ---- the values the document shows as text ------------------------------ */

/* What the frame knows before the document is drawn: a phone-sized window, the
 * workspace panel's line at 1180 points, and whether an answer is playing.
 * examples/assistant.c computes the same three in its frame. */
static void values(cui_xml *doc, cui_ctx *ui) {
    float vw = ui ? cui_viewport_size(ui).x : 1440;
    float t = g.started >= 0 ? (float)(cui_time(ui) - g.started) : 99;
    g.narrow = vw < 760;
    if (!g.narrow) g.drawer = false;
    cui_xml_set_var(doc, "narrow", g.narrow ? "1" : "0");
    cui_xml_set_var(doc, "side", vw >= 1180 ? "1" : "0");
    cui_xml_set_var(doc, "asked", g.asked_once ? "1" : "0");
    cui_xml_set_var(doc, "askedText", g.asked);
    cui_xml_set_var(doc, "busy", g.asked_once && t < 7.5f ? "1" : "0");
}

/* ---- wiring ------------------------------------------------------------- */

static cui_xml *open_document(const char *path) {
    cui_xml *doc = cui_xml_load(path);
    if (!doc) return NULL;
    cui_xml_bind_int(doc, "chat", &g.chat);
    cui_xml_set_var(doc, "narrow", "0");
    cui_xml_set_var(doc, "side", "1");
    cui_xml_set_var(doc, "asked", "0");
    cui_xml_set_var(doc, "askedText", "");
    cui_xml_set_var(doc, "busy", "0");
    cui_xml_on_any(doc, on_action, NULL);
    cui_xml_register_tag(doc, "Answer", tag_answer, NULL);
    cui_xml_register_tag(doc, "ChatMessage", tag_chat_message, NULL);
    cui_xml_register_tag(doc, "ModelPicker", tag_model_picker, NULL);
    cui_xml_register_tag(doc, "ContextMeter", tag_context_meter, NULL);
    cui_xml_register_tag(doc, "Plan", tag_plan, NULL);
    cui_xml_register_tag(doc, "PromptSuggestions", tag_prompt_suggestions, NULL);
    cui_xml_register_tag(doc, "ChatComposer", tag_chat_composer, NULL);
    cui_xml_register_tag(doc, "UserMenu", tag_user_menu, NULL);
    cui_xml_register_tag(doc, "Drawer", tag_drawer, NULL);
    return doc;
}

/* One frame: the values, then the window. The assistant has no clock of its
 * own beyond the answer that is playing, so the shot and the app run this and
 * nothing else. */
static void frame(cui_ctx *ui, cui_xml *doc) {
    values(doc, ui);
    cui_xml_render(doc, ui);
}

/* `assistantxml --shot out.png`: the same window drawn offscreen with the
 * software renderer, with the viewport, the theme, the accent and the six
 * frames of examples/assistant.c, so the two pictures can be compared. */
/* The frame the app draws, as cui_app_run calls it. */
static void assistant_xml_frame(cui_ctx *ui, void *user) {
    cui_xml *doc = user;
    frame(ui, doc);
    cui_xml_poll(doc);
    if (cui_xml_quit_requested(doc)) cui_app_quit(NULL);
}

static int write_shot(cui_xml *doc, const char *out_path) {
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui;
    const cui_draw_data *draw;
    unsigned char *pixels;
    int width = 0, height = 0, i;
    if (!sw) return 1;
    {
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
    const char *path = CUI_ASSISTANT_XML_PATH;
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
        int rc = cui_app_run(&desc, assistant_xml_frame, doc);
        cui_xml_free(doc);
        return rc;
    }
}
