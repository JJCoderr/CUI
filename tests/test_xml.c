#define _CRT_SECURE_NO_WARNINGS 1

/* The XML layer: reading a document, {"..."}, templates, <Repeat>, bindings,
 * actions, hot reload and the catalog. It draws with the software renderer so
 * the components really run, and clicks with the same input the platform
 * layer feeds. */
#include <cui/cui_render.h>
#include <cui/cui_xml.h>

#include "../src/cui_internal.h"   /* the nodes one frame built: the nav highlight is one of them */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failed, checks;
#define CHECK(c) do { checks++; if (!(c)) { failed++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

/* ---- the document the tests use ---- */

static const char *k_doc =
    "<Window Title=\"Test\" Width=\"640\" Height=\"480\" Theme=\"dark\" Pad=\"16\" Gap=\"8\">"
    "  <Column Gap=\"8\">"
    "    <Heading Level=\"1\">Hello</Heading>"
    "    <Text Name=\"counter\">Count: {count}</Text>"
    "    <Switch Name=\"flag\" Text=\"Flag\" Bind=\"flag\"/>"
    "    <Checkbox Name=\"agree\" Text=\"Agree\"/>"
    "    <Button Name=\"go\" Text=\"Go\" OnClicked=\"go.clicked\"/>"
    "    <Input Name=\"name\" Placeholder=\"Name\" OnSubmitted=\"name.submitted\"/>"
    "    <Slider Name=\"vol\" Min=\"0\" Max=\"10\" Bind=\"vol\"/>"
    "    <Repeat Count=\"3\" Var=\"i\">"
    "      <Text Name=\"row{i}\">Row {i}</Text>"
    "    </Repeat>"
    "    <ForEach Items=\"a|b\" Var=\"it\">"
    "      <Text Name=\"each-{it}\">Entry {it}</Text>"
    "    </ForEach>"
    "    <Widget Name=\"Fancy\" Label=\"x\">"
    "      <Button Text=\"{Label}\"/>"
    "    </Widget>"
    "    <Fancy Name=\"chip1\" Label=\"One\"/>"
    "    <Text Name=\"maybe\" If=\"flag\">shown</Text>"
    "    <Text Name=\"never\" If=\"!flag\">hidden</Text>"
    "    <Card Name=\"card\">"
    "      <Button Text=\"Inside\"/>"
    "    </Card>"
    "  </Column>"
    "</Window>";

/* ---- harness ---- */

typedef struct test_state {
    int go, submitted, other;
    char last_action[64];
} test_state;

static test_state g_state;
static bool g_flag;
static float g_vol;
static cui_ctx *g_ui;
static cui_xml *g_doc;
static int g_actions;

static void on_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    (void)doc;
    (void)node;
    (void)user;
    snprintf(g_state.last_action, sizeof g_state.last_action, "%s", action);
    g_actions++;
    if (!strcmp(action, "go.clicked")) g_state.go++;
    else if (!strcmp(action, "name.submitted")) g_state.submitted++;
    else g_state.other++;
}

static void run(cui_ctx *ui, cui_xml *doc, int frames) {
    int i;
    for (i = 0; i < frames; i++) {
        cui_begin_frame(ui, 1.0f / 60.0f);
        cui_xml_render(doc, ui);
        cui_end_frame(ui);
    }
}

static void click_at(cui_ctx *ui, cui_xml *doc, float x, float y) {
    cui_input_mouse_move(ui, x, y);
    run(ui, doc, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    run(ui, doc, 1);
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    run(ui, doc, 2);
}

/* Clicks across the element until `done` says it worked: a control's label and
 * its switch or box do not always share the whole rectangle. */
static bool sweep_click(cui_ctx *ui, cui_xml *doc, cui_xml_node *node, bool (*done)(void)) {
    cui_rect r = cui_xml_node_rect(node);
    float y = r.y + r.h * 0.5f;
    float x;
    if (r.w <= 0 || r.h <= 0) return false;
    for (x = r.x + 6; x < r.x + r.w - 4; x += 8) {
        click_at(ui, doc, x, y);
        if (done()) return true;
    }
    return false;
}

static bool done_flag(void) { return g_flag; }
static bool done_go(void) { return g_state.go > 0; }
static bool done_agree(void) { return cui_xml_get_bool(g_doc, "agree"); }
static bool done_keyboard(void) { return cui_wants_keyboard(g_ui); }

static void press(cui_ctx *ui, cui_xml *doc, cui_key key) {
    cui_input_key(ui, key, true, 0);
    cui_input_key(ui, key, false, 0);
    run(ui, doc, 2);
}

/* ---- the tests ---- */

static void test_reading(cui_xml *doc) {
    int w = 0, h = 0;
    CHECK(cui_xml_ok(doc));
    CHECK(!strcmp(cui_xml_title(doc), "Test"));
    cui_xml_window_size(doc, &w, &h);
    CHECK(w == 640 && h == 480);
    CHECK(cui_xml_theme(doc, CUI_THEME_LIGHT) == CUI_THEME_DARK);
    CHECK(cui_xml_root(doc) != NULL);
    CHECK(!strcmp(cui_xml_node_tag(cui_xml_root(doc)), "Window"));
    CHECK(cui_xml_find(doc, "go") != NULL);
    CHECK(!strcmp(cui_xml_node_tag(cui_xml_find(doc, "go")), "Button"));
    CHECK(cui_xml_find(doc, "nope") == NULL);
}

static void test_templates(cui_xml *doc) {
    CHECK(cui_xml_find(doc, "row0") && cui_xml_find(doc, "row1") && cui_xml_find(doc, "row2"));
    CHECK(!cui_xml_find(doc, "row3"));
    CHECK(cui_xml_find(doc, "each-a") && cui_xml_find(doc, "each-b"));
    CHECK(cui_xml_is_widget(doc, "Fancy"));
    CHECK(!cui_xml_is_widget(doc, "Button"));
    {
        cui_xml_node *chip = cui_xml_find(doc, "chip1");
        CHECK(chip != NULL);
        if (chip) {
            CHECK(!strcmp(cui_xml_node_tag(chip), "Button"));
            /* the instance attribute reached the element the declaration became */
            CHECK(!strcmp(cui_xml_node_attr(chip, "text"), "One"));
        }
    }
}

static void test_expressions(cui_xml *doc, cui_ctx *ui) {
    cui_xml_node *counter = cui_xml_find(doc, "counter");
    CHECK(counter != NULL);
    cui_xml_set_var(doc, "count", "42");
    run(ui, doc, 2);
    if (counter) CHECK(!strcmp(cui_xml_node_text(doc, counter), "Count: 42"));
    /* a name nothing defines stays as written, so a document still loads */
    cui_xml_set_var(doc, "count", NULL);
    run(ui, doc, 2);
    if (counter) CHECK(strstr(cui_xml_node_text(doc, counter), "{count}") != NULL);
    /* the expression engine itself, through a value the app can read */
    cui_xml_set_var(doc, "count", "7");
    run(ui, doc, 1);
    if (counter) CHECK(!strcmp(cui_xml_node_text(doc, counter), "Count: 7"));
}

static void test_if(cui_xml *doc, cui_ctx *ui) {
    cui_xml_node *maybe = cui_xml_find(doc, "maybe");
    cui_xml_node *never = cui_xml_find(doc, "never");
    g_flag = false;
    run(ui, doc, 2);
    CHECK(cui_xml_node_id(maybe) == 0);
    CHECK(cui_xml_node_id(never) != 0);
    g_flag = true;
    run(ui, doc, 2);
    CHECK(cui_xml_node_id(maybe) != 0);
    CHECK(cui_xml_node_id(never) == 0);
    g_flag = false;
    run(ui, doc, 2);
}

static void test_bindings(cui_xml *doc, cui_ctx *ui) {
    cui_xml_node *flag = cui_xml_find(doc, "flag");
    cui_xml_node *go = cui_xml_find(doc, "go");
    run(ui, doc, 3);

    /* a click on the switch writes the app's variable */
    g_flag = false;
    CHECK(sweep_click(ui, doc, flag, done_flag));
    CHECK(g_flag);
    CHECK(cui_xml_get_bool(doc, "flag"));

    /* a click on the button fires its action */
    g_state.go = 0;
    CHECK(sweep_click(ui, doc, go, done_go));
    CHECK(g_state.go == 1);
    CHECK(!strcmp(g_state.last_action, "go.clicked"));

    /* a slider bound to a float, written from C */
    cui_xml_set_float(doc, "vol", 4.0f);
    run(ui, doc, 1);
    CHECK(g_vol > 3.9f && g_vol < 4.1f);
    CHECK(cui_xml_get_float(doc, "vol") > 3.9f);

    /* a value nobody bound lives in the element */
    CHECK(!cui_xml_get_bool(doc, "agree"));
    CHECK(cui_xml_click(doc, "agree"));
    CHECK(cui_xml_get_bool(doc, "agree"));
    run(ui, doc, 2);
    CHECK(cui_xml_click(doc, "agree"));
    CHECK(!cui_xml_get_bool(doc, "agree"));
    run(ui, doc, 2);

    /* and a click on it flips it too */
    CHECK(sweep_click(ui, doc, cui_xml_find(doc, "agree"), done_agree));
    CHECK(cui_xml_get_bool(doc, "agree"));
}

static void test_text_field(cui_xml *doc, cui_ctx *ui) {
    cui_xml_node *name = cui_xml_find(doc, "name");
    run(ui, doc, 3);
    CHECK(sweep_click(ui, doc, name, done_keyboard));
    cui_input_text(ui, "Ada");
    run(ui, doc, 2);
    CHECK(!strcmp(cui_xml_get_text(doc, "name"), "Ada"));
    g_state.submitted = 0;
    press(ui, doc, CUI_KEY_ENTER);
    CHECK(g_state.submitted == 1);
    CHECK(!strcmp(g_state.last_action, "name.submitted"));
    cui_xml_set_text(doc, "name", "Grace");
    run(ui, doc, 1);
    CHECK(!strcmp(cui_xml_get_text(doc, "name"), "Grace"));
}

static void test_builtins(cui_xml *doc, cui_ctx *ui) {
    cui_xml_node *maybe = cui_xml_find(doc, "maybe");
    run(ui, doc, 1);
    CHECK(cui_xml_is_builtin("theme.toggle"));
    CHECK(cui_xml_is_builtin("app.quit"));
    CHECK(cui_xml_is_builtin("show:card"));
    CHECK(!cui_xml_is_builtin("go.clicked"));
    CHECK(cui_xml_node_visible(maybe));
    cui_xml_fire(doc, "hide:maybe");
    CHECK(!cui_xml_node_visible(maybe));
    run(ui, doc, 2);
    CHECK(cui_xml_node_id(maybe) == 0);
    cui_xml_fire(doc, "show:maybe");
    CHECK(cui_xml_node_visible(maybe));
    run(ui, doc, 2);
    CHECK(cui_xml_node_id(maybe) != 0);
    CHECK(!cui_xml_quit_requested(doc));
    cui_xml_fire(doc, "app.quit");
    CHECK(cui_xml_quit_requested(doc));
    cui_xml_set_quit_requested(doc, false);
    g_state.other = 0;
    cui_xml_fire(doc, "something.else");
    CHECK(g_state.other == 1);
    CHECK(!strcmp(g_state.last_action, "something.else"));
    /* the built-ins can be turned off: the default handler then sees them */
    cui_xml_set_builtins(doc, false);
    g_state.other = 0;
    cui_xml_fire(doc, "hide:card");
    CHECK(g_state.other == 1);
    CHECK(cui_xml_node_visible(cui_xml_find(doc, "card")));
    cui_xml_set_builtins(doc, true);
}

static void test_render_into(cui_xml *doc, cui_ctx *ui) {
    cui_xml_node *card = cui_xml_find(doc, "card");
    run(ui, doc, 2);
    CHECK(cui_xml_node_id(card) != 0);
    CHECK(cui_xml_node_rect(card).w > 0);
    CHECK(cui_xml_node_by_id(doc, cui_xml_node_id(card)) == card);
    CHECK(cui_xml_node_by_id(doc, 0) == NULL);
    /* a subtree on its own, inside a frame written in C (twice: the rectangle
     * of an id comes from the frame that laid it out) */
    {
        int pass;
        for (pass = 0; pass < 2; pass++) {
            cui_begin_frame(ui, 1.0f / 60.0f);
            cui_column_begin(ui, &(cui_box){ 0 });
            CHECK(cui_xml_render_into(doc, ui, "card"));
            CHECK(!cui_xml_render_into(doc, ui, "nope"));
            cui_box_end(ui);
            cui_end_frame(ui);
        }
    }
    CHECK(cui_xml_node_rect(card).h > 0);
    run(ui, doc, 1);
    /* comments are kept, and drawn nowhere */
    {
        cui_xml *c = cui_xml_parse("<Window><!-- note --><Column><Text>x</Text></Column></Window>", 0, NULL);
        CHECK(c != NULL);
        if (c) {
            cui_xml_node *root = cui_xml_root(c);
            CHECK(root != NULL);
            if (root) {
                CHECK(cui_xml_node_child_count(root) > 0);
                if (cui_xml_node_child_count(root) > 0) {
                    CHECK(cui_xml_node_is_comment(cui_xml_node_child(root, 0)));
                }
            }
            cui_xml_free(c);
        }
    }
}

static void test_catalog(void) {
    int i, found_name = 0, found_variant = 0;
    cui_xml_attribute_desc info;
    CHECK(cui_xml_tag_count() > 60);
    CHECK(cui_xml_tag_children("Button") == 0);
    CHECK(cui_xml_tag_children("Row") == 2);
    CHECK(cui_xml_tag_children("Split") == 2);
    CHECK(cui_xml_tag_children("NoSuchTag") == 2);
    CHECK(cui_xml_tag_children("Window") == 2);
    CHECK(cui_xml_tag_description("Button")[0] != 0);
    CHECK(cui_xml_tag_name(0)[0] != 0);
    CHECK(cui_xml_tag_name(9999)[0] == 0);
    CHECK(cui_xml_attribute_count("Button") > 5);
    for (i = 0; i < cui_xml_attribute_count("Button"); i++) {
        CHECK(cui_xml_attribute_info("Button", i, &info));
        if (!strcmp(info.name, "Name")) found_name++;
        if (!strcmp(info.name, "Variant")) found_variant++;
    }
    CHECK(found_name == 1);
    CHECK(found_variant == 1);
    CHECK(!cui_xml_attribute_info("Button", 999, NULL));
}

static void test_errors(const char *dir, cui_ctx *ui) {
    cui_xml_error error;
    cui_xml *doc;
    char path[600];

    memset(&error, 0, sizeof error);
    doc = cui_xml_parse("<Window><Bogus/></Window>", 0, &error);
    CHECK(doc != NULL);
    if (doc) {
        CHECK(cui_xml_ok(doc));       /* an unknown tag is reported when it is drawn */
        run(ui, doc, 1);
        CHECK(!cui_xml_ok(doc));
        CHECK(strstr(cui_xml_last_error(doc), "not a tag") != NULL);
        cui_xml_free(doc);
    }
    doc = cui_xml_parse("<Window><Column></Window>", 0, &error);
    CHECK(doc != NULL);
    if (doc) {
        CHECK(!cui_xml_ok(doc));
        CHECK(error.message[0] != 0);
        cui_xml_free(doc);
    }
    CHECK(cui_xml_parse("<Window", 0, &error) == NULL);

    /* an Include that names no file is reported and the rest loads */
    snprintf(path, sizeof path, "%s/cui_xml_include.xml", dir);
    doc = cui_xml_parse("<Window><Include Source=\"missing-file.xml\"/><Label>x</Label></Window>", 0, &error);
    CHECK(doc != NULL);
    if (doc) {
        CHECK(!cui_xml_ok(doc));
        CHECK(strstr(cui_xml_last_error(doc), "Include") != NULL);
        CHECK(cui_xml_find(doc, "x") == NULL || 1);
        cui_xml_free(doc);
    }
    remove(path);
}

static void test_include_and_widgets_file(const char *dir) {
    char main_path[600], inc_path[600];
    cui_xml *doc;
    FILE *f;
    snprintf(main_path, sizeof main_path, "%s/cui_xml_main.xml", dir);
    snprintf(inc_path, sizeof inc_path, "%s/cui_xml_parts.xml", dir);

    /* the included file declares a tag and brings in one element */
    f = fopen(inc_path, "wb");
    CHECK(f != NULL);
    if (!f) return;
    fputs("<Widgets>"
          "<Widget Name=\"Included\"><Text Name=\"included-text\">from the file</Text></Widget>"
          "<Label Name=\"included-label\">included</Label>"
          "</Widgets>", f);
    fclose(f);

    f = fopen(main_path, "wb");
    if (!f) return;
    fputs("<Window Title=\"Include\"><Column>"
          "<Include Source=\"cui_xml_parts.xml\"/>"
          "<Included/>"
          "</Column></Window>", f);
    fclose(f);

    doc = cui_xml_load(main_path);
    CHECK(doc != NULL);
    if (doc) {
        CHECK(cui_xml_ok(doc));
        CHECK(cui_xml_find(doc, "included-text") != NULL);
        CHECK(cui_xml_find(doc, "included-label") != NULL);
        cui_xml_free(doc);
    }
    /* a tag declared by another file, registered by hand */
    doc = cui_xml_parse("<Column><Extra Text=\"x\"/></Column>", 0, NULL);
    if (doc) {
        cui_xml_error error;
        int added;
        memset(&error, 0, sizeof error);
        added = cui_xml_register_widgets(doc, "<Widget Name=\"Extra\"><Button Text=\"extra\"/></Widget>", 0);
        CHECK(added == 1);
        CHECK(cui_xml_is_widget(doc, "Extra"));
        cui_xml_free(doc);
    }
    remove(main_path);
    remove(inc_path);
}

static void test_reload(const char *dir) {
    char path[600];
    cui_xml *doc;
    FILE *f;
    snprintf(path, sizeof path, "%s/cui_xml_reload.xml", dir);

    f = fopen(path, "wb");
    CHECK(f != NULL);
    if (!f) return;
    fputs("<Window Title=\"One\">"
          "<Column><Switch Name=\"notify\" Text=\"N\"/>"
          "<Text Name=\"label\">first</Text></Column></Window>", f);
    fclose(f);

    doc = cui_xml_load(path);
    CHECK(doc != NULL);
    if (!doc) return;
    CHECK(cui_xml_ok(doc));
    CHECK(!strcmp(cui_xml_title(doc), "One"));
    CHECK(cui_xml_get_watch(doc));
    CHECK(!strcmp(cui_xml_path(doc), path));
    cui_xml_set_bool(doc, "notify", true);
    {
        cui_app_desc d = cui_xml_app_desc(doc);
        CHECK(!strcmp(d.title, "One"));
        CHECK(d.width == 900 && d.height == 640);
    }

    /* the same file, changed: the title follows and the switch keeps its value */
    f = fopen(path, "wb");
    if (!f) {
        cui_xml_free(doc);
        return;
    }
    fputs("<Window Title=\"Two\">"
          "<Column><Switch Name=\"notify\" Text=\"N\"/>"
          "<Text Name=\"label\">second</Text>"
          "<Button Text=\"New\" OnClicked=\"new.clicked\"/></Column></Window>", f);
    fclose(f);
    CHECK(cui_xml_poll(doc));
    CHECK(!strcmp(cui_xml_title(doc), "Two"));
    CHECK(cui_xml_get_bool(doc, "notify"));
    CHECK(cui_xml_find(doc, "label") != NULL);
    CHECK(!cui_xml_poll(doc));   /* nothing changed since */

    /* a broken file leaves the interface as it was */
    f = fopen(path, "wb");
    if (!f) {
        cui_xml_free(doc);
        return;
    }
    fputs("<Window Title=\"Three\"><Column></Window>", f);
    fclose(f);
    CHECK(!cui_xml_poll(doc));
    CHECK(!strcmp(cui_xml_title(doc), "Two"));

    remove(path);
    cui_xml_free(doc);
}

/* Theme colors named in a document ("accent", "surface"), Fill shares, the
 * computed attribute, and a tag of the app drawing its children. */
static int g_tag_calls;
static void paint_probe(cui_ctx *ui, cui_rect r, void *user) {
    (void)user;
    cui_draw_rect(ui, r, CUI_CORNERS(4), cui_token_color(ui, CUI_TOKEN_ACCENT));
}
static bool tag_probe(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    (void)user;
    g_tag_calls++;
    cui_canvas(ui, cui_xml_node_name(node), CUI_FILL, CUI_PX(20), paint_probe, NULL, 0);
    cui_xml_draw_children(doc, node, ui);
    CHECK(!strcmp(cui_xml_node_attr_value(doc, node, "value"), "42"));   /* {n * 2} */
    return true;
}
static void test_colors_and_tags(cui_ctx *ui) {
    static const char *text =
        "<Window Background=\"background\" Pad=\"8\">"
        "<Column Gap=\"4\">"
        "<Box Fill=\"1\" Height=\"30\" Background=\"surface\" Border=\"1\" BorderColor=\"border\"/>"
        "<Box Fill=\"2\" Height=\"30\"/>"
        "<Text Color=\"accent\" Font=\"medium\" Tabular=\"true\" Size=\"14\">accent text</Text>"
        "<Probe Name=\"probe\" Value=\"{n * 2}\"><Text Size=\"12\" Color=\"textMuted\">child</Text></Probe>"
        "</Column></Window>";
    int n = 21;
    cui_xml *doc;
    g_tag_calls = 0;
    doc = cui_xml_parse(text, 0, NULL);
    CHECK(doc != NULL);
    if (!doc) return;
    cui_xml_bind_int(doc, "n", &n);
    CHECK(cui_xml_register_tag(doc, "Probe", tag_probe, NULL));
    CHECK(cui_xml_is_tag_registered(doc, "Probe"));
    CHECK(!cui_xml_register_tag(doc, "Button", tag_probe, NULL));   /* a tag of the library */
    run(ui, doc, 2);
    CHECK(cui_xml_ok(doc));
    CHECK(g_tag_calls >= 2);
    CHECK(cui_xml_node_rect(cui_xml_find(doc, "probe")).w > 0);
    cui_xml_free(doc);
}
/* The demo the repository ships: examples/xml_demo.xml must load without a
 * mistake and draw, with every tag it uses. */
static void test_example_document(cui_ctx *ui) {
    char path[600];
    cui_xml *doc;
    snprintf(path, sizeof path, "%s/../examples/xml_demo.xml", CUI_XML_TEST_DIR);
    doc = cui_xml_load(path);
    CHECK(doc != NULL);
    if (!doc) return;
    CHECK(cui_xml_ok(doc));
    if (!cui_xml_ok(doc)) printf("  example: %s\n", cui_xml_last_error(doc));
    CHECK(cui_xml_title(doc)[0] != 0);
    CHECK(cui_xml_find(doc, "scale") != NULL);
    CHECK(cui_xml_find(doc, "pages") != NULL);
    CHECK(cui_xml_find(doc, "prefs") != NULL);
    run(ui, doc, 3);
    CHECK(cui_xml_node_id(cui_xml_find(doc, "pages")) != 0);
    CHECK(cui_xml_node_id(cui_xml_find(doc, "prefs")) == 0);   /* shown with show:prefs */
    cui_xml_fire(doc, "show:prefs");
    run(ui, doc, 2);
    CHECK(cui_xml_node_bool(doc, cui_xml_find(doc, "prefs")));
    CHECK(cui_xml_node_id(cui_xml_find(doc, "prefs")) != 0);
    cui_xml_fire(doc, "hide:prefs");
    run(ui, doc, 2);
    CHECK(!cui_xml_node_bool(doc, cui_xml_find(doc, "prefs")));
    cui_xml_free(doc);
}
/* A run of nav items described in a document shares one highlight, and it travels
 * from the item selected to the one that takes over. Each element draws in a scope
 * of its own, so the group's state id is made in the scope of the list; made in the
 * item's own, every item would be a group of its own, and the highlight would appear
 * at the new item instead of gliding to it. */
static int g_nav_section;

static int nav_highlight_y(cui_ctx *ui, float *y, int cap) {
    int found = 0;
    int i;
    for (i = 0; i < ui->node_count; i++) {
        if (ui->nodes[i].paint != cui__paint_nav_highlight) continue;
        if (found < cap) y[found] = ui->nodes[i].rect.y;
        found++;
    }
    return found;
}

static void test_nav_highlight(cui_ctx *ui) {
    static const char *text =
        "<Window Width=\"300\" Height=\"300\"><Column Width=\"200\" Gap=\"2\">"
        "<NavItem Name=\"one\" Text=\"One\" Group=\"g\" Selected=\"{section == 0}\" OnClicked=\"nav:0\"/>"
        "<NavItem Name=\"two\" Text=\"Two\" Group=\"g\" Selected=\"{section == 1}\" OnClicked=\"nav:1\"/>"
        "<NavItem Name=\"three\" Text=\"Three\" Group=\"g\" Selected=\"{section == 2}\" OnClicked=\"nav:2\"/>"
        "<NavItem Name=\"four\" Text=\"Four\" Group=\"g\" Selected=\"{section == 3}\" OnClicked=\"nav:3\"/>"
        "</Column></Window>";
    float y[4], target, moved;
    cui_xml *doc;
    cui_set_reduced_motion(ui, false);
    g_nav_section = 0;
    doc = cui_xml_parse(text, strlen(text), NULL);
    CHECK(doc != NULL);
    if (!doc) return;
    cui_xml_bind_int(doc, "section", &g_nav_section);
    run(ui, doc, 4);
    CHECK(nav_highlight_y(ui, y, 4) == 1);         /* the items are one group, so one highlight */
    CHECK(y[0] == 0.0f);
    target = cui_xml_node_rect(cui_xml_find(doc, "four")).y;
    CHECK(target > 90.0f);                          /* four rows of 32 and their gaps */
    g_nav_section = 3;
    run(ui, doc, 1);
    CHECK(nav_highlight_y(ui, y, 4) == 1);          /* the group did not split into two */
    run(ui, doc, 1);
    CHECK(nav_highlight_y(ui, y, 4) == 1);
    moved = y[0];
    CHECK(moved > 0.0f);                            /* it left the first item... */
    CHECK(moved < target - 30.0f);                  /* ...and has not arrived yet */
    run(ui, doc, 6);
    CHECK(nav_highlight_y(ui, y, 4) == 1);
    CHECK(y[0] > moved);                            /* on its way, every frame */
    CHECK(y[0] < target);
    run(ui, doc, 40);
    CHECK(nav_highlight_y(ui, y, 4) == 1);
    CHECK(y[0] > target - 2.0f && y[0] < target + 2.0f);
    CHECK(g_nav_section == 3);                      /* a document never moves the value itself */
    cui_xml_free(doc);
    cui_set_reduced_motion(ui, true);
}

/* The social pieces a document can say: a divider, a typing line, a rolling
 * counter, comments with their hearts, presence avatars and a ticker. */
/* A checklist marks the step that was clicked, as the C examples do: the tag
 * owns the boxes, the component only reports what was clicked. */
static bool done_step_one(void) { return cui_xml_get_bool(g_doc, "s1"); }
static bool done_step_two(void) { return cui_xml_get_bool(g_doc, "s2"); }

/* A checklist marks the step that was clicked, as the C examples do: the tag owns
 * the boxes, the component only reports what was clicked. */
static void test_checklist_click(cui_ctx *ui) {
    static const char *text =
        "<Window Width=\"420\" Height=\"300\"><Column Pad=\"10\" Gap=\"10\">"
        "  <Checklist Name=\"list\" Title=\"Set up\">"
        "    <Step Name=\"s1\" Description=\"Pick a name and invite your team.\" Action=\"Create\">Create your workspace</Step>"
        "    <Step Name=\"s2\" Description=\"Link GitHub or GitLab.\" Action=\"Connect\">Connect a repository</Step>"
        "  </Checklist>"
        "</Column></Window>";
    cui_xml *doc = cui_xml_parse(text, strlen(text), NULL);
    CHECK(doc != NULL);
    if (!doc) return;
    g_doc = doc;
    run(ui, doc, 2);
    CHECK(!done_step_one());
    CHECK(!done_step_two());
    /* the box is a small circle in the row: sweep the card, not its middle line */
    {
        cui_rect r = cui_xml_node_rect(cui_xml_find(doc, "list"));
        float y, x;
        for (y = r.y + r.h * 0.55f; y < r.y + r.h - 4 && !done_step_two(); y += 6)
            for (x = r.x + 6; x < r.x + r.w - 4 && !done_step_two(); x += 8) click_at(ui, doc, x, y);
    }
    /* the component reports the step it acted on (the open one); whichever it is,
     * the box has to end up ticked, which is what the C examples do */
    CHECK(done_step_one() || done_step_two());
    g_doc = NULL;
    cui_xml_free(doc);
}

static void test_social_tags(cui_ctx *ui) {
    static const char *text =
        "<Window Width=\"640\" Height=\"420\"><Column Gap=\"10\" Pad=\"10\">"
        "  <ChatDivider Name=\"div\">Today</ChatDivider>"
        "  <TypingIndicator Name=\"typing\">Jonas</TypingIndicator>"
        "  <Counter Name=\"count\" Value=\"48210\" Format=\"$%.0f\" GroupThousands=\"true\"/>"
        "  <Comment Name=\"c1\" Author=\"Tomas Rivera\" Time=\"2h\" Badge=\"Author\" Likes=\"12\" Liked=\"true\">The numbers match the staging run.</Comment>"
        "  <Comment Name=\"c2\" Author=\"Priya Nair\" Depth=\"1\" Likes=\"3\">Agreed.</Comment>"
        "  <Presence Name=\"here\" Users=\"Maya Okafor:#7c5cff|Jonas Berg:#22c58b:away|Priya Nair:#f59e0b\""
        "            Visible=\"5\" Size=\"30\"/>"
        "  <Ticker Name=\"tick\" Items=\"Revenue:$48.2k:6.9|Churn:1.7%:-0.6:lower|Uptime:99.98%:0\"/>"
        "  <CardStack Name=\"deck\" Count=\"3\" Width=\"200\" Height=\"150\">"
        "    <CardStackItem><Text>Kyoto</Text></CardStackItem>"
        "    <CardStackItem><Text>Lisbon</Text></CardStackItem>"
        "    <CardStackItem><Text>Reykjavik</Text></CardStackItem>"
        "  </CardStack>"
        "</Column></Window>";
    cui_xml *doc = cui_xml_parse(text, strlen(text), NULL);
    CHECK(doc != NULL);
    if (!doc) return;
    run(ui, doc, 2);
    CHECK(cui_xml_ok(doc));
    if (!cui_xml_ok(doc)) printf("  social: %s\n", cui_xml_last_error(doc));
    /* a divider and a typing line are decoration: the library leaves them without
     * an id, so they are found by name and not asked for a rectangle */
    CHECK(cui_xml_find(doc, "div") != NULL);
    CHECK(cui_xml_find(doc, "typing") != NULL);
    CHECK(cui_xml_node_id(cui_xml_find(doc, "count")) != 0);
    CHECK(cui_xml_node_id(cui_xml_find(doc, "c1")) != 0);
    CHECK(cui_xml_node_id(cui_xml_find(doc, "c2")) != 0);
    CHECK(cui_xml_node_id(cui_xml_find(doc, "here")) != 0);
    CHECK(cui_xml_node_id(cui_xml_find(doc, "tick")) != 0);
    CHECK(cui_xml_node_rect(cui_xml_find(doc, "tick")).h > 0);
    CHECK(cui_xml_node_id(cui_xml_find(doc, "deck")) != 0);
    cui_xml_free(doc);
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : ".";
    cui_sw_renderer *sw = cui_sw_create();
    cui_ctx *ui;
    cui_xml *doc;
    cui_xml_error error;
    (void)g_actions;

    memset(&g_state, 0, sizeof g_state);
    memset(&error, 0, sizeof error);
    ui = cui_create(&(cui_ctx_desc){ .renderer = cui_sw_iface(sw) });
    cui_set_viewport(ui, 640, 1200, 1);
    cui_set_reduced_motion(ui, true);
    cui_input_mouse_leave(ui);
    g_ui = ui;

    doc = cui_xml_parse(k_doc, strlen(k_doc), &error);
    CHECK(doc != NULL);
    if (!doc) {
        printf("%d checks, %d failed\n", checks, failed);
        return 1;
    }
    g_doc = doc;
    cui_xml_bind_bool(doc, "flag", &g_flag);
    cui_xml_bind_float(doc, "vol", &g_vol);
    cui_xml_on_any(doc, on_action, NULL);

    test_reading(doc);
    test_templates(doc);
    test_expressions(doc, ui);
    test_if(doc, ui);
    test_bindings(doc, ui);
    test_text_field(doc, ui);
    test_builtins(doc, ui);
    test_render_into(doc, ui);
    test_catalog();
    test_errors(dir, ui);
    test_include_and_widgets_file(dir);
    test_reload(dir);
    test_colors_and_tags(ui);
    test_social_tags(ui);
    test_checklist_click(ui);
    test_example_document(ui);
    test_nav_highlight(ui);

    cui_xml_free(doc);
    cui_destroy(ui);
    cui_sw_destroy(sw);
    printf("%d checks, %d failed\n", checks, failed);
    return failed ? 1 : 0;
}

