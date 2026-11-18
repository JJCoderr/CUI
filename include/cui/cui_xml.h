/*
 * CUI XML: an interface described in a file, loaded as a tree, drawn every
 * frame -- the declarative layer Raven has, adapted to CUI's immediate mode.
 *
 *     cui_xml *doc = cui_xml_load("settings.xml");
 *     cui_app *app = cui_app_create(&cui_xml_app_desc(doc));
 *     while (cui_app_frame(app)) {
 *         cui_ctx *ui = cui_app_ui(app);
 *         cui_xml_poll(doc);            (reloads the file when it changed)
 *         cui_xml_render(doc, ui);      (draws the whole document)
 *     }
 *
 *     <!-- settings.xml -->
 *     <Window Title="Settings" Width="880" Height="620" Pad="24" Gap="12">
 *       <Heading Level="1">Settings</Heading>
 *       <Card>
 *         <Row Gap="8">
 *           <Input Name="path" Label="Folder" Placeholder="~/Projects" Fill="1"/>
 *           <Button Text="Save" Variant="Primary" OnClicked="file.save"/>
 *         </Row>
 *       </Card>
 *       <Switch Name="notify" Text="Notifications" Bind="notify"/>
 *       <Slider Text="Scale" Min="0.5" Max="3" Step="0.1" Bind="scale"/>
 *     </Window>
 *
 * Rules (the same ones Raven documents, in CUI terms)
 *   - An element is a component; its tag is the component type (Button,
 *     VBox, ...). Tags and attribute names are not case sensitive.
 *   - Child elements are drawn inside their parent, in order. The text
 *     inside an element is its Text: <Label>Hello</Label> is
 *     <Label Text="Hello"/>. Element text wins over the Text attribute.
 *   - {"..."} is computed where an attribute is read, so a label can show a
 *     value: Text="{count} items". "{{" writes one brace.
 *   - <Widget Name="X" Param="default">...</Widget> declares a tag of your
 *     own; <X Param="..."/> uses it. Inside it, {Param} is the value, and
 *     <Children/> is where the children given where the tag is used go.
 *   - <Include Source="file.xml"/>, <Repeat Count="3" Var="i">,
 *     <ForEach Items="a|b|c" Var="item"/> and If="expr" on any element.
 *   - Every tag and attribute is listed in docs/xml.md; cui_xml_tag_* and
 *     cui_xml_attribute_* give the same list to a tool.
 *   - An unknown tag or a bad value is reported with its line and column
 *     through the error callback, and the rest is drawn anyway.
 *   - A document read from a file follows that file: save it in an editor
 *     and the interface changes while the app runs. Values the app keeps
 *     (what was typed, a slider, the open tab) survive the reload.
 *
 * Values and bindings
 *   - What a control holds (a switch, a slider, a text field, the selected
 *     tab) lives in the document, keyed by Name, so it is stable frame to
 *     frame. cui_xml_get_bool(doc, "notify") reads it.
 *   - Register the app's own variables and the control writes them instead:
 *     cui_xml_bind_bool(doc, "notify", &notify). Bind="name" pairs a control
 *     with a binding whose name differs from its Name.
 *   - cui_xml_set_text / set_number / set_index change what a control shows;
 *     "show:NAME", "hide:NAME" and the other built-in actions do the same
 *     from XML, with no C.
 *
 * Actions
 *   - OnClicked / OnChanged / OnSubmitted / OnAction="name" fire an action.
 *     cui_xml_on(doc, "file.save", fn, user) binds it; cui_xml_on_any takes
 *     the names nothing else handles. Names CUI answers by itself (they apply
 *     when no function is bound): app.quit, theme.dark, theme.light,
 *     theme.toggle, accent:NAME, show:NAME, hide:NAME, toggle:NAME,
 *     enable:NAME, disable:NAME, page:NAME=+1, copy:TEXT.
 *
 * MIT License. See LICENSE.
 */
#ifndef CUI_XML_H
#define CUI_XML_H

#include "cui.h"
#include "cui_app.h"   /* cui_app_desc, for cui_xml_app_desc */

#ifdef __cplusplus
extern "C" {
#endif

/* One document: an interface read from XML. Free it with cui_xml_free. */
typedef struct cui_xml cui_xml;
/* One element of a document: a component, a comment or a logical tag. */
typedef struct cui_xml_node cui_xml_node;

/* Where and why a text could not be read. Line and column start at 1. */
typedef struct cui_xml_error {
    int line;
    int column;
    char message[160];
} cui_xml_error;

/* The kind of value an attribute takes (cui_xml_attribute_info). */
typedef enum cui_xml_value_kind {
    CUI_XML_TEXT = 0,   /* free text, {"..."} computed                              */
    CUI_XML_BOOL,       /* true false yes no 1 0                                    */
    CUI_XML_NUMBER,     /* 12  0.5  -3  (always with a dot, whatever the locale)    */
    CUI_XML_CHOICE,     /* one of `choices`                                         */
    CUI_XML_COLOR,      /* #RGB #RGBA #RRGGBB #RRGGBBAA, a color token name, or a
                         * CSS-like name: white black red ... transparent          */
    CUI_XML_ACTION,     /* the name of an action (cui_xml_on)                       */
    CUI_XML_FILE,       /* a path, looked for next to the document                  */
    CUI_XML_NUMBERS,    /* numbers separated by commas: "8,4"                       */
    CUI_XML_LIST        /* texts separated by '|': "One|Two|Three"                  */
} cui_xml_value_kind;

/* One attribute of a tag, for a tool that lists what can be written. The
 * strings belong to the library. */
typedef struct cui_xml_attribute_desc {
    const char *name;
    cui_xml_value_kind kind;
    const char *choices;      /* CUI_XML_CHOICE: the words, separated by '|'; else NULL */
    const char *group;        /* the tag it belongs to, "Common" for every component    */
    const char *description;  /* one line                                                */
} cui_xml_attribute_desc;

/* Reported for each mistake in a document (an unknown tag, a value that does
 * not parse). The rest of the document is loaded and drawn anyway. `path` is
 * the file, or "" for a text given from memory. */
typedef void (*cui_xml_error_fn)(const char *path, const cui_xml_error *error, void *user);

/* An action fired by a control. `node` is the element that fired it (NULL for
 * cui_xml_fire), and `action` its name as written. */
typedef void (*cui_xml_action_fn)(cui_xml *doc, cui_xml_node *node, const char *action, void *user);

/* Called after the file of a document was read again. ok is false when the
 * new text could not be used: the interface is then unchanged. */
typedef void (*cui_xml_reload_fn)(cui_xml *doc, bool ok, void *user);

/* A tag made by the application: draw the element and return true when it
 * did. Registered with cui_xml_register_tag; read its attributes with
 * cui_xml_node_attr, its state with cui_xml_node_bool/int/float/text. */
typedef bool (*cui_xml_tag_fn)(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user);

/* ------------------------------------------------------------------------ */
/* Loading                                                                  */
/* ------------------------------------------------------------------------ */

/* Reads a file. Returns NULL when it cannot be read; the reason goes to the
 * error callback (and to cui_xml_last_error for a document that loaded with
 * warnings). */
CUI_API cui_xml *cui_xml_load(const char *path);
/* Reads a text (UTF-8; it need not be null-terminated). `error`, when not
 * NULL, receives the first mistake and nothing goes to the callback; the
 * document is returned anyway when the rest could be read. */
CUI_API cui_xml *cui_xml_parse(const char *text, size_t size, cui_xml_error *error);
CUI_API void cui_xml_free(cui_xml *doc);

CUI_API bool cui_xml_ok(const cui_xml *doc);              /* no mistake was reported */
CUI_API const char *cui_xml_last_error(const cui_xml *doc);   /* "" when there is none */
CUI_API void cui_xml_set_error_callback(cui_xml *doc, cui_xml_error_fn fn, void *user);

/* The path the document was read from, or "" for a text given from memory. */
CUI_API const char *cui_xml_path(const cui_xml *doc);

/* What <Window> says, for the app: its Title, its client size and its theme
 * (CUI_THEME_LIGHT, CUI_THEME_DARK, or `fallback` when it names none). */
CUI_API const char *cui_xml_title(const cui_xml *doc);
CUI_API void cui_xml_window_size(const cui_xml *doc, int *width, int *height);
CUI_API cui_theme_mode cui_xml_theme(const cui_xml *doc, cui_theme_mode fallback);

/* A cui_app_desc that opens the document's window: title, size and theme.
 * The strings belong to the document, which must outlive the app. Any field
 * the caller wants can be overwritten on the returned struct:
 *
 *     cui_app_desc d = cui_xml_app_desc(doc);
 *     d.continuous = true;
 *     cui_app *app = cui_app_create(&d); */
CUI_API cui_app_desc cui_xml_app_desc(const cui_xml *doc);

/* ------------------------------------------------------------------------ */
/* Drawing                                                                  */
/* ------------------------------------------------------------------------ */

/* Draws the document: call it every frame, between cui_begin_frame and
 * cui_end_frame. A <Window> root fills the viewport (its Pad and Gap apply);
 * any other root is drawn in place, so a document can be one panel of an
 * interface written in C. The theme and accent the document names are applied
 * on the first frame. */
CUI_API void cui_xml_render(cui_xml *doc, cui_ctx *ui);
/* Draws what an element contains: for a tag made with cui_xml_register_tag
 * that behaves like a container (a row with a background of its own). */
CUI_API void cui_xml_draw_children(cui_xml *doc, cui_xml_node *node, cui_ctx *ui);
/* Draws one named element and what it contains, in place: the way to use a
 * piece of a document inside a hand-written frame. Returns false when no
 * element has that name. */
CUI_API bool cui_xml_render_into(cui_xml *doc, cui_ctx *ui, const char *name);

/* ------------------------------------------------------------------------ */
/* Values                                                                   */
/* ------------------------------------------------------------------------ */

/* Control state lives in the document; these bind it to the app's variables,
 * by the name of the control (its Name, or its Bind). Return false when the
 * name is empty or the table is full. */
CUI_API bool cui_xml_bind_bool(cui_xml *doc, const char *name, bool *value);
CUI_API bool cui_xml_bind_int(cui_xml *doc, const char *name, int *value);
CUI_API bool cui_xml_bind_float(cui_xml *doc, const char *name, float *value);
CUI_API bool cui_xml_bind_double(cui_xml *doc, const char *name, double *value);
CUI_API bool cui_xml_bind_text(cui_xml *doc, const char *name, char *buf, size_t cap);
/* Binds a text buffer and a character budget (a text field stops there). */
CUI_API bool cui_xml_unbind(cui_xml *doc, const char *name);

/* Reads what a control holds now (0, false, "" when the name is unknown). */
CUI_API bool cui_xml_get_bool(cui_xml *doc, const char *name);
CUI_API int cui_xml_get_int(cui_xml *doc, const char *name);
CUI_API float cui_xml_get_float(cui_xml *doc, const char *name);
CUI_API double cui_xml_get_double(cui_xml *doc, const char *name);
CUI_API const char *cui_xml_get_text(cui_xml *doc, const char *name);

/* Writes what a control shows (the app sets a value, the XML does the rest). */
CUI_API void cui_xml_set_bool(cui_xml *doc, const char *name, bool value);
CUI_API void cui_xml_set_int(cui_xml *doc, const char *name, int value);
CUI_API void cui_xml_set_float(cui_xml *doc, const char *name, float value);
CUI_API void cui_xml_set_double(cui_xml *doc, const char *name, double value);
CUI_API void cui_xml_set_text(cui_xml *doc, const char *name, const char *text);
/* The selected index of a tab strip, a select, a segmented control... */
CUI_API void cui_xml_set_index(cui_xml *doc, const char *name, int index);

/* True while the control of that name changed during the last frame (a text
 * field changed, a slider moved, a switch flipped). */
CUI_API bool cui_xml_changed(cui_xml *doc, const char *name);

/* Variables {"..."} can read: <Var Name="User" Value="Ana"/> does the same
 * from XML. A value of NULL removes the variable. */
CUI_API void cui_xml_set_var(cui_xml *doc, const char *name, const char *value);
CUI_API const char *cui_xml_get_var(cui_xml *doc, const char *name);

/* ------------------------------------------------------------------------ */
/* Actions                                                                  */
/* ------------------------------------------------------------------------ */

/* Binds an action name; a second binding of the same name replaces the first
 * (the file dialog has one Save). cui_xml_off removes it. */
CUI_API bool cui_xml_on(cui_xml *doc, const char *action, cui_xml_action_fn fn, void *user);
CUI_API void cui_xml_off(cui_xml *doc, const char *action);
/* The handler for the names nothing else handles. */
CUI_API void cui_xml_on_any(cui_xml *doc, cui_xml_action_fn fn, void *user);
/* Fires an action now, as a control would: the bound function, else the
 * built-in one (cui_xml_is_builtin). */
CUI_API void cui_xml_fire(cui_xml *doc, const char *action);
/* Clicks the control of that name: its OnClicked action fires, or its value
 * flips when it holds one (a switch, a chip, a checkbox). Returns false when
 * no control has the name. */
CUI_API bool cui_xml_click(cui_xml *doc, const char *name);

/* The actions CUI answers itself: "app.quit", "theme.dark", "theme.light",
 * "theme.toggle", "accent:NAME", "show:NAME", "hide:NAME", "toggle:NAME",
 * "enable:NAME", "disable:NAME", "page:NAME=N", "copy:TEXT". They apply only
 * when no function is bound to the name. cui_xml_set_builtins(false) turns
 * them off (a preview shows an interface without running it). */
CUI_API bool cui_xml_is_builtin(const char *action);
CUI_API void cui_xml_set_builtins(cui_xml *doc, bool enabled);

/* "app.quit" was fired. Poll it in the loop:
 *     if (cui_xml_quit_requested(doc)) cui_app_quit(app); */
CUI_API bool cui_xml_quit_requested(const cui_xml *doc);
CUI_API void cui_xml_set_quit_requested(cui_xml *doc, bool requested);

/* ------------------------------------------------------------------------ */
/* Elements                                                                 */
/* ------------------------------------------------------------------------ */

/* The element with that name (its Name attribute), or NULL. */
CUI_API cui_xml_node *cui_xml_find(cui_xml *doc, const char *name);
CUI_API cui_xml_node *cui_xml_root(cui_xml *doc);

CUI_API const char *cui_xml_node_tag(const cui_xml_node *node);
/* The Name of the element, or the one the document generated for it. */
CUI_API const char *cui_xml_node_name(const cui_xml_node *node);
/* The text inside the element, computed ({"..."}) for this frame; "" when it
 * has none. */
CUI_API const char *cui_xml_node_text(cui_xml *doc, cui_xml_node *node);
/* An attribute as written (no {"..."}), or "". */
CUI_API const char *cui_xml_node_attr(const cui_xml_node *node, const char *name);
/* The same attribute with {"..."} computed, for this frame: what a tag made by
 * the application reads (its Art="{now.art}", its If, its numbers). */
CUI_API const char *cui_xml_node_attr_value(cui_xml *doc, cui_xml_node *node, const char *name);
/* Adds or changes an attribute; NULL removes it. Returns false for a bad name
 * or a read-only attribute (Name is fine, it takes effect on the next frame). */
CUI_API bool cui_xml_node_set_attr(cui_xml_node *node, const char *name, const char *value);

/* Whether the element is shown, and the built-in show:/hide: actions. */
CUI_API bool cui_xml_node_visible(const cui_xml_node *node);
CUI_API void cui_xml_node_show(cui_xml_node *node, bool shown);
CUI_API bool cui_xml_node_enabled(const cui_xml_node *node);
CUI_API void cui_xml_node_enable(cui_xml_node *node, bool enabled);

/* What the element holds: a bool for a switch, an int for the selected tab,
 * a float for a slider, a double for a number field, the text of a field. */
CUI_API bool cui_xml_node_bool(cui_xml *doc, cui_xml_node *node);
CUI_API int cui_xml_node_int(cui_xml *doc, cui_xml_node *node);
CUI_API float cui_xml_node_float(cui_xml *doc, cui_xml_node *node);
CUI_API double cui_xml_node_double(cui_xml *doc, cui_xml_node *node);
CUI_API const char *cui_xml_node_value(cui_xml *doc, cui_xml_node *node);
/* The index a list reported this frame (the clicked breadcrumb, the row of a
 * table), or -1. */
CUI_API int cui_xml_node_index(const cui_xml_node *node);

/* The id and rectangle the element had in the last frame (0 and an empty
 * rectangle when it was not drawn): for a tour, a tooltip, a popover. */
CUI_API cui_id cui_xml_node_id(const cui_xml_node *node);
CUI_API cui_rect cui_xml_node_rect(const cui_xml_node *node);
/* The element by CUI id (cui_xml_node_id), or NULL. */
CUI_API cui_xml_node *cui_xml_node_by_id(cui_xml *doc, cui_id id);

CUI_API int cui_xml_node_child_count(const cui_xml_node *node);
CUI_API cui_xml_node *cui_xml_node_child(const cui_xml_node *node, int index);
CUI_API cui_xml_node *cui_xml_node_parent(const cui_xml_node *node);
/* True for a comment element (<!-- ... -->): drawn nowhere, kept for the tools. */
CUI_API bool cui_xml_node_is_comment(const cui_xml_node *node);

/* ------------------------------------------------------------------------ */
/* Live reload                                                              */
/* ------------------------------------------------------------------------ */

/* On by default for a document read from a file: cui_xml_poll reads the file
 * again when it changed (its timestamp and size), and only what changed is
 * applied -- a control still in the file keeps what it holds. A file with a
 * mistake leaves the interface as it is and reports the error. */
CUI_API void cui_xml_set_watch(cui_xml *doc, bool enabled);
CUI_API bool cui_xml_get_watch(const cui_xml *doc);
CUI_API void cui_xml_set_reload_callback(cui_xml *doc, cui_xml_reload_fn fn, void *user);
/* Looks at the file now. Returns true when it was read again and used. */
CUI_API bool cui_xml_poll(cui_xml *doc);

/* ------------------------------------------------------------------------ */
/* Templates and tags made by the application                               */
/* ------------------------------------------------------------------------ */

/* Reads <Widget Name="..."> declarations from a text (or the children of a
 * <Widgets> root) so the tags they declare can be used by this document.
 * Returns how many were added. A <Widget> written inside a document already
 * declares one; this is for a library of them shared by several documents. */
CUI_API int cui_xml_register_widgets(cui_xml *doc, const char *text, size_t size);
CUI_API bool cui_xml_is_widget(const cui_xml *doc, const char *tag);

/* A tag drawn by C code: the element is passed to `fn` when the document is
 * drawn, at its place among the other components. Returns false when the name
 * is one of the library's tags or the table is full (64). */
CUI_API bool cui_xml_register_tag(cui_xml *doc, const char *tag, cui_xml_tag_fn fn, void *user);
CUI_API bool cui_xml_is_tag_registered(const cui_xml *doc, const char *tag);

/* ------------------------------------------------------------------------ */
/* What can be written (for a designer, an editor, a help panel)            */
/* ------------------------------------------------------------------------ */

CUI_API int cui_xml_tag_count(void);
CUI_API const char *cui_xml_tag_name(int index);
/* One line about a tag; "" when there is nothing to say. */
CUI_API const char *cui_xml_tag_description(const char *tag);
/* 0: no child element; 1: exactly one; 2: any number. Unknown tags give 2. */
CUI_API int cui_xml_tag_children(const char *tag);
/* The attributes of a tag: those every component has, then its own. */
CUI_API int cui_xml_attribute_count(const char *tag);
CUI_API bool cui_xml_attribute_info(const char *tag, int index, cui_xml_attribute_desc *info);
/* The path an attribute naming a file resolves to (relative ones are looked
 * for next to the document). Valid until the next call. */
CUI_API const char *cui_xml_resolve_path(cui_xml *doc, const char *path);

#ifdef __cplusplus
}
#endif

#endif /* CUI_XML_H */
