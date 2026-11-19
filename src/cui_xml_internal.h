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

#define CUI__XML_TAGS_6(X)                                                                      \
    X(DROPZONE,    "Dropzone",      0, "A target for files dropped from the desktop, or a click to browse.")         \
    X(SIGNATUREPAD, "SignaturePad", 0, "A pad to draw a signature on, with the mouse or a pen.")                      \
    X(AVATARUPLOAD, "AvatarUpload", 0, "A round avatar that takes a dropped image.")                                 \
    X(TOC,         "Toc",           2, "A table of contents for a scroll area, with <TocItem> children.")             \
    X(TOCITEM,     "TocItem",       0, "One entry of a Toc: its Anchor names a TocAnchor in the scroll area.")         \
    X(TOCANCHOR,   "TocAnchor",     0, "Where a section starts inside the scroll area its Toc follows.")              \
    X(INLINEEDIT,  "InlineEdit",    0, "Text that turns into a field where it stands; holds text.")                    \
    X(SCRUB,       "Scrub",         0, "A number field you drag sideways; holds a number.")                            \
    X(MONEYINPUT,  "MoneyInput",    0, "An amount with grouped digits and a currency; holds a number.")                \
    X(SHORTCUTRECORDER, "ShortcutRecorder", 0, "A field that records a key combination; holds the keys.")               \
    X(EMOJIPICKER, "EmojiPicker",   0, "A grid of emoji with search and groups; reports the one picked.")            \
    X(DOTCHART,    "DotChart",      2, "A dot-matrix chart with <Series> and <Annotation> children.")                    \
    X(ANNOTATION,  "Annotation",    0, "A line or a point over a chart, with a label.")                                \
    X(EVENTTRACK,  "EventTrack",    0, "Moments on an axis that can be dragged, added and removed.")                  \
    X(HOVERCARD,   "HoverCard",     2, "A panel that appears when the pointer rests on it, around its children.")       \
    X(CONFIRMPOPOVER, "ConfirmPopover", 0, "A question anchored to the element before it; OnClicked when confirmed.")    \
    X(EXPANDABLETEXT, "ExpandableText", 0, "Text clamped to a few lines, with Show more.")                             \
    X(EXPANDABLECARD, "ExpandableCard", 2, "A card whose header opens a body; Open says whether it starts open.")       \
    X(COUNTDOWN,   "Countdown",     0, "Digits counting down to a moment, Seconds from the first frame.")               \
    X(CHECKLIST,   "Checklist",     2, "An onboarding checklist card, with <Step> children.")                           \
    X(STEP,        "Step",          0, "One step of a Checklist: title, Description, Action, Done.")                     \
    X(VEIL,        "Veil",          2, "Covers its children with moving dots until Revealed is set.")                  \
    X(SPLITBUTTON, "SplitButton",   0, "A main action with related ones in a menu; reports the one chosen.")            \
    X(TOGGLEGROUP, "ToggleGroup",   0, "A row of toggles, independent or exclusive like radio buttons.")                 \
    X(CONFIRMBUTTON, "ConfirmButton", 0, "A destructive button that asks once before it fires.")                        \
    X(STATUSBUTTON, "StatusButton", 0, "A button that shows the work it started: busy, done or failed.")                \
    X(REACTIONS,   "Reactions",     0, "Reaction pills with counts; a click reacts.")                                   \
    X(THEMESWITCHER, "ThemeSwitcher", 0, "Light, dark and system as a compact segmented control.")                      \
    X(THEMETOGGLE, "ThemeToggle",   0, "A sun and moon button that switches the theme.")                                \
    X(USERMENU,    "UserMenu",      2, "An avatar, a name and a menu of account actions around its children.")          \
    X(SWIPEITEM,   "SwipeItem",     2, "A row whose children slide to reveal actions on Left and Right.")                \
    X(SPEEDDIAL,   "SpeedDial",     0, "A round button that fans out related actions; reports the one chosen.")      \
    X(MULTISELECT, "MultiSelect",   0, "A field of chips over a list with checkboxes; holds which items are chosen.")   \
    X(DATERANGEPICKER, "DateRangePicker", 0, "A start and an end date, with presets; holds the range.")                 \
    X(RANGECALENDAR, "RangeCalendar", 0, "One or two months to pick a range in; holds the range.")                      \
    X(TIMEPICKER,  "TimePicker",    0, "A field with hour and minute columns; holds a time.")                            \
    X(TIMEDIAL,    "TimeDial",      0, "A clock face to set a time on; holds hours and minutes.")                        \
    X(WHEELPICKER, "WheelPicker",   0, "A drum of options; holds the chosen index.")                                    \
    X(RULERSLIDER, "RulerSlider",   0, "A tape measure under a fixed marker; holds a number.")                          \
    X(HISTOGRAMSLIDER, "HistogramSlider", 0, "A range over the data it filters; holds two numbers.")                  \
    X(CHATMESSAGE, "ChatMessage",   0, "One message of a conversation; Mine aligns it to the right.")                   \
    X(REASONING,   "Reasoning",     2, "What the model thought, folded away; Thinking keeps it open.")                  \
    X(TOOLCALL,    "ToolCall",      0, "One call a tool made, with its input and output.")                              \
    X(AGENTSTATUS, "AgentStatus",   0, "What the agent is doing now, and for how long.")                                \
    X(TYPEWRITER,  "Typewriter",    0, "Text that types itself in.")                                                    \
    X(CITATION,    "Citation",      0, "A number that shows a source from a SourceList on hover.")                      \
    X(SOURCELIST,  "SourceList",    2, "The documents an answer used, with <Source> children.")                         \
    X(SOURCE,      "Source",        0, "One document: Title, URL, Snippet.")                                            \
    X(RESPONSEACTIONS, "ResponseActions", 0, "Copy, regenerate, vote and share under an answer.")                       \
    X(CONTEXTMETER, "ContextMeter", 2, "A ring of how full the context is, with <Part> children.")                      \
    X(CHATCOMPOSER, "ChatComposer", 0, "A growing field with attachments and a model picker; holds text.")              \
    X(AGENTPLAN,   "AgentPlan",     2, "Steps with their status, with <PlanStep> children.")                            \
    X(PLANSTEP,    "PlanStep",      0, "One step: Detail and Status.")                                                   \
    X(PROMPTSUGGESTIONS, "PromptSuggestions", 2, "Cards to start a conversation with, from <Suggestion> children.")     \
    X(SUGGESTION,  "Suggestion",    0, "One card: Icon, Detail.")                                                       \
    X(KNOB,        "Knob",          0, "A round control turned by dragging; holds a number.")                            \
    X(FADER,       "Fader",         0, "A track with a handle and a level meter beside it; holds a number.")             \
    X(LEVELMETER,  "LevelMeter",    0, "Bars that follow the levels they are given.")                                    \
    X(TOUR,        "Tour",          2, "A guided tour of elements by name, with <TourStep> children.")                   \
    X(TOURSTEP,    "TourStep",      0, "One step: Target, Description, Placement.")                                     \
    X(RATIOSLIDER, "RatioSlider",   0, "Two named parts with a bar between them; holds a share.")                        \
    X(GRADIENTSLIDER, "GradientSlider", 0, "A slider whose track is the value: hue, white balance, opacity.")            \
    X(STEPSLIDER,  "StepSlider",    0, "A slider that snaps to a few stops; holds an index.")                            \
    X(PALETTESTRIP, "PaletteStrip", 0, "Swatches that lock and shuffle; holds the colors.")                             \
    X(ASPECTPICKER, "AspectPicker", 0, "Ratio choices drawn as shapes; holds an index.")                                \
    X(OPTIONACTION, "OptionAction", 0, "A compact choice beside the action it feeds; holds an index.")                   \
    X(SEEDFIELD,   "SeedField",     0, "A random seed with a dice and a lock; holds a number.")                         \
    X(LOADER,      "Loader",        0, "A looping wait: dots, bars, pulse, orbit or grid.")                              \
    X(SHIMMERTEXT, "ShimmerText",   0, "Text with a highlight sweeping across it.")                                      \
    X(TEXTREVEAL,  "TextReveal",    0, "Words that fade and rise into place.")                                           \
    X(WORDROTATE,  "WordRotate",    0, "One word at a time, rolling to the next.")                                       \
    X(GRADIENTTEXT, "GradientText", 0, "Text painted with a gradient, drifting when Animate is set.")                    \
    X(SPOTLIGHT,   "Spotlight",     2, "A card with a light that follows the pointer, around its children.")             \
    X(DOCK,        "Dock",          2, "A row of icons that grow as the pointer passes, with <DockItem> children.")       \
    X(DOCKITEM,    "DockItem",      0, "One icon of a Dock: Icon, Color, Running, Badge.")                               \
    X(DOCKSEPARATOR, "DockSeparator", 0, "A line between groups of a Dock.")                                             \
    X(CONTROLTILE, "ControlTile",   0, "A large toggle for quick settings; holds a bool.")                               \
    X(NOWPLAYING,  "NowPlaying",    0, "Artwork, a scrubber and transport; holds the track and its position.")           \
    X(WAVEFORM,    "Waveform",      0, "Bars from peaks; holds how much has played.")                                    \
    X(TABBAR,      "TabBar",        2, "A bottom tab bar with a gliding pill, from <TabItem> children.")                 \
    X(TABITEM,     "TabItem",       0, "One tab: Icon, Badge.")                                                          \
    X(TREEMAP,     "Treemap",       2, "Rectangles by value, from <Part> children.")                                      \
    X(FUNNEL,      "Funnel",        2, "Stages that narrow, from <Part> children.")                                       \
    X(SPLITBAR,    "SplitBar",      2, "One bar split into parts, with a legend, from <Part> children.")                  \
    X(WAFFLE,      "Waffle",        2, "A grid of squares by share, from <Part> children.")                               \
    X(BARLIST,     "BarList",       2, "Ranked bars from <Part> children.")                                               \
    X(ACTIVITYRINGS, "ActivityRings", 2, "Concentric rings, from <Part> children.")                                       \
    X(HEATMAP,     "Heatmap",       0, "A grid of cells from Values, Rows and Cols.")                                     \
    X(CALENDARHEATMAP, "CalendarHeatmap", 0, "A year of activity, one square a day.")                                     \
    X(RADIALGAUGE, "RadialGauge",   0, "A value on an arc, with bands and a target.")                                     \
    X(RADAR,       "Radar",         2, "Series over shared Axes, from <Series> children.")                                \
    X(SCATTER,     "Scatter",       2, "Points per series, from <Series Points=\"x,y;...\"> children.")                   \
    X(SANKEY,      "Sankey",        0, "Flows between named Nodes, listed in Links.")                                    \
    X(KANBAN,      "Kanban",        2, "A board of <KanbanColumn> children; OnChanged when a card moves.")               \
    X(KANBANCOLUMN, "KanbanColumn", 2, "One column: Dot, Limit, Width, and <KanbanCard> children.")                      \
    X(KANBANCARD,  "KanbanCard",    2, "One card: Card is its id; its children are its content.")                        \
    X(WEEKCALENDAR, "WeekCalendar", 2, "Seven days of events, from <Event> children.")                                   \
    X(CALEVENT,    "Event",         0, "One event: Day, Start, End, Detail, Tone.")                                       \
    X(GANTT,       "Gantt",         2, "A plan of <Task> bars, Days long.")                                              \
    X(TASK,        "Task",          0, "One bar: Owner, Start, End, Progress, DependsOn, Milestone.")                    \
    X(MONTHCALENDAR, "MonthCalendar", 2, "A month grid, from <MonthEvent> children.")                                     \
    X(MONTHEVENT,  "MonthEvent",    0, "One event: Day, Span, Time, AllDay, Tone.")                                      \
    X(VOICENOTE,   "VoiceNote",     0, "A recorded message with its waveform; holds the clip and its position.")         \
    X(VOICEWAVE,   "VoiceWave",     0, "A strip of levels, one column each, for one voice or two.")                       \
    X(VOICESESSION, "VoiceSession", 0, "A spoken conversation: the strip, the live transcript, Mute and End.")          \
    X(CHATDIVIDER, "ChatDivider",   0, "A line with a label between groups of messages, e.g. Today.")                    \
    X(TYPINGINDICATOR, "TypingIndicator", 0, "Bouncing dots under the last message: \"Jonas is typing\".")               \
    X(COMMENT,     "Comment",       0, "One comment with replies, likes and a badge; holds the like.")                    \
    X(PRESENCE,    "Presence",      0, "Overlapping avatars of who is here, from Users.")                                \
    X(COUNTER,     "Counter",       0, "A figure whose changed digits roll into place.")                                \
    X(TICKER,      "Ticker",        0, "A strip of figures scrolling by, pausing on hover.")                    \
    X(CARDSTACK,   "CardStack",     2, "Cards a flick throws away, from <CardStackItem> children.")                \
    X(CARDSTACKITEM, "CardStackItem", 2, "One card of a CardStack; its children are its content.")                       \
    X(FILTERBAR,   "FilterBar",     2, "A search field and chips; holds what is chosen and typed.")                       \
    X(FIELD,       "Field",         0, "One filter: Icon, Options.")                                                      \
    X(TREETABLE,   "TreeTable",     2, "A table with disclosure and indentation, from <TreeRow> children.")               \
    X(TREEROW,     "TreeRow",       0, "One row: Depth, Open, Icon, Cells.")                                              \
    X(PROBLEMS,    "Problems",      2, "Diagnostics grouped by file, from <Diagnostic> children.")                          \
    X(DIAGNOSTIC,  "Diagnostic",    0, "One diagnostic: Severity, File, Line, Column, Source, Code.")                       \
    X(KVEDITOR,    "KvEditor",      2, "Key and value rows, from <Pair> children; holds the pairs.")                        \
    X(PAIR,        "Pair",          0, "One pair: Key, Value, Enabled, Secret.")                                           \
    X(PIPELINE,    "Pipeline",      2, "Stages of <Stage> names with <Job> children.")                                       \
    X(STAGE,       "Stage",         0, "One stage name.")                                                                  \
    X(JOB,         "Job",           0, "One job: Stage, Status, Duration, AllowFailure.")                                  \
    X(QUERYBUILDER, "QueryBuilder", 2, "Nested <Condition> groups over <Field> names.")                                   \
    X(CONDITION,   "Condition",     0, "A group (Group=""all|any"") or a rule (Field, Op, Value).")                          \
    X(SCHEDULEPICKER, "SchedulePicker", 0, "A cron schedule edited the friendly way; holds the expression.")              \
    X(KEYMAP,      "Keymap",        2, "A keyboard of <Binding> children and <Category> names.")                          \
    X(BINDING,     "Binding",       0, "One binding: Keys (""primary+s""), Category.")                                     \
    X(CATEGORY,    "Category",      0, "One category name of a Keymap.")                                                \
    X(TESTEXPLORER, "TestExplorer", 2, "Suites and tests, from <Test> children.")                                         \
    X(TEST,        "Test",          0, "One test: Depth, Status, Duration, Message.")                                     \
    X(STACKTRACE,  "StackTrace",    2, "An exception and its <Frame> children.")                                          \
    X(FRAME,       "Frame",         0, "One frame: File, Line, Column, Library, Source, First.")                          \
    X(REQUESTBAR,  "RequestBar",    0, "The method, the URL and Send; holds both.")                                       \
    X(RESPONSESTATUS, "ResponseStatus", 0, "A response's status, time and size.")                                         \
    X(URLPARAMS,   "UrlParams",     0, "The query of a URL as key and value rows; holds them.")                           \
    X(COVERAGE,    "Coverage",      2, "Coverage per file, from <File> children.")                                        \
    X(COVERAGEFILE, "CoverageFile", 0, "One file: Lines, LinesHit, Branches, BranchesHit, Functions, FunctionsHit.")      \
    X(FEATUREFLAGS, "FeatureFlags", 2, "A switch per environment, from <Flag> and <Env> children.")                       \
    X(FLAG,        "Flag",          0, "One flag: Description, Envs, Rollouts.")                                          \
    X(ENV,         "Env",           0, "One environment name.")                                                           \
    X(APIEXPLORER, "ApiExplorer",   2, "Endpoints by tag, from <Endpoint> children; holds the open one.")                 \
    X(ENDPOINT,    "Endpoint",      2, "One endpoint: Method, Path, Category, Deprecated, with <Param> children.")        \
    X(PARAM,       "Param",         0, "One parameter: In, Type, Required, Description.")                     \
    X(REGEXTESTER, "RegexTester",   0, "A pattern and a sample text; matches are marked as you type.")                 \
    X(NODEEDITOR,  "NodeEditor",    2, "A canvas of <Node> children, joined by <NodeLink>.")                       \
    X(NODE,        "Node",          2, "One node: Width, Icon, with <NodeInput>, <NodeOutput> and <NodeContent>.")      \
    X(NODEINPUT,   "NodeInput",     0, "An input pin: Pin, Type.")                                                      \
    X(NODEOUTPUT,  "NodeOutput",    0, "An output pin: Pin, Type.")                                                     \
    X(NODECONTENT, "NodeContent",   2, "The row of a node that holds ordinary components.")                             \
    X(NODELINK,    "NodeLink",      0, "A link: Link, From pin, To pin.")                                               \
    X(SELECTIONRING, "SelectionRing", 0, "A ring in someone's color around what it follows: Tone, Name.")              \
    X(CANVASCONTROLS, "CanvasControls", 2, "A zoom rail over the previous component, with <CanvasToggle> children.")   \
    X(CANVASTOGGLE, "CanvasToggle", 0, "One switch of the rail: Icon, Text, On.")                                       \
    X(LIVECURSORS, "LiveCursors",   0, "Other people's pointers on the canvas, from Users.")                           \
    X(PRESENCEPILL, "PresencePill", 0, "Who is here, as overlapping avatars, from Users.")                             \
    X(FORMATBAR,   "FormatBar",     0, "Color, weight, style, alignment and lists for a selection; holds them.")

/* The halves, in order: one list for the enum, the catalog and the drawing
 * switch. It is split because one macro invocation per chunk keeps each
 * expansion short (MSVC is unhappy with very long ones). */
#define CUI__XML_TAGS(X) CUI__XML_TAGS_1(X) CUI__XML_TAGS_2(X) CUI__XML_TAGS_3(X) CUI__XML_TAGS_4(X) CUI__XML_TAGS_5(X) \
                          CUI__XML_TAGS_6(X)


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
    char *pending_popover;   /* "popover:NAME", consumed in the target's ID scope */

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
