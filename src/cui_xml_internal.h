/* Declarations shared by the two files of the XML layer: cui_xml.c (the
 * document: reading, expanding, expressions, values, actions, reloading) and
 * cui_xml_widgets.c (the tags: what each one draws, and the catalog a tool
 * reads). Everything here is internal; the public API is cui/cui_xml.h.
 *
 * Naming: cui__xml_ for internals, cui_xml_ for the public API, CUI__XML_ for
 * constants. All of it is prefixed because tools/amalgamate.py compiles the
 * library as one file.
 */
#ifndef CUI_XML_INTERNAL_H
#define CUI_XML_INTERNAL_H

#include "cui_internal.h"
#include "cui/cui_xml.h"

/* ------------------------------------------------------------------------ */
/* The tags                                                                 */
/* ------------------------------------------------------------------------ */

/* X(id, name, children, description): children is 0 (none), 1 (exactly one)
 * or 2 (any number). The enum, the catalog and the drawing switch all come
 * from this list, so a tag is added in one place. */
#define CUI__XML_TAGS_1(X)                                                                      \
    X(UNKNOWN,     "",               2, "A tag this version of CUI does not know.")              \
    X(WINDOW,      "Window",         2, "The root: a column that fills the window.")             \
    X(COLUMN,      "Column",         2, "Children stacked top to bottom.")                       \
    X(ROW,         "Row",            2, "Children laid out left to right.")                      \
    X(STACK,       "Stack",          2, "Children on top of each other, the last in front.")     \
    X(GRID,        "Grid",           2, "Children in columns of equal width.")                    \
    X(BOX,         "Box",            2, "A plain container: padding, gap, background, border.")  \
    X(CARD,        "Card",           2, "A bordered surface with generous padding.")             \
    X(SCROLL,      "Scroll",         2, "A scrolling area around its children.")                 \
    X(SPLIT,       "Split",          2, "Two children with a draggable divider between them.")    \
    X(CENTER,      "Center",         2, "Centers its child on both axes.")                       \
    X(SPACER,      "Spacer",         0, "Flexible space; fills the room left in a row or column.")\
    X(GAP,         "Gap",            0, "A fixed space, Size points.")                           \
    X(SEPARATOR,   "Separator",      0, "A hairline across the container.")                      \
    X(HEADING,     "Heading",        0, "A heading, Level 1 to 4.")                              \
    X(TEXT,        "Text",           0, "Body text.")                                            \
    X(LABEL,       "Label",          0, "Medium weight text.")                                    \
    X(SECONDARY,   "Secondary",      0, "Secondary color text.")                                 \
    X(MUTED,       "Muted",          0, "Small, muted text.")                                    \
    X(CODE,        "Code",           0, "Inline code.")                                          \
    X(KBD,         "Kbd",            0, "A keyboard keycap, e.g. \"Ctrl K\".")                    \
    X(LINK,        "Link",           0, "A link: OnClicked fires when it is clicked.")            \
    X(ICON,        "Icon",           0, "One icon from the Lucide set.")                         \
    X(BUTTON,      "Button",         0, "A button.")                                              \
    X(ICONBUTTON,  "IconButton",     0, "A square button with only an icon; give it a Tooltip.")  \
    X(CHECKBOX,    "Checkbox",       0, "A checkbox with a label; holds a bool.")                 \
    X(SWITCH,      "Switch",         0, "An on/off switch with a label; holds a bool.")           \
    X(RADIOGROUP,  "RadioGroup",     2, "A group of <Radio> children; holds the selected index.") \
    X(RADIO,       "Radio",          0, "One radio option, inside a RadioGroup.")                 \
    X(SLIDER,      "Slider",         0, "A slider; holds a number between Min and Max.")          \
    X(RANGESLIDER, "RangeSlider",    0, "A slider with two thumbs; holds Low and High.")          \
    X(RATING,      "Rating",         0, "A row of stars; holds an int.")                          \
    X(CHIP,        "Chip",           0, "A selectable chip; holds a bool.")                       \
    X(INPUT,       "Input",          0, "A single line text field; holds text.")                   \
    X(TEXTAREA,    "Textarea",       0, "A multi-line text field; holds text.")                    \
    X(SEARCH,      "Search",         0, "A search field; holds text.")                             \

#define CUI__XML_TAGS_2(X)                                                                      \
    X(NUMBER,      "Number",         0, "A number field with - and +; holds a number.")            \
    X(SELECT,      "Select",         2, "A dropdown; Items or <Item> children; holds an index.")   \
    X(SEGMENTED,   "Segmented",      2, "A row of options with a gliding highlight; an index.")    \
    X(TABS,        "Tabs",           2, "Tab headers; holds the selected index.")                  \
    X(NAVITEM,     "NavItem",        0, "A sidebar navigation item.")                              \
    X(NAVSECTION,  "NavSection",     0, "A small heading above a run of NavItems.")                \
    X(BREADCRUMB,  "Breadcrumb",     2, "Ancestor links; Index reports the one clicked.")          \
    X(PAGINATION,  "Pagination",     0, "Page buttons; holds the page (0 based).")                  \
    X(STEPPER,     "Stepper",        2, "Process steps, Current highlighted.")                      \
    X(PROGRESS,    "Progress",       0, "A progress bar, Value from 0 to 1.")                       \
    X(SPINNER,     "Spinner",        0, "A loading spinner.")                                       \
    X(SKELETON,    "Skeleton",       0, "A loading placeholder.")                                   \
    X(BADGE,       "Badge",          0, "A small tone-colored label.")                              \
    X(AVATAR,      "Avatar",         0, "A person's initials or picture.")                          \
    X(METRIC,      "Metric",         0, "A big value with its label and trend.")                    \
    X(METER,       "Meter",          0, "A labelled usage bar.")                                    \
    X(EMPTYSTATE,  "EmptyState",     0, "An empty state with an optional action.")                   \
    X(ALERT,       "Alert",          0, "An inline alert; Dismissible keeps a close button.")         \
    X(BANNER,      "Banner",         0, "A banner across the top of a panel.")                        \
    X(ACCORDION,   "Accordion",      2, "A collapsible section; holds whether it is open.")           \
    X(TREE,        "Tree",           2, "A tree node with its children under it.")                    \
    X(LISTITEM,    "ListItem",       0, "A selectable row with icon, subtitle and trailing text.")     \
    X(TABLE,       "Table",          2, "<TableColumn> children, then <TableRow> of <Cell>.")            \
    X(TABLECOLUMN, "TableColumn",    0, "One table column: Title, Width, Align, Sortable.")               \
    X(TABLEROW,    "TableRow",       2, "One table row of <Cell> children; Selected draws it chosen.")    \
    X(CELL,        "Cell",           2, "One table cell: Text or components.")                           \
    X(MARKDOWN,    "Markdown",       0, "Markdown text, drawn as native content (Text or Source).")       \
    X(CODEBLOCK,   "CodeBlock",      0, "A code block with syntax highlight (Text or Source).")           \
    X(JSON,        "Json",           0, "A collapsible JSON tree (Text or Source).")                     \
    X(LINECHART,   "LineChart",      2, "A line chart with <Series> children.")                           \
    X(BARCHART,    "BarChart",       2, "A bar chart with <Series> children.")                            \
    X(DONUTCHART,  "DonutChart",     2, "A donut chart with <Slice> children.")                            \
    X(SERIES,      "Series",         0, "One chart series: Name, Values, Color.")                          \
    X(SLICE,       "Slice",          0, "One donut slice: Text, Value.")                                    \
    X(SPARKLINE,   "Sparkline",      0, "A small line under a value.")                                       \
    X(GAUGE,       "Gauge",          0, "A round gauge with a value.")                                        \
    X(PATTERNMETER,"PatternMeter",   2, "A stacked usage bar with <Part> children.")                          \

#define CUI__XML_TAGS_3(X)                                                                      \
    X(PART,        "Part",           0, "One part of a PatternMeter: Name, Value.")                            \
    X(IMAGE,       "Image",          0, "A picture from Source (PNG or JPEG).")                                \
    X(TOOLBAR,     "Toolbar",        2, "A strip of icon buttons and groups.")                                  \
    X(TOOLBARBUTTON, "ToolbarButton",0, "One toolbar icon button.")                                             \
    X(TOOLBARPRIMARY,"ToolbarPrimary",0, "The main action at the end of a toolbar.")                            \
    X(TOOLBARSEPARATOR, "ToolbarSeparator", 0, "A separator between toolbar groups.")                           \
    X(TOOLBARSPACER,"ToolbarSpacer", 0, "Pushes what follows to the right.")                                    \
    X(TOOLBARGROUP,"ToolbarGroup",   2, "A group of toolbar items on one well.")                                \
    X(TOOLBARRADIO,"ToolbarRadio",   2, "Icons of which one is on; holds an index.")                            \
    X(STATUSBAR,   "StatusBar",      2, "A row of small items at the bottom of a window.")                      \
    X(STATUSTEXT,  "StatusText",     0, "A message in a status bar.")                                            \
    X(STATUSITEM,  "StatusItem",     0, "A small clickable status bar item.")                                    \
    X(STATUSPROGRESS, "StatusProgress", 0, "A small progress bar in a status bar.")                              \
    X(STATUSSPACER,"StatusSpacer",   0, "Pushes the following status items to the right.")                        \
    X(MENUBAR,     "MenuBar",        2, "A menu bar with <MenuBarMenu> children.")                                 \
    X(MENUBARMENU, "MenuBarMenu",    2, "One menu of a menu bar, with <MenuItem> children.")                        \
    X(MENU,        "Menu",           2, "A button that opens a dropdown of <MenuItem> children.")                    \
    X(MENUITEM,    "MenuItem",       0, "One item of a menu.")                                                       \
    X(MENUSEPARATOR, "MenuSeparator",0, "A line between menu items.")                                                \
    X(MENULABEL,   "MenuLabel",      0, "A small heading inside a menu.")                                            \
    X(DIALOG,      "Dialog",         2, "A modal dialog over the interface; show:NAME opens it.")                     \
    X(SHORTCUT,    "Shortcut",       0, "A key press anywhere that fires Keys' action.")                              \
    X(VAR,         "Var",            0, "A variable {\"...\"} can read: Name and Value.")                               \
    X(INCLUDE,     "Include",        0, "Brings in another file: Source.")                                            \
    X(REPEAT,      "Repeat",         2, "Repeats its children Count times; Var names the index.")                     \
    X(FOREACH,     "ForEach",        2, "Repeats its children for each Items entry; Var names the entry.")            \
    X(CHILDREN,    "Children",       0, "Inside a <Widget>: where the children given where it is used go.")           \
    X(WIDGET,      "Widget",         2, "Declares a tag of your own: Name, plus its parameters.")                     \
    X(WIDGETS,     "Widgets",        2, "A root that holds <Widget> declarations only.")                              \
    X(ITEM,        "Item",           0, "One entry of a Select, Segmented, Tabs, Breadcrumb, Stepper.")               \
    X(TAB,         "Tab",            0, "One tab of Tabs or DocTabs.")                                                \
    X(DOCTABS,     "DocTabs",        2, "Editor style tabs with <Tab> children; holds the selected index.")            \
    X(STATUSTAG,   "StatusTag",      0, "A colored state tag (CI status, review state...).")                           \
    X(COLORPICKER, "ColorPicker",    0, "A color swatch that opens a picker; holds a color.")                          \
    X(CAROUSEL,    "Carousel",       2, "Slides with a pager; holds the selected index.")                               \
    X(SLIDE,       "Slide",          2, "One slide of a Carousel.")

#define CUI__XML_TAGS_4(X)                                                                      \
    X(BUTTONGROUP, "ButtonGroup",    2, "Connected buttons on one well; put <Button> children in.")  \
    X(COPYBUTTON,  "CopyButton",     0, "An icon button that copies its text and confirms with a check.") \
    X(HOLDBUTTON,  "HoldButton",     0, "A button that must be held until it fills before it fires.")      \
    X(TAGINPUT,    "TagInput",       0, "A field of tags, comma separated; holds text.")                     \
    X(OTP,         "Otp",            0, "Boxes for a one time code; holds text.")                             \
    X(PASSWORDSTRENGTH, "PasswordStrength", 0, "A meter of the bound password's strength.")                    \
    X(DATEPICKER,  "DatePicker",     0, "A date field with a calendar; holds a date.")                          \
    X(RADIOCARDS,  "RadioCards",     2, "Choice cards with <Choice> children; holds an index.")                 \
    X(CHOICE,      "Choice",         0, "One card of a RadioCards: Title, Description, Icon, Badge.")            \
    X(SKELETONGROUP, "SkeletonGroup",2, "Loading bars that animate together, Dither or not.")                     \
    X(STATUSSPINNER, "StatusSpinner",0, "A job's state: idle, busy, success or error.")                           \
    X(TIMELINEITEM, "TimelineItem",  0, "One row of an activity timeline.")                                        \
    X(PROGRESSRING, "ProgressRing",  0, "A circular progress with the value in the middle.")                        \
    X(AVATARGROUP, "AvatarGroup",    0, "A stack of avatars, from Items or <Item> children.")                        \
    X(POPOVER,     "Popover",        2, "A panel over the interface; popover:NAME opens it.")                        \
    X(CONTEXTMENU, "ContextMenu",    2, "A menu on right click over the element before it.")                          \
    X(REORDER,     "Reorder",        2, "Children the user drags into another order; OnChanged fires.")              \
    X(REORDERITEM, "ReorderItem",    2, "One row of a Reorder; its children are drawn inside the row.")

#define CUI__XML_TAGS_5(X)                                                                      \
    X(READOUT,     "Readout",       0, "A label, a big value with its unit, and a line under it.")                  \
    X(DOTDISPLAY,  "DotDisplay",    0, "Text drawn with the 5 by 7 dot font, for clocks and counters.")              \
    X(FOOTER,      "DialogFooter",  2, "The row of buttons at the bottom of a Dialog.")

/* The halves, in order: one list for the enum, the catalog and the drawing
 * switch. It is split because one macro invocation per chunk keeps each
 * expansion short (MSVC is unhappy with very long ones). */
#define CUI__XML_TAGS(X) CUI__XML_TAGS_1(X) CUI__XML_TAGS_2(X) CUI__XML_TAGS_3(X) CUI__XML_TAGS_4(X) CUI__XML_TAGS_5(X)


typedef enum cui_xml_tag_id {
#define CUI__XML_TAG_ENUM(id, name, kids, desc) CUI__XML_TAG_##id,
    CUI__XML_TAGS(CUI__XML_TAG_ENUM)
#undef CUI__XML_TAG_ENUM
    CUI__XML_TAG_COUNT
} cui_xml_tag_id;

/* ------------------------------------------------------------------------ */
/* Nodes                                                                    */
/* ------------------------------------------------------------------------ */

typedef struct cui_xml_var {
    char *name;
    char *value;
} cui_xml_var;

typedef struct cui_xml_attr {
    char *name;
    char *value;
} cui_xml_attr;

struct cui_xml_node {
    char *tag;
    int tag_id;
    char *text;              /* the text inside the element, before {"..."} */
    bool comment;

    cui_xml_attr *attrs;
    int attr_count, attr_cap;

    struct cui_xml_node **kids;
    int kid_count, kid_cap;
    struct cui_xml_node *parent;

    int line, column;
    char *key;               /* Name, or the path the document generated */
    char *name;              /* Name as written, or NULL */
    bool key_owned;

    /* What the element holds between frames (used unless the app bound it). */
    bool b;
    int i;
    float f;                 /* split ratio, slider low */
    double d;                /* number field, range high */
    char *buf;
    size_t buf_cap;
    bool seen;               /* the control was drawn at least once */

    /* Results of the last frame. */
    bool changed;
    cui_id id;
    cui_rect rect;
    int reported;            /* index a list reported, -1 when none */

    /* What the element is shown and enabled, and the built-in actions. */
    bool shown;
    bool enabled;

    /* Parsed attribute caches (freed by node_free, dropped by set_attr). */
    char **items;
    int item_count, item_cap;
    float *nums;
    int num_count, num_cap;
    bool nums_parsed;
    void *cache;             /* widget specific: table columns, chart series */
    void (*cache_free)(void *);
    void *cache2;            /* a second one: chart labels, the icons of a Tabs */
    void (*cache2_free)(void *);

    /* {"..."} variables of this element (a template's parameters, a repeat's
     * index), looked up before the document's. */
    cui_xml_var *vars;
    int var_count, var_cap;
};

/* One <Widget> declaration. */
typedef struct cui_xml_widget {
    char *name;
    cui_xml_attr *params;
    int param_count, param_cap;
    cui_xml_node *body;      /* owned copy of the declaration's content */
} cui_xml_widget;

/* ------------------------------------------------------------------------ */
/* The document                                                             */
/* ------------------------------------------------------------------------ */

typedef enum cui_xml_bind_kind {
    CUI__XML_BIND_NONE = 0,
    CUI__XML_BIND_BOOL,
    CUI__XML_BIND_INT,
    CUI__XML_BIND_FLOAT,
    CUI__XML_BIND_DOUBLE,
    CUI__XML_BIND_TEXT
} cui_xml_bind_kind;

typedef struct cui_xml_bind {
    char *name;
    int kind;
    void *ptr;
    size_t cap;              /* text only */
} cui_xml_bind;

typedef struct cui_xml_action_entry {
    char *name;
    cui_xml_action_fn fn;
    void *user;
} cui_xml_action_entry;

typedef struct cui_xml_tag_entry {
    char *name;
    cui_xml_tag_fn fn;
    void *user;
} cui_xml_tag_entry;

#define CUI__XML_SCRATCH 8     /* values read in one component call do not clobber each other */
#define CUI__XML_MAX_DEPTH 64  /* nesting of elements */
#define CUI__XML_SPLICE_DEPTH 16

struct cui_xml {
    char *path;              /* "" for a text given from memory */
    char *dir;               /* folder of the path, with a trailing separator */

    cui_xml_node *root;
    char *title;
    int width, height;
    cui_theme_mode theme;
    bool has_theme;
    cui_accent accent;
    bool has_accent;

    cui_xml_error_fn error_fn;
    void *error_user;
    char last_error[200];
    char last_message[160];
    int err_line, err_col;   /* where the first mistake was */
    bool ok;

    cui_xml_bind *binds;
    int bind_count, bind_cap;
    cui_xml_var *vars;
    int var_count, var_cap;

    cui_xml_action_entry *actions;
    int action_count, action_cap;
    cui_xml_action_fn any_fn;
    void *any_user;
    bool builtins;
    bool quit;
    char *pending_copy;      /* "copy:TEXT", put on the clipboard at the next frame */
    char *pending_popover;   /* "popover:NAME", toggled at the next frame */

    cui_xml_widget *widgets;
    int widget_count, widget_cap;
    cui_xml_tag_entry *tags;
    int tag_count, tag_cap;

    bool watch;
    bool has_stamp;
    long long stamp[2];      /* size, modification time */
    cui_xml_reload_fn reload_fn;
    void *reload_user;

    uint64_t frame;
    bool prepared;
    int scratch_pos;
    char *scratch[CUI__XML_SCRATCH];
    size_t scratch_cap[CUI__XML_SCRATCH];
    int depth;
};

/* ------------------------------------------------------------------------ */
/* Shared by cui_xml.c and cui_xml_widgets.c                                */
/* ------------------------------------------------------------------------ */

/* Reports a mistake: the callback when it is set, cui_xml_last_error always. */
void cui__xml_error(cui_xml *doc, int line, int column, const char *fmt, ...);
void cui__xml_warn(cui_xml *doc, const cui_xml_node *node, const char *fmt, ...);

/* Zeroed memory for what a document owns (plain malloc, freed by free). */
void *cui__xml_alloc(size_t size);
char *cui__xml_strdup(const char *text);
char *cui__xml_strndup(const char *text, size_t len);

/* The attribute as written, NULL when the element does not have it. */
const char *cui__xml_attr_raw(const cui_xml_node *node, const char *name);
/* The attribute with {"..."} computed, or `fallback` when it is absent. The
 * text lives in the document's frame scratch: use it inside the call. */
const char *cui__xml_attr_text(cui_xml *doc, cui_xml_node *node, const char *name, const char *fallback);
/* The element's own text, computed, or "". */
const char *cui__xml_node_text_computed(cui_xml *doc, cui_xml_node *node);

float cui__xml_attr_num(cui_xml *doc, cui_xml_node *node, const char *name, float fallback);
bool cui__xml_attr_bool(cui_xml *doc, cui_xml_node *node, const char *name, bool fallback);
cui_size cui__xml_attr_size(cui_xml *doc, cui_xml_node *node, const char *name, cui_size fallback);
/* A token color ("accent", "text"), #RRGGBB[AA] or a name; `fallback` (a
 * resolved color) when absent or unreadable. */
cui_color cui__xml_attr_color(cui_xml *doc, cui_xml_node *node, const char *name, cui_color fallback);
/* Index of `value` in `names` (case insensitive), or `fallback`. */
int cui__xml_attr_choice(cui_xml *doc, cui_xml_node *node, const char *name,
                         const char *const *names, int count, int fallback);
/* The entries of Items="a|b|c" or of the element's <Item> children, parsed
 * once and kept: *out points into the node. Returns the count. */
int cui__xml_attr_items(cui_xml *doc, cui_xml_node *node, const char ***out);
/* The numbers of a "1,2,3" attribute, parsed once: *out points into the node. */
int cui__xml_attr_numbers(cui_xml *doc, cui_xml_node *node, const char *name, const float **out);

/* What a control holds, after the bindings: writing through these pointers
 * writes the app's variable (or the element's own state). */
bool *cui__xml_slot_bool(cui_xml *doc, cui_xml_node *node);
int *cui__xml_slot_int(cui_xml *doc, cui_xml_node *node);
float *cui__xml_slot_float(cui_xml *doc, cui_xml_node *node);
double *cui__xml_slot_double(cui_xml *doc, cui_xml_node *node);
char *cui__xml_slot_text(cui_xml *doc, cui_xml_node *node, size_t *cap, size_t want);

/* Fires the action named by an attribute ("onclicked", "onchanged"...). */
void cui__xml_fire(cui_xml *doc, cui_xml_node *node, const char *attr);
void cui__xml_fire_name(cui_xml *doc, cui_xml_node *node, const char *action);
void cui__xml_changed_hook(cui_xml *doc, cui_xml_node *node);

/* {"..."} as a condition (If="..."): `fallback` when it cannot be computed. */
bool cui__xml_test(cui_xml *doc, cui_xml_node *node, const char *expression, bool fallback);

/* The document's frame scratch: a string that stays valid until eight more
 * were asked for. */
char *cui__xml_scratch_alloc(cui_xml *doc, size_t size);

/* Reads a file into memory (NULL when it cannot be read). */
char *cui__xml_read_file(const char *path, size_t *size);

/* The tag table: the id of a tag ("vbox" is COLUMN), CUI__XML_TAG_UNKNOWN
 * when it is not a tag of the library. */
int cui__xml_tag_lookup(const char *tag);

/* The tag the application registered for an element, or NULL when the
 * element is one of the library's. */
cui_xml_tag_entry *cui__xml_tag_entry(cui_xml *doc, const cui_xml_node *node);

/* Draws one element and what it contains (cui_xml_widgets.c). Returns true
 * when the element had its place in the frame. */
bool cui__xml_draw(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, bool disabled);
/* Draws the children of an element the way its tag says (rows, cells...). */
void cui__xml_draw_kids(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, bool disabled);

#endif /* CUI_XML_INTERNAL_H */
