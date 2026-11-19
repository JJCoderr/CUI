/* CUI XML: the tags. Every element of a document becomes one call (or a few)
 * to a CUI component, every frame, in document order. The values a component
 * keeps live in the element (or in the variable the app bound to it), which
 * is what makes a text written once behave like a retained interface.
 *
 * The tag list is the X macro CUI__XML_TAGS in cui_xml_internal.h: the
 * drawing switch, the catalog and the alias table all come from it.
 */
#include "cui_xml_internal.h"
#include "cui_xml_icons.h"

/* ------------------------------------------------------------------------ */
/* The tag table                                                            */
/* ------------------------------------------------------------------------ */

typedef struct cui__xml_tag_row {
    const char *name;
    int id;
} cui__xml_tag_row;

static const cui__xml_tag_row cui__xml_tag_rows[] = {
#define CUI__XML_TAG_ROW(id, name, kids, desc) { name, CUI__XML_TAG_##id },
    CUI__XML_TAGS(CUI__XML_TAG_ROW)
#undef CUI__XML_TAG_ROW
    /* the names a document is likely to write instead of the canonical one */
    { "VBox", CUI__XML_TAG_COLUMN },
    { "VerticalBox", CUI__XML_TAG_COLUMN },
    { "HBox", CUI__XML_TAG_ROW },
    { "HorizontalBox", CUI__XML_TAG_ROW },
    { "Overlay", CUI__XML_TAG_STACK },
    { "Panel", CUI__XML_TAG_BOX },
    { "ScrollBox", CUI__XML_TAG_SCROLL },
    { "Column", CUI__XML_TAG_COLUMN },
    { "Row", CUI__XML_TAG_ROW },
    { "Group", CUI__XML_TAG_CARD },
    { "Paragraph", CUI__XML_TAG_TEXT },
    { "CheckButton", CUI__XML_TAG_CHECKBOX },
    { "TextField", CUI__XML_TAG_INPUT },
    { "LineEdit", CUI__XML_TAG_INPUT },
    { "Combo", CUI__XML_TAG_SELECT },
    { "ComboBox", CUI__XML_TAG_SELECT },
    { "Dropdown", CUI__XML_TAG_SELECT },
    { "TabView", CUI__XML_TAG_TABS },
    { "Image", CUI__XML_TAG_IMAGE },
    { "ProgressBar", CUI__XML_TAG_PROGRESS },
    { "SeparatorLine", CUI__XML_TAG_SEPARATOR },
    { "Title", CUI__XML_TAG_HEADING },
    { "LabelText", CUI__XML_TAG_LABEL },
    { "SvgIcon", CUI__XML_TAG_ICON },
    { "AvatarImage", CUI__XML_TAG_AVATAR },
    { "Notification", CUI__XML_TAG_ALERT },
    { "Expandable", CUI__XML_TAG_ACCORDION },
    { "TreeView", CUI__XML_TAG_TREE },
    { "DataTable", CUI__XML_TAG_TABLE },
    { "MenuEntry", CUI__XML_TAG_MENUITEM },
    { "Popup", CUI__XML_TAG_DIALOG },
    { "Keys", CUI__XML_TAG_SHORTCUT },
    { "Variable", CUI__XML_TAG_VAR },
    { "Loop", CUI__XML_TAG_REPEAT },
    { "Slot", CUI__XML_TAG_CHILDREN }
};

int cui__xml_tag_lookup(const char *tag) {
    int i;
    if (!tag || !*tag) return CUI__XML_TAG_UNKNOWN;
    for (i = 0; i < CUI_COUNTOF(cui__xml_tag_rows); i++) {
        const char *name = cui__xml_tag_rows[i].name;
        const char *a = tag;
        const char *b = name;
        if (!b || !*b) continue;
        while (*a && *b && (cui__ascii_lower((unsigned char)*a) == cui__ascii_lower((unsigned char)*b))) {
            a++;
            b++;
        }
        if (!*a && !*b) return cui__xml_tag_rows[i].id;
    }
    return CUI__XML_TAG_UNKNOWN;
}

static const char *const cui__xml_tag_names[] = {
#define CUI__XML_TAG_NAME(id, name, kids, desc) name,
    CUI__XML_TAGS(CUI__XML_TAG_NAME)
#undef CUI__XML_TAG_NAME
};

static const char *const cui__xml_tag_descs[] = {
#define CUI__XML_TAG_DESC(id, name, kids, desc) desc,
    CUI__XML_TAGS(CUI__XML_TAG_DESC)
#undef CUI__XML_TAG_DESC
};

int cui_xml_tag_count(void) { return (int)(CUI_COUNTOF(cui__xml_tag_names) - 1); }

const char *cui_xml_tag_name(int index) {
    /* 0 is <Window>: the catalog lists the tags a document can write, and
     * UNKNOWN (the empty name) is not one of them */
    if (index < 0 || index + 1 >= (int)CUI_COUNTOF(cui__xml_tag_names)) return "";
    return cui__xml_tag_names[index + 1];
}

const char *cui_xml_tag_description(const char *tag) {
    int id = cui__xml_tag_lookup(tag);
    if (id <= 0 || id >= (int)CUI_COUNTOF(cui__xml_tag_descs)) return "";
    return cui__xml_tag_descs[id];
}

int cui_xml_tag_children(const char *tag) {
    switch (cui__xml_tag_lookup(tag)) {
#define CUI__XML_TAG_KIDS(id, name, kids, desc) case CUI__XML_TAG_##id: return kids;
        CUI__XML_TAGS(CUI__XML_TAG_KIDS)
#undef CUI__XML_TAG_KIDS
    default:
        break;
    }
    return 2;
}

/* ------------------------------------------------------------------------ */
/* The attributes a document can write                                      */
/* ------------------------------------------------------------------------ */

typedef struct cui__xml_attr_row {
    const char *tag;          /* NULL: every element */
    const char *name;
    cui_xml_value_kind kind;
    const char *choices;
    const char *description;
} cui__xml_attr_row;

static const cui__xml_attr_row cui__xml_attr_rows[] = {
    { NULL, "Name", CUI_XML_TEXT, NULL, "Identity of the element: cui_xml_find, cui_xml_get_*, actions." },
    { NULL, "Id", CUI_XML_TEXT, NULL, "The same as Name." },
    { NULL, "Bind", CUI_XML_TEXT, NULL, "Name of the variable the value goes to instead of the element." },
    { NULL, "Text", CUI_XML_TEXT, NULL, "The text shown; {\"...\"} is computed. Element text wins." },
    { NULL, "Width", CUI_XML_CHOICE, "fit|fill|8|50%|1*", "Fixed points, a share of the parent, or fill." },
    { NULL, "Height", CUI_XML_CHOICE, "fit|fill|32|50%|1*", "Fixed points, a share of the parent, or fill." },
    { NULL, "Fill", CUI_XML_NUMBER, NULL, "Share of the free width in a Row (1 = one share)." },
    { NULL, "FillV", CUI_XML_NUMBER, NULL, "Share of the free height in a Column." },
    { NULL, "Pad", CUI_XML_NUMBER, NULL, "Padding inside a container, all sides." },
    { NULL, "PadX", CUI_XML_NUMBER, NULL, "Padding left and right." },
    { NULL, "PadY", CUI_XML_NUMBER, NULL, "Padding top and bottom." },
    { NULL, "Gap", CUI_XML_NUMBER, NULL, "Space between the children." },
    { NULL, "Align", CUI_XML_CHOICE, "default|start|center|end|stretch", "Cross axis alignment of the children." },
    { NULL, "Justify", CUI_XML_CHOICE, "start|center|end|between|around", "Main axis distribution of the children." },
    { NULL, "If", CUI_XML_TEXT, NULL, "The element is drawn only when this {\"...\"} is true." },
    { NULL, "Visible", CUI_XML_BOOL, NULL, "false hides the element and what it contains." },
    { NULL, "Disabled", CUI_XML_BOOL, NULL, "true greys the element; its children too." },
    { NULL, "Tooltip", CUI_XML_TEXT, NULL, "Text shown when the pointer rests on the element." },
    { NULL, "OnClicked", CUI_XML_ACTION, NULL, "Action fired when the element is clicked." },
    { NULL, "OnChanged", CUI_XML_ACTION, NULL, "Action fired when its value changes." },
    { NULL, "OnSubmitted", CUI_XML_ACTION, NULL, "Action fired when Enter is pressed in a field." },
    { NULL, "OnAction", CUI_XML_ACTION, NULL, "Action fired by the element's own action (an EmptyState)." },
    { NULL, "Item", CUI_XML_TEXT, NULL, "One entry of a Select, Segmented, Tabs, Breadcrumb or Stepper." },
    { NULL, "Items", CUI_XML_LIST, "One|Two|Three", "The entries of a list, separated by '|'." },
    { NULL, "Into", CUI_XML_TEXT, NULL, "Inside a <Widget>'s use: which <Children Into=\"...\"/> it goes to." },
    { "Window", "Title", CUI_XML_TEXT, NULL, "The window's title (cui_xml_title)." },
    { "Window", "Theme", CUI_XML_CHOICE, "light|dark", "The theme applied on the first frame." },
    { "Window", "Accent", CUI_XML_CHOICE, "neutral|violet|blue|green|amber|orange|coral|rose", "The accent color applied." },
    { "Column", "Pad", CUI_XML_NUMBER, NULL, "Padding inside." },
    { "Row", "Wrap", CUI_XML_BOOL, NULL, "Children that do not fit move to a new line." },
    { "Box", "Background", CUI_XML_COLOR, NULL, "Fill color: a token name or #RRGGBB." },
    { "Box", "Border", CUI_XML_NUMBER, NULL, "Border width in points." },
    { "Box", "BorderColor", CUI_XML_COLOR, NULL, "Border color." },
    { "Box", "Radius", CUI_XML_NUMBER, NULL, "Corner radius in points." },
    { "Box", "Elevation", CUI_XML_CHOICE, "none|resting|raised|floating", "Shadow of the surface." },
    { "Card", "Hoverable", CUI_XML_BOOL, NULL, "Lifts on hover and takes clicks (OnClicked)." },
    { "Card", "Muted", CUI_XML_BOOL, NULL, "Muted surface without a border." },
    { "Card", "Selected", CUI_XML_BOOL, NULL, "Accent border." },
    { "Card", "MinWidth", CUI_XML_NUMBER, NULL, "Narrowest the card gets before its content wraps." },
    { "Card", "MaxWidth", CUI_XML_NUMBER, NULL, "Widest it grows, e.g. a fill card that stops on wide screens." },
    { "Scroll", "Horizontal", CUI_XML_BOOL, NULL, "Also scroll sideways." },
    { "Scroll", "Scroll", CUI_XML_BOOL, NULL, "Scroll the content vertically (clips it)." },
    { "Split", "Ratio", CUI_XML_NUMBER, NULL, "Size of the first pane, 0 to 1 (default 0.5)." },
    { "Split", "Vertical", CUI_XML_BOOL, NULL, "Panes on top of each other instead of side by side." },
    { "Heading", "Level", CUI_XML_NUMBER, NULL, "1 to 4." },
    { "Text", "Size", CUI_XML_NUMBER, NULL, "Text size in points; the theme's body size when absent." },
    { "Text", "Color", CUI_XML_COLOR, NULL, "Text color." },
    { "Text", "Align", CUI_XML_CHOICE, "start|center|right|left|end", "Alignment of the text." },
    { "Text", "Nowrap", CUI_XML_BOOL, NULL, "One line, with an ellipsis." },
    { "Icon", "Icon", CUI_XML_TEXT, NULL, "Name of a Lucide icon, e.g. plus." },
    { "Icon", "Size", CUI_XML_NUMBER, NULL, "Size in points (default 18)." },
    { "Button", "Variant", CUI_XML_CHOICE, "default|secondary|primary|outline|ghost|danger|link", "How the button looks." },
    { "Button", "Size", CUI_XML_CHOICE, "md|sm|lg", "Control height." },
    { "Button", "Icon", CUI_XML_TEXT, NULL, "Leading icon." },
    { "Button", "IconRight", CUI_XML_TEXT, NULL, "Trailing icon." },
    { "Button", "Loading", CUI_XML_BOOL, NULL, "A spinner instead of the label; clicks are ignored." },
    { "Button", "FullWidth", CUI_XML_BOOL, NULL, "As wide as the parent." },
    { "Button", "Selected", CUI_XML_BOOL, NULL, "Drawn toggled on." },
    { "Button", "Shortcut", CUI_XML_TEXT, NULL, "Shown in the tooltip, e.g. \"Ctrl S\"." },
    { "Checkbox", "Description", CUI_XML_TEXT, NULL, "Second line under the label." },
    { "Switch", "Description", CUI_XML_TEXT, NULL, "Second line under the label." },
    { "Slider", "Min", CUI_XML_NUMBER, NULL, "Lowest value." },
    { "Slider", "Max", CUI_XML_NUMBER, NULL, "Highest value." },
    { "Slider", "Value", CUI_XML_NUMBER, NULL, "Starting value (the first frame)." },
    { "Slider", "Step", CUI_XML_NUMBER, NULL, "0 keeps it continuous." },
    { "Slider", "ShowValue", CUI_XML_BOOL, NULL, "The value at the end of the track." },
    { "Slider", "Format", CUI_XML_TEXT, NULL, "printf format of the value, default \"%.0f\"." },
    { "RangeSlider", "Low", CUI_XML_NUMBER, NULL, "Starting low value." },
    { "RangeSlider", "High", CUI_XML_NUMBER, NULL, "Starting high value." },
    { "Rating", "Max", CUI_XML_NUMBER, NULL, "Number of stars (default 5)." },
    { "Input", "Placeholder", CUI_XML_TEXT, NULL, "Text shown while the field is empty." },
    { "Input", "Label", CUI_XML_TEXT, NULL, "Label above the field." },
    { "Input", "Hint", CUI_XML_TEXT, NULL, "Helper text below the field." },
    { "Input", "Error", CUI_XML_TEXT, NULL, "Error text below the field." },
    { "Input", "Icon", CUI_XML_TEXT, NULL, "Leading icon." },
    { "Input", "Password", CUI_XML_BOOL, NULL, "Dots, with an eye button." },
    { "Input", "Clearable", CUI_XML_BOOL, NULL, "An x button when the field is not empty." },
    { "Input", "ReadOnly", CUI_XML_BOOL, NULL, "The text cannot be edited." },
    { "Input", "Autofocus", CUI_XML_BOOL, NULL, "Takes the keyboard when it appears." },
    { "Input", "Prefix", CUI_XML_TEXT, NULL, "Muted text before the value, e.g. \"$\"." },
    { "Input", "Suffix", CUI_XML_TEXT, NULL, "Muted text after the value, e.g. \"USD\"." },
    { "Input", "Value", CUI_XML_TEXT, NULL, "Starting text (the first frame)." },
    { "Textarea", "Height", CUI_XML_NUMBER, NULL, "Height in points; negative fills the space." },
    { "Number", "Min", CUI_XML_NUMBER, NULL, "Lowest accepted value." },
    { "Number", "Max", CUI_XML_NUMBER, NULL, "Highest accepted value." },
    { "Number", "Step", CUI_XML_NUMBER, NULL, "What - and + add." },
    { "Number", "Decimals", CUI_XML_NUMBER, NULL, "Digits shown." },
    { "Select", "Searchable", CUI_XML_BOOL, NULL, "Filter the options by typing." },
    { "Tabs", "Variant", CUI_XML_CHOICE, "underline|pill", "How the headers look." },
    { "Segmented", "FullWidth", CUI_XML_BOOL, NULL, "As wide as the parent." },
    { "NavItem", "Icon", CUI_XML_TEXT, NULL, "Icon before the label." },
    { "NavItem", "Badge", CUI_XML_TEXT, NULL, "Small text at the right edge, e.g. a count." },
    { "NavItem", "Selected", CUI_XML_BOOL, NULL, "The current page." },
    { "NavItem", "Group", CUI_XML_TEXT, NULL, "Items of one group share the highlight." },
    { "Pagination", "PageCount", CUI_XML_NUMBER, NULL, "Number of pages (default 5)." },
    { "Stepper", "Current", CUI_XML_NUMBER, NULL, "Index of the step being done." },
    { "Progress", "Value", CUI_XML_NUMBER, NULL, "0 to 1." },
    { "Progress", "Indeterminate", CUI_XML_BOOL, NULL, "No value: something is running." },
    { "Progress", "Tone", CUI_XML_CHOICE, "neutral|info|success|warning|danger|violet|teal|rose|accent", "Color of the fill." },
    { "Progress", "Label", CUI_XML_TEXT, NULL, "A row above the bar with this name." },
    { "Progress", "ShowValue", CUI_XML_BOOL, NULL, "A percentage at the end of that row." },
    { "Progress", "Dither", CUI_XML_BOOL, NULL, "Draw the fill in dots." },
    { "Spinner", "Size", CUI_XML_NUMBER, NULL, "Height in points (default 20)." },
    { "Spinner", "Style", CUI_XML_CHOICE, "arc|comet|dotmatrix|filldrain|radar|ripple|upload|download|transfer|orbit|bounce|scan|pulse|spiral|plasma", "The kind of wait it draws." },
    { "Badge", "Tone", CUI_XML_CHOICE, "neutral|info|success|warning|danger|violet|teal|rose|accent", "Color of the label." },
    { "Badge", "Dot", CUI_XML_BOOL, NULL, "A dot before the text." },
    { "Badge", "Solid", CUI_XML_BOOL, NULL, "Filled instead of tinted." },
    { "Avatar", "Size", CUI_XML_NUMBER, NULL, "Diameter in points." },
    { "Avatar", "Status", CUI_XML_CHOICE, "online|busy|away|", "A dot at the bottom right." },
    { "Avatar", "Square", CUI_XML_BOOL, NULL, "Rounded square instead of a circle." },
    { "Metric", "Label", CUI_XML_TEXT, NULL, "What the value measures." },
    { "Metric", "Value", CUI_XML_TEXT, NULL, "The big value." },
    { "Metric", "Delta", CUI_XML_NUMBER, NULL, "Change in percent, with a trend arrow." },
    { "Metric", "Icon", CUI_XML_TEXT, NULL, "Icon beside the label." },
    { "Metric", "Values", CUI_XML_NUMBERS, NULL, "A sparkline under the value." },
    { "Meter", "Used", CUI_XML_NUMBER, NULL, "What is used." },
    { "Meter", "Total", CUI_XML_NUMBER, NULL, "What there is." },
    { "Meter", "Unit", CUI_XML_TEXT, NULL, "After every value, e.g. GB." },
    { "EmptyState", "Description", CUI_XML_TEXT, NULL, "Text under the title." },
    { "EmptyState", "Action", CUI_XML_TEXT, NULL, "Label of the button under it; OnAction fires." },
    { "Alert", "Description", CUI_XML_TEXT, NULL, "Text under the title." },
    { "Alert", "Dismissible", CUI_XML_BOOL, NULL, "A close button." },
    { "Accordion", "Open", CUI_XML_BOOL, NULL, "Whether it starts open." },
    { "Tree", "Open", CUI_XML_BOOL, NULL, "Whether it starts open." },
    { "Tree", "Leaf", CUI_XML_BOOL, NULL, "A node without children." },
    { "ListItem", "Subtitle", CUI_XML_TEXT, NULL, "Second line under the title." },
    { "ListItem", "Trailing", CUI_XML_TEXT, NULL, "Muted text at the right edge." },
    { "ListItem", "Selected", CUI_XML_BOOL, NULL, "Drawn chosen." },
    { "Table", "Striped", CUI_XML_BOOL, NULL, "Alternate row colors." },
    { "Table", "Compact", CUI_XML_BOOL, NULL, "Rows 36 points tall instead of 44." },
    { "Table", "Selectable", CUI_XML_BOOL, NULL, "Rows take the keyboard and the hand cursor." },
    { "Table", "AutoSort", CUI_XML_BOOL, NULL, "Sort rows when a sortable header is clicked." },
    { "Cell", "SortValue", CUI_XML_TEXT, NULL, "Sort key for a cell with custom content." },
    { "TableColumn", "Title", CUI_XML_TEXT, NULL, "Header text." },
    { "TableColumn", "Sortable", CUI_XML_BOOL, NULL, "Clicking the header sorts." },
    { "TableColumn", "Align", CUI_XML_CHOICE, "start|center|right|left|end", "Alignment of the cells." },
    { "Markdown", "Source", CUI_XML_FILE, NULL, "A file to read instead of Text." },
    { "CodeBlock", "Lang", CUI_XML_TEXT, NULL, "Language of the highlighter: c, cpp, js, ts, python..." },
    { "CodeBlock", "Title", CUI_XML_TEXT, NULL, "Shown in the title bar, e.g. a file name." },
    { "CodeBlock", "LineNumbers", CUI_XML_BOOL, NULL, "A column of line numbers." },
    { "CodeBlock", "HighlightFrom", CUI_XML_NUMBER, NULL, "First line of the range to emphasize, 1 based." },
    { "CodeBlock", "HighlightTo", CUI_XML_NUMBER, NULL, "Last line of that range." },
    { "Json", "ExpandDepth", CUI_XML_NUMBER, NULL, "Levels that start open (default 2)." },
    { "LineChart", "Area", CUI_XML_BOOL, NULL, "Fill under the lines." },
    { "LineChart", "Smooth", CUI_XML_BOOL, NULL, "Curved lines." },
    { "LineChart", "Height", CUI_XML_NUMBER, NULL, "Default 220." },
    { "Series", "Values", CUI_XML_NUMBERS, NULL, "The data, comma separated." },
    { "Series", "Color", CUI_XML_COLOR, NULL, "Line or bar color; the series palette when absent." },
    { "Slice", "Value", CUI_XML_NUMBER, NULL, "Weight of the slice." },
    { "Sparkline", "Values", CUI_XML_NUMBERS, NULL, "The data, comma separated." },
    { "Gauge", "Value", CUI_XML_NUMBER, NULL, "0 to 1." },
    { "Gauge", "Size", CUI_XML_NUMBER, NULL, "Diameter in points." },
    { "PatternMeter", "Total", CUI_XML_NUMBER, NULL, "The whole bar; 0 sums the parts." },
    { "PatternMeter", "Unit", CUI_XML_TEXT, NULL, "After every value." },
    { "Part", "Name", CUI_XML_TEXT, NULL, "Name of the part." },
    { "Part", "Value", CUI_XML_NUMBER, NULL, "Size of the part." },
    { "Image", "Source", CUI_XML_FILE, NULL, "PNG or JPEG, looked for next to the document." },
    { "Image", "Radius", CUI_XML_NUMBER, NULL, "Corner radius in points." },
    { "Toolbar", "Style", CUI_XML_CHOICE, "plain|grouped|floating", "How the strip looks." },
    { "ToolbarButton", "Icon", CUI_XML_TEXT, NULL, "The icon of the button." },
    { "ToolbarButton", "Tooltip", CUI_XML_TEXT, NULL, "The name of an icon only button." },
    { "ToolbarButton", "Toggle", CUI_XML_BOOL, NULL, "A toggle button; its state lives in the element." },
    { "StatusBar", "NoBorder", CUI_XML_BOOL, NULL, "No hairline above it." },
    { "StatusText", "Icon", CUI_XML_TEXT, NULL, "Icon before the message." },
    { "StatusProgress", "Value", CUI_XML_NUMBER, NULL, "0 to 1." },
    { "MenuBarMenu", "Width", CUI_XML_NUMBER, NULL, "Width of the menu." },
    { "MenuItem", "Shortcut", CUI_XML_TEXT, NULL, "Shown at the right edge." },
    { "MenuItem", "Danger", CUI_XML_BOOL, NULL, "In the danger color." },
    { "MenuItem", "Checked", CUI_XML_BOOL, NULL, "A check mark at the right edge." },
    { "Menu", "Icon", CUI_XML_TEXT, NULL, "Icon of the button that opens the menu." },
    { "Menu", "IconOnly", CUI_XML_BOOL, NULL, "An icon button; the label is its tooltip." },
    { "Dialog", "Title", CUI_XML_TEXT, NULL, "Title of the dialog." },
    { "Dialog", "Description", CUI_XML_TEXT, NULL, "Text under the title." },
    { "Dialog", "Width", CUI_XML_NUMBER, NULL, "Default 440." },
    { "Dialog", "Persistent", CUI_XML_BOOL, NULL, "A click outside or Esc does not close it." },
    { "Shortcut", "Keys", CUI_XML_CHOICE, "Ctrl+S|Ctrl+Shift+Z|F5|Alt+Enter", "The keys that fire the action." },
    { "Shortcut", "Action", CUI_XML_ACTION, NULL, "The action fired." },
    { "Repeat", "Count", CUI_XML_NUMBER, NULL, "How many times the children repeat." },
    { "Repeat", "Var", CUI_XML_TEXT, NULL, "Name of the index {\"...\"} reads (default i)." },
    { "ForEach", "Var", CUI_XML_TEXT, NULL, "Name of the entry {\"...\"} reads (default item)." },
    { "Include", "Source", CUI_XML_FILE, NULL, "The file whose content is brought in here." },
    { "Widget", "Name", CUI_XML_TEXT, NULL, "The tag this declaration adds." },
    { "Tab", "Icon", CUI_XML_TEXT, NULL, "Icon of the tab." },
    { "Tab", "Dirty", CUI_XML_BOOL, NULL, "Unsaved changes: a dot in place of the x." },
    { "Segmented", "Icons", CUI_XML_LIST, NULL, "An icon before each option: Icon=\"sun|moon\"." },
    { "Tabs", "Icons", CUI_XML_LIST, NULL, "An icon in each header, one per entry." },
    { "Tabs", "FullWidth", CUI_XML_BOOL, NULL, "As wide as the parent." },
    { "Metric", "DeltaLabel", CUI_XML_TEXT, NULL, "Text beside the value instead of the percentage." },
    { "Metric", "LowerIsBetter", CUI_XML_BOOL, NULL, "A fall is good news: the trend shows green." },
    { "LineChart", "Labels", CUI_XML_LIST, NULL, "Labels of the x axis, one per point." },
    { "LineChart", "Format", CUI_XML_TEXT, NULL, "printf format of the values, e.g. \"$%.0fk\"." },
    { "LineChart", "Dither", CUI_XML_BOOL, NULL, "Fill each band with dots that thin out downward." },
    { "LineChart", "Stacked", CUI_XML_BOOL, NULL, "Bands stack in one column." },
    { "BarChart", "Labels", CUI_XML_LIST, NULL, "Labels of the x axis, one per group." },
    { "BarChart", "Format", CUI_XML_TEXT, NULL, "printf format of the values." },
    { "BarChart", "Stacked", CUI_XML_BOOL, NULL, "Bars stack in one column." },
    { "DonutChart", "Format", CUI_XML_TEXT, NULL, "printf format of the shares, e.g. \"%.0f%%\"." },
    { "DonutChart", "Height", CUI_XML_NUMBER, NULL, "Default 220." },
    { "PatternMeter", "Grayscale", CUI_XML_BOOL, NULL, "Every part in one color, told apart by density." },
    { "PatternMeter", "Label", CUI_XML_TEXT, NULL, "Title over the bar." },
    { "PatternMeter", "FreeLabel", CUI_XML_TEXT, NULL, "Legend entry of the free space." },
    { "PatternMeter", "Height", CUI_XML_NUMBER, NULL, "Bar height in points, default 18." },
    { "Gauge", "Label", CUI_XML_TEXT, NULL, "Name under the value." },
    { "ButtonGroup", "Gap", CUI_XML_NUMBER, NULL, "Space between the buttons." },
    { "CopyButton", "Copy", CUI_XML_TEXT, NULL, "The text put on the clipboard; Text when absent." },
    { "HoldButton", "Duration", CUI_XML_NUMBER, NULL, "Seconds to hold before it fires, default 1.2." },
    { "HoldButton", "Icon", CUI_XML_TEXT, NULL, "Leading icon." },
    { "HoldButton", "Variant", CUI_XML_CHOICE, "default|secondary|primary|outline|ghost|danger|link", "How it looks." },
    { "TagInput", "Label", CUI_XML_TEXT, NULL, "Label above the field." },
    { "TagInput", "Placeholder", CUI_XML_TEXT, NULL, "Text shown while it is empty." },
    { "TagInput", "MaxTags", CUI_XML_NUMBER, NULL, "Most tags it holds; 0 means no limit." },
    { "TagInput", "Value", CUI_XML_TEXT, NULL, "The tags it starts with, comma separated." },
    { "Otp", "Length", CUI_XML_NUMBER, NULL, "Number of boxes, default 6." },
    { "Otp", "Value", CUI_XML_TEXT, NULL, "The code it starts with." },
    { "PasswordStrength", "Bind", CUI_XML_TEXT, NULL, "The text whose strength it shows." },
    { "DatePicker", "Label", CUI_XML_TEXT, NULL, "Label above the field." },
    { "DatePicker", "Year", CUI_XML_NUMBER, NULL, "The date it starts with." },
    { "DatePicker", "Month", CUI_XML_NUMBER, NULL, "1 to 12." },
    { "DatePicker", "Day", CUI_XML_NUMBER, NULL, "1 to 31." },
    { "RadioCards", "Columns", CUI_XML_NUMBER, NULL, "Cards per row; 0 picks 3 or 2 by the count." },
    { "Choice", "Description", CUI_XML_TEXT, NULL, "A sentence under the title." },
    { "Choice", "Icon", CUI_XML_TEXT, NULL, "Icon of the card." },
    { "Choice", "Badge", CUI_XML_TEXT, NULL, "A small accent label beside the title, e.g. \"Default\"." },
    { "SkeletonGroup", "Dither", CUI_XML_BOOL, NULL, "Draw the bars in dots." },
    { "SkeletonGroup", "Gap", CUI_XML_NUMBER, NULL, "Space between the bars, default 8." },
    { "StatusSpinner", "Status", CUI_XML_CHOICE, "idle|busy|success|error", "The state of the job it shows." },
    { "StatusSpinner", "Size", CUI_XML_NUMBER, NULL, "Points, default 20." },
    { "StatusSpinner", "Keep", CUI_XML_BOOL, NULL, "The check or cross stays instead of dissolving." },
    { "TimelineItem", "Time", CUI_XML_TEXT, NULL, "Muted text at the right of the title." },
    { "TimelineItem", "Description", CUI_XML_TEXT, NULL, "Text under the title." },
    { "TimelineItem", "Icon", CUI_XML_TEXT, NULL, "Icon in the dot." },
    { "TimelineItem", "Tone", CUI_XML_CHOICE, "neutral|info|success|warning|danger|violet|teal|rose|accent", "Color of the dot." },
    { "TimelineItem", "Last", CUI_XML_BOOL, NULL, "The final row: no connector under its dot." },
    { "ProgressRing", "Value", CUI_XML_NUMBER, NULL, "0 to 1." },
    { "ProgressRing", "Size", CUI_XML_NUMBER, NULL, "Diameter in points, default 104." },
    { "AvatarGroup", "Items", CUI_XML_LIST, NULL, "The people, separated by bars." },
    { "AvatarGroup", "Max", CUI_XML_NUMBER, NULL, "How many are drawn before the count." },
    { "AvatarGroup", "Size", CUI_XML_NUMBER, NULL, "Diameter in points, default 32." },
    { "Popover", "Width", CUI_XML_NUMBER, NULL, "Width of the panel." },
    { "Popover", "Placement", CUI_XML_CHOICE, "bottom|bottomstart|bottomend|top|topstart|topend|right|left", "Where it opens." },
    { "Popover", "Gap", CUI_XML_NUMBER, NULL, "Space between the panel and what it is anchored to." },
    { "Readout", "Label", CUI_XML_TEXT, NULL, "Small text above the value." },
    { "Readout", "Value", CUI_XML_TEXT, NULL, "The big value." },
    { "Readout", "Unit", CUI_XML_TEXT, NULL, "After the value, smaller and muted, e.g. MG." },
    { "Readout", "Detail", CUI_XML_TEXT, NULL, "A muted line under the value." },
    { "Readout", "Size", CUI_XML_CHOICE, "md|sm|lg", "Size of the value." },
    { "Readout", "Tone", CUI_XML_CHOICE, "neutral|info|success|warning|danger|violet|teal|rose|accent", "Color of the value." },
    { "Readout", "Accent", CUI_XML_BOOL, NULL, "The value in the accent color." },
    { "DotDisplay", "Pitch", CUI_XML_NUMBER, NULL, "Distance between dot centers in points, default 4." },
    { "DotDisplay", "Color", CUI_XML_COLOR, NULL, "Lit dots." },
    { "DotDisplay", "Accent", CUI_XML_BOOL, NULL, "Lit dots in the accent color." },
    { "DotDisplay", "NoDim", CUI_XML_BOOL, NULL, "Hides the unlit dots." },
    { "DotDisplay", "Marquee", CUI_XML_BOOL, NULL, "Scrolls text wider than the box instead of shrinking it." },
    { "Dropzone", "Title", CUI_XML_TEXT, NULL, "The line over the hint; its text when absent." },
    { "Dropzone", "Description", CUI_XML_TEXT, NULL, "The hint under the title." },
    { "Dropzone", "Icon", CUI_XML_TEXT, NULL, "Icon in the circle, default an upload arrow." },
    { "Dropzone", "Accept", CUI_XML_TEXT, NULL, "Extensions it takes, e.g. \"png;jpg;pdf\"." },
    { "Dropzone", "Height", CUI_XML_NUMBER, NULL, "Height in points, default 168." },
    { "Dropzone", "Compact", CUI_XML_BOOL, NULL, "One row instead of a tall area." },
    { "SignaturePad", "Height", CUI_XML_NUMBER, NULL, "Height in points, default 180." },
    { "SignaturePad", "Placeholder", CUI_XML_TEXT, NULL, "Faint text shown while the pad is empty." },
    { "AvatarUpload", "Size", CUI_XML_NUMBER, NULL, "Diameter in points, default 88." },
    { "Toc", "Scroll", CUI_XML_TEXT, NULL, "The Name of the Scroll it follows." },
    { "TocItem", "Anchor", CUI_XML_TEXT, NULL, "The TocAnchor it scrolls to; its text when absent." },
    { "TocItem", "Level", CUI_XML_NUMBER, NULL, "Indentation, 0 for a top level section." },
    { "TocAnchor", "Anchor", CUI_XML_TEXT, NULL, "The name its TocItem gives; its text when absent." },
    { "InlineEdit", "Font", CUI_XML_CHOICE, "body|medium|display|mono", "Font of the text and of the field." },
    { "InlineEdit", "Size", CUI_XML_NUMBER, NULL, "Font size in points." },
    { "InlineEdit", "Placeholder", CUI_XML_TEXT, NULL, "Shown while the text is empty." },
    { "Scrub", "Min", CUI_XML_NUMBER, NULL, "Lowest value." },
    { "Scrub", "Max", CUI_XML_NUMBER, NULL, "Highest value." },
    { "Scrub", "Value", CUI_XML_NUMBER, NULL, "Starting value (the first frame)." },
    { "Scrub", "Step", CUI_XML_NUMBER, NULL, "What a drag or an arrow adds; 0 spreads the range over 100 steps." },
    { "Scrub", "Format", CUI_XML_TEXT, NULL, "printf format of the value, default \"%.0f\"." },
    { "Scrub", "Suffix", CUI_XML_TEXT, NULL, "After the value, e.g. \"px\" or \"%\"." },
    { "MoneyInput", "Label", CUI_XML_TEXT, NULL, "Label above the field." },
    { "MoneyInput", "Value", CUI_XML_NUMBER, NULL, "Starting amount (the first frame)." },
    { "MoneyInput", "Symbol", CUI_XML_TEXT, NULL, "Before the amount, default \"$\"." },
    { "MoneyInput", "Code", CUI_XML_TEXT, NULL, "Currency code after the amount, e.g. USD." },
    { "MoneyInput", "Decimals", CUI_XML_NUMBER, NULL, "Digits kept; -1 drops them when the amount is whole." },
    { "MoneyInput", "Hint", CUI_XML_TEXT, NULL, "Helper text below the field." },
    { "ShortcutRecorder", "Shortcut", CUI_XML_TEXT, NULL, "The combination it starts with, e.g. \"Ctrl+Shift+P\"." },
    { "ShortcutRecorder", "Placeholder", CUI_XML_TEXT, NULL, "Shown while no keys are recorded." },
    { "EmojiPicker", "Height", CUI_XML_NUMBER, NULL, "Height of the grid, default 280." },
    { "EmojiPicker", "Size", CUI_XML_NUMBER, NULL, "Emoji size in points, default 26." },
    { "EmojiPicker", "Tone", CUI_XML_NUMBER, NULL, "Skin tone it starts on, 0 to 5." },
    { "EmojiPicker", "NoSearch", CUI_XML_BOOL, NULL, "Hides the search field." },
    { "DotChart", "Height", CUI_XML_NUMBER, NULL, "The plot and its labels, default 200." },
    { "DotChart", "XMin", CUI_XML_NUMBER, NULL, "What the first sample stands for." },
    { "DotChart", "XMax", CUI_XML_NUMBER, NULL, "What the last sample stands for." },
    { "DotChart", "XUnits", CUI_XML_CHOICE, "number|hours", "How x labels are written; hours shows a time of day." },
    { "DotChart", "XStep", CUI_XML_NUMBER, NULL, "Spacing of the x labels; 0 picks one that fits." },
    { "DotChart", "Min", CUI_XML_NUMBER, NULL, "Lowest value of the y range." },
    { "DotChart", "Max", CUI_XML_NUMBER, NULL, "Highest value of the y range." },
    { "DotChart", "Unit", CUI_XML_TEXT, NULL, "After values in the readout, e.g. MG." },
    { "DotChart", "Pitch", CUI_XML_NUMBER, NULL, "Distance between dot centers, default 6." },
    { "DotChart", "NoAxis", CUI_XML_BOOL, NULL, "Hides the x labels, e.g. when an EventTrack under it has them." },
    { "Series", "Accent", CUI_XML_BOOL, NULL, "Draws this series in the accent color, the one selected." },
    { "Annotation", "Axis", CUI_XML_CHOICE, "x|y|point", "A vertical line, a horizontal one, or a dot." },
    { "Annotation", "At", CUI_XML_NUMBER, NULL, "Where the line sits on x." },
    { "Annotation", "Value", CUI_XML_NUMBER, NULL, "Where the line sits on y." },
    { "Annotation", "Detail", CUI_XML_TEXT, NULL, "A muted second line under the label." },
    { "EventTrack", "Values", CUI_XML_NUMBERS, NULL, "The moments it starts with, comma separated." },
    { "EventTrack", "Min", CUI_XML_NUMBER, NULL, "Start of the axis." },
    { "EventTrack", "Max", CUI_XML_NUMBER, NULL, "End of the axis." },
    { "EventTrack", "Step", CUI_XML_NUMBER, NULL, "What events snap to; 0 does not snap." },
    { "EventTrack", "Units", CUI_XML_CHOICE, "number|hours", "How the axis labels are written." },
    { "EventTrack", "LabelStep", CUI_XML_NUMBER, NULL, "Spacing of the axis labels." },
    { "EventTrack", "Selected", CUI_XML_NUMBER, NULL, "The event selected first, or -1." },
    { "EventTrack", "NoAdd", CUI_XML_BOOL, NULL, "Clicks and Enter do not add events." },
    { "EventTrack", "NoRemove", CUI_XML_BOOL, NULL, "Double-clicks and Delete do not remove events." },
    { "HoverCard", "Width", CUI_XML_NUMBER, NULL, "Width of the panel in points." },
    { "HoverCard", "Placement", CUI_XML_CHOICE, "bottomstart|bottomend|topstart|topend|leftstart|rightstart|left|right", "Where the panel opens." },
    { "HoverCard", "Delay", CUI_XML_NUMBER, NULL, "Seconds the pointer rests before it shows, default 0.45." },
    { "ConfirmPopover", "Title", CUI_XML_TEXT, NULL, "The question." },
    { "ConfirmPopover", "Description", CUI_XML_TEXT, NULL, "A line under it." },
    { "ConfirmPopover", "ConfirmLabel", CUI_XML_TEXT, NULL, "Text of the confirm button, default Confirm." },
    { "ConfirmPopover", "CancelLabel", CUI_XML_TEXT, NULL, "Text of the cancel button." },
    { "ConfirmPopover", "Danger", CUI_XML_BOOL, NULL, "The confirming button in the danger color." },
    { "ExpandableText", "Lines", CUI_XML_NUMBER, NULL, "Lines shown while collapsed, default 3." },
    { "ExpandableText", "Size", CUI_XML_NUMBER, NULL, "Font size in points." },
    { "ExpandableCard", "Open", CUI_XML_BOOL, NULL, "Whether it starts open." },
    { "ExpandableCard", "Description", CUI_XML_TEXT, NULL, "Second line under the title." },
    { "ExpandableCard", "Icon", CUI_XML_TEXT, NULL, "Icon beside the title." },
    { "ExpandableCard", "Badge", CUI_XML_TEXT, NULL, "A badge at the end of the header." },
    { "ExpandableCard", "BadgeTone", CUI_XML_CHOICE, "neutral|info|success|warning|danger|violet|teal|rose|accent", "Color of that badge." },
    { "Countdown", "Seconds", CUI_XML_NUMBER, NULL, "What is left when the element is first drawn." },
    { "Countdown", "HideDays", CUI_XML_BOOL, NULL, "No days tile; hours count the days in." },
    { "Countdown", "Compact", CUI_XML_BOOL, NULL, "Small tiles joined by colons." },
    { "Checklist", "Title", CUI_XML_TEXT, NULL, "Heading of the card, default Get started." },
    { "Checklist", "Description", CUI_XML_TEXT, NULL, "A line under the heading." },
    { "Step", "Description", CUI_XML_TEXT, NULL, "What the step is about." },
    { "Step", "Action", CUI_XML_TEXT, NULL, "Button under the first step not done." },
    { "Step", "Done", CUI_XML_BOOL, NULL, "Whether the step starts complete." },
    { "Veil", "Revealed", CUI_XML_BOOL, NULL, "Whether it starts uncovered." },
    { "Veil", "Label", CUI_XML_TEXT, NULL, "A line in the middle of the veil." },
    { "Veil", "Hint", CUI_XML_TEXT, NULL, "A smaller line under it." },
    { "Veil", "Toggle", CUI_XML_BOOL, NULL, "A click on the shown content covers it again." },
    { "Veil", "Radius", CUI_XML_NUMBER, NULL, "Corners of the veil in points." },
    { "SplitButton", "Items", CUI_XML_TEXT, NULL, "The menu's entries, separated by |." },
    { "SplitButton", "Icons", CUI_XML_TEXT, NULL, "One icon per entry." },
    { "SplitButton", "Variant", CUI_XML_CHOICE, "default|secondary|primary|outline|ghost|danger|link", "How the main button looks." },
    { "ToggleGroup", "Items", CUI_XML_TEXT, NULL, "The toggles, separated by |." },
    { "ToggleGroup", "Icons", CUI_XML_TEXT, NULL, "One icon per toggle." },
    { "ToggleGroup", "On", CUI_XML_TEXT, NULL, "Indices that start on, comma separated." },
    { "ToggleGroup", "IconOnly", CUI_XML_BOOL, NULL, "Only the icons, with the labels as tooltips." },
    { "ToggleGroup", "Exclusive", CUI_XML_BOOL, NULL, "One on at a time, like radio buttons." },
    { "ConfirmButton", "ConfirmLabel", CUI_XML_TEXT, NULL, "What the armed button says." },
    { "ConfirmButton", "Timeout", CUI_XML_NUMBER, NULL, "Seconds before it disarms, default 3." },
    { "StatusButton", "Status", CUI_XML_CHOICE, "idle|busy|success|error", "What the button shows." },
    { "StatusButton", "SuccessLabel", CUI_XML_TEXT, NULL, "Text when the work is done, default Done." },
    { "StatusButton", "BusyLabel", CUI_XML_TEXT, NULL, "Text while it runs." },
    { "StatusButton", "ErrorLabel", CUI_XML_TEXT, NULL, "Text when it failed." },
    { "Reactions", "Items", CUI_XML_TEXT, NULL, "icon:count[:on] entries, separated by |." },
    { "ThemeSwitcher", "Mode", CUI_XML_NUMBER, NULL, "0 light, 1 dark, 2 system; -1 follows the window." },
    { "ThemeSwitcher", "System", CUI_XML_BOOL, NULL, "Offer the system setting as a third choice." },
    { "UserMenu", "Detail", CUI_XML_TEXT, NULL, "Second line under the name." },
    { "UserMenu", "Status", CUI_XML_TEXT, NULL, "Dot on the avatar: online, busy, away." },
    { "UserMenu", "Compact", CUI_XML_BOOL, NULL, "Only the avatar as the button." },
    { "SwipeItem", "Left", CUI_XML_TEXT, NULL, "Actions on a right swipe: icon:label:tone entries." },
    { "SwipeItem", "Right", CUI_XML_TEXT, NULL, "Actions on a left swipe, same shape." },
    { "SpeedDial", "Items", CUI_XML_TEXT, NULL, "icon:label entries, separated by |." },
    { "SpeedDial", "Tooltip", CUI_XML_TEXT, NULL, "Tooltip of the round button." },
    { "MultiSelect", "Items", CUI_XML_TEXT, NULL, "The items, separated by |." },
    { "MultiSelect", "On", CUI_XML_TEXT, NULL, "Indices chosen at first, comma separated." },
    { "MultiSelect", "Label", CUI_XML_TEXT, NULL, "Label above the field." },
    { "MultiSelect", "Placeholder", CUI_XML_TEXT, NULL, "Shown while nothing is chosen." },
    { "MultiSelect", "Searchable", CUI_XML_BOOL, NULL, "A search field at the top of the list." },
    { "MultiSelect", "MaxChips", CUI_XML_NUMBER, NULL, "Chips before \"+N\", default 3." },
    { "DateRangePicker", "From", CUI_XML_TEXT, NULL, "Start of the range, 2026-11-03." },
    { "DateRangePicker", "To", CUI_XML_TEXT, NULL, "End of the range." },
    { "DateRangePicker", "NoPresets", CUI_XML_BOOL, NULL, "Hides the list of preset ranges." },
    { "RangeCalendar", "From", CUI_XML_TEXT, NULL, "Start of the range." },
    { "RangeCalendar", "To", CUI_XML_TEXT, NULL, "End of the range." },
    { "RangeCalendar", "Months", CUI_XML_NUMBER, NULL, "Months side by side, default 2." },
    { "TimePicker", "Hour", CUI_XML_NUMBER, NULL, "Starting hour, 0 to 23." },
    { "TimePicker", "Minute", CUI_XML_NUMBER, NULL, "Starting minute." },
    { "TimePicker", "TwelveHour", CUI_XML_BOOL, NULL, "1 to 12 with AM and PM." },
    { "TimePicker", "MinuteStep", CUI_XML_NUMBER, NULL, "Minutes between entries, default 5." },
    { "TimeDial", "Hour", CUI_XML_NUMBER, NULL, "Starting hour." },
    { "TimeDial", "Minute", CUI_XML_NUMBER, NULL, "Starting minute." },
    { "TimeDial", "Size", CUI_XML_NUMBER, NULL, "Diameter in points." },
    { "WheelPicker", "Items", CUI_XML_TEXT, NULL, "The options, separated by |." },
    { "WheelPicker", "Value", CUI_XML_NUMBER, NULL, "Starting index." },
    { "WheelPicker", "Rows", CUI_XML_NUMBER, NULL, "Rows in view, default 5." },
    { "WheelPicker", "Loop", CUI_XML_BOOL, NULL, "The options wrap around." },
    { "WheelPicker", "JoinLeft", CUI_XML_BOOL, NULL, "Square the left end of the selection band." },
    { "WheelPicker", "JoinRight", CUI_XML_BOOL, NULL, "Square the right end of it." },
    { "RulerSlider", "Min", CUI_XML_NUMBER, NULL, "Lowest value." },
    { "RulerSlider", "Max", CUI_XML_NUMBER, NULL, "Highest value." },
    { "RulerSlider", "Value", CUI_XML_NUMBER, NULL, "Starting value." },
    { "RulerSlider", "Step", CUI_XML_NUMBER, NULL, "Distance between ticks." },
    { "RulerSlider", "MajorEvery", CUI_XML_NUMBER, NULL, "Every Nth tick is labelled, default 10." },
    { "RulerSlider", "Spacing", CUI_XML_NUMBER, NULL, "Points between ticks, default 9." },
    { "RulerSlider", "Unit", CUI_XML_TEXT, NULL, "After the value, e.g. cm." },
    { "HistogramSlider", "Values", CUI_XML_NUMBERS, NULL, "The data behind the histogram." },
    { "HistogramSlider", "Min", CUI_XML_NUMBER, NULL, "Start of the scale." },
    { "HistogramSlider", "Max", CUI_XML_NUMBER, NULL, "End of the scale." },
    { "HistogramSlider", "Low", CUI_XML_NUMBER, NULL, "Starting low value." },
    { "HistogramSlider", "High", CUI_XML_NUMBER, NULL, "Starting high value." },
    { "ChatMessage", "Mine", CUI_XML_BOOL, NULL, "Your own message: right aligned, in the accent bubble." },
    { "ChatMessage", "Author", CUI_XML_TEXT, NULL, "Name above the first message of a group." },
    { "ChatMessage", "Time", CUI_XML_TEXT, NULL, "Shown after the author's name." },
    { "ChatMessage", "Status", CUI_XML_TEXT, NULL, "Under your own last message, e.g. Read." },
    { "ChatMessage", "Grouped", CUI_XML_BOOL, NULL, "Continues the previous message." },
    { "ChatMessage", "LastInGroup", CUI_XML_BOOL, NULL, "Shows the avatar or the status." },
    { "ChatMessage", "Markdown", CUI_XML_BOOL, NULL, "Draw the text as markdown." },
    { "Reasoning", "Thinking", CUI_XML_BOOL, NULL, "Still thinking: the header shimmers and the body stays open." },
    { "Reasoning", "Seconds", CUI_XML_NUMBER, NULL, "Time spent, for the header." },
    { "Reasoning", "Title", CUI_XML_TEXT, NULL, "Header text instead of Thinking / Thought for." },
    { "ToolCall", "Status", CUI_XML_CHOICE, "pending|running|done|failed|skipped", "What became of the call." },
    { "ToolCall", "Icon", CUI_XML_TEXT, NULL, "Icon before the tool's name." },
    { "ToolCall", "Detail", CUI_XML_TEXT, NULL, "Muted text after the name, e.g. the file or query." },
    { "ToolCall", "Input", CUI_XML_TEXT, NULL, "What was sent; the row opens to show it." },
    { "ToolCall", "Output", CUI_XML_TEXT, NULL, "What came back." },
    { "ToolCall", "Seconds", CUI_XML_NUMBER, NULL, "How long it took." },
    { "AgentStatus", "Status", CUI_XML_CHOICE, "pending|running|done|failed|skipped", "State of the work." },
    { "AgentStatus", "Seconds", CUI_XML_NUMBER, NULL, "Elapsed time after the text." },
    { "Typewriter", "Size", CUI_XML_NUMBER, NULL, "Font size in points." },
    { "Typewriter", "Replay", CUI_XML_NUMBER, NULL, "Change the number to restart the animation." },
    { "TextReveal", "Replay", CUI_XML_NUMBER, NULL, "Change the number to restart the animation." },
    { "Kanban", "AutoMove", CUI_XML_BOOL, NULL, "Keep cards where they were dropped." },
    { "ContextMenu", "Target", CUI_XML_TEXT, NULL, "Name of the element that opens the menu." },
    { "Citation", "Number", CUI_XML_NUMBER, NULL, "The number shown in the pill." },
    { "Citation", "From", CUI_XML_TEXT, NULL, "Name of the SourceList it points into." },
    { "Citation", "Index", CUI_XML_NUMBER, NULL, "Which entry of that list, 0 based." },
    { "Source", "Title", CUI_XML_TEXT, NULL, "Heading of the card." },
    { "Source", "URL", CUI_XML_TEXT, NULL, "Address; its domain is shown." },
    { "Source", "Snippet", CUI_XML_TEXT, NULL, "Excerpt in the citation's hover card." },
    { "ResponseActions", "CopyText", CUI_XML_TEXT, NULL, "What the copy button copies." },
    { "ResponseActions", "NoRegenerate", CUI_XML_BOOL, NULL, "Hides the regenerate button." },
    { "ResponseActions", "NoShare", CUI_XML_BOOL, NULL, "Hides the share button." },
    { "ContextMeter", "Used", CUI_XML_NUMBER, NULL, "How much of the context is used." },
    { "ContextMeter", "Limit", CUI_XML_NUMBER, NULL, "How much there is." },
    { "ContextMeter", "Unit", CUI_XML_TEXT, NULL, "After the amounts." },
    { "ChatComposer", "Placeholder", CUI_XML_TEXT, NULL, "Hint in the empty field." },
    { "ChatComposer", "Busy", CUI_XML_BOOL, NULL, "A reply is being written: send becomes stop." },
    { "ChatComposer", "Attachments", CUI_XML_TEXT, NULL, "File names shown as chips, separated by |." },
    { "ChatComposer", "Models", CUI_XML_TEXT, NULL, "name:detail:badge:icon entries for the model picker." },
    { "ChatComposer", "Model", CUI_XML_NUMBER, NULL, "Index of the model chosen." },
    { "AgentPlan", "Title", CUI_XML_TEXT, NULL, "Heading of the plan, default Plan." },
    { "PlanStep", "Detail", CUI_XML_TEXT, NULL, "Smaller line under the step." },
    { "PlanStep", "Status", CUI_XML_CHOICE, "pending|running|done|failed|skipped", "State of the step." },
    { "PromptSuggestions", "Columns", CUI_XML_NUMBER, NULL, "Cards per row, default 2." },
    { "Suggestion", "Icon", CUI_XML_TEXT, NULL, "Icon on the card." },
    { "Suggestion", "Detail", CUI_XML_TEXT, NULL, "Muted line under the title." },
    { "Treemap", "Height", CUI_XML_NUMBER, NULL, "Height in points, default 300." },
    { "Treemap", "Format", CUI_XML_TEXT, NULL, "The values, e.g. \"%.0f KB\"." },
    { "Funnel", "Format", CUI_XML_TEXT, NULL, "The values and their share." },
    { "SplitBar", "Format", CUI_XML_TEXT, NULL, "The values in the legend, e.g. \"%.0f ms\"." },
    { "SplitBar", "NoLegend", CUI_XML_BOOL, NULL, "Leaves the legend out." },
    { "Waffle", "Cell", CUI_XML_NUMBER, NULL, "Size of a square, default 18." },
    { "Waffle", "Columns", CUI_XML_NUMBER, NULL, "Squares per row, default 10." },
    { "Waffle", "Rows", CUI_XML_NUMBER, NULL, "Rows of squares, default 10." },
    { "BarList", "Format", CUI_XML_TEXT, NULL, "The values beside the bars." },
    { "BarList", "MaxRows", CUI_XML_NUMBER, NULL, "Shows only this many rows; 0 shows all." },
    { "BarList", "KeepOrder", CUI_XML_BOOL, NULL, "Keeps the order given instead of sorting." },
    { "ActivityRings", "Size", CUI_XML_NUMBER, NULL, "Diameter in points." },
    { "ActivityRings", "Center", CUI_XML_TEXT, NULL, "Large text in the middle." },
    { "ActivityRings", "Caption", CUI_XML_TEXT, NULL, "Small text under that." },
    { "Heatmap", "Values", CUI_XML_NUMBERS, NULL, "The cells, row by row." },
    { "Heatmap", "Rows", CUI_XML_NUMBER, NULL, "Number of rows." },
    { "Heatmap", "Cols", CUI_XML_NUMBER, NULL, "Number of columns." },
    { "Heatmap", "RowLabels", CUI_XML_TEXT, NULL, "One label per row, separated by |." },
    { "Heatmap", "ColLabels", CUI_XML_TEXT, NULL, "Column labels, separated by |." },
    { "Heatmap", "ColLabelStep", CUI_XML_NUMBER, NULL, "Writes a label every Nth column." },
    { "Heatmap", "Format", CUI_XML_TEXT, NULL, "The value in the readout, e.g. \"%.0f req\"." },
    { "CalendarHeatmap", "Values", CUI_XML_NUMBERS, NULL, "One value a day." },
    { "CalendarHeatmap", "Days", CUI_XML_NUMBER, NULL, "How many days, ending today unless Start says." },
    { "CalendarHeatmap", "Start", CUI_XML_TEXT, NULL, "First day, 2025-11-20; default a year back." },
    { "CalendarHeatmap", "Unit", CUI_XML_TEXT, NULL, "What a day counts, e.g. contributions." },
    { "RadialGauge", "Value", CUI_XML_NUMBER, NULL, "The value." },
    { "RadialGauge", "Min", CUI_XML_NUMBER, NULL, "Start of the scale." },
    { "RadialGauge", "Max", CUI_XML_NUMBER, NULL, "End of the scale." },
    { "RadialGauge", "Label", CUI_XML_TEXT, NULL, "Under the value." },
    { "RadialGauge", "Format", CUI_XML_TEXT, NULL, "The value and the ends." },
    { "RadialGauge", "Unit", CUI_XML_TEXT, NULL, "After the value." },
    { "RadialGauge", "Detail", CUI_XML_TEXT, NULL, "A muted line under the gauge." },
    { "RadialGauge", "Style", CUI_XML_CHOICE, "auto|arc|segments|ring", "How the value is drawn." },
    { "RadialGauge", "Bands", CUI_XML_TEXT, NULL, "Colored stretches: \"60:success:Normal|85:warning:Busy\"." },
    { "RadialGauge", "Target", CUI_XML_NUMBER, NULL, "A marker on the arc." },
    { "RadialGauge", "ShowTarget", CUI_XML_BOOL, NULL, "Draws that marker." },
    { "Radar", "Axes", CUI_XML_TEXT, NULL, "The axis names, separated by |." },
    { "Radar", "Size", CUI_XML_NUMBER, NULL, "Diameter in points." },
    { "Radar", "Max", CUI_XML_NUMBER, NULL, "Value at the outer ring." },
    { "Series", "Points", CUI_XML_TEXT, NULL, "Scatter: \"x,y;x,y\" pairs." },
    { "Series", "Sizes", CUI_XML_NUMBERS, NULL, "Scatter: one bubble size per point." },
    { "Scatter", "Height", CUI_XML_NUMBER, NULL, "Height in points." },
    { "Scatter", "XLabel", CUI_XML_TEXT, NULL, "Name of the x axis." },
    { "Scatter", "YLabel", CUI_XML_TEXT, NULL, "Name of the y axis." },
    { "Sankey", "Nodes", CUI_XML_TEXT, NULL, "The node names, separated by |." },
    { "Sankey", "Links", CUI_XML_TEXT, NULL, "Flows as \"from-to:value\", separated by |." },
    { "Sankey", "Height", CUI_XML_NUMBER, NULL, "Height in points." },
    { "Knob", "Min", CUI_XML_NUMBER, NULL, "Lowest value." },
    { "Knob", "Max", CUI_XML_NUMBER, NULL, "Highest value." },
    { "Knob", "Value", CUI_XML_NUMBER, NULL, "Starting value." },
    { "Knob", "Unit", CUI_XML_TEXT, NULL, "After the value, e.g. Hz." },
    { "Knob", "Format", CUI_XML_TEXT, NULL, "printf format of the value." },
    { "Knob", "Size", CUI_XML_NUMBER, NULL, "Diameter in points, default 48." },
    { "Knob", "Reset", CUI_XML_NUMBER, NULL, "Value a double-click goes back to." },
    { "Knob", "LogScale", CUI_XML_BOOL, NULL, "Equal turns multiply the value: frequency, time." },
    { "Knob", "Bipolar", CUI_XML_BOOL, NULL, "The arc grows from the middle: pan, gain." },
    { "Fader", "Min", CUI_XML_NUMBER, NULL, "Lowest value." },
    { "Fader", "Max", CUI_XML_NUMBER, NULL, "Highest value." },
    { "Fader", "Value", CUI_XML_NUMBER, NULL, "Starting value." },
    { "Fader", "Length", CUI_XML_NUMBER, NULL, "Track length, default 160." },
    { "Fader", "Ticks", CUI_XML_NUMBERS, NULL, "Values to mark beside the track." },
    { "Fader", "Meter", CUI_XML_NUMBERS, NULL, "Linear sample peaks for the meter beside it." },
    { "Fader", "Horizontal", CUI_XML_BOOL, NULL, "The track runs left to right." },
    { "LevelMeter", "Levels", CUI_XML_NUMBERS, NULL, "One linear peak per channel, read every frame." },
    { "LevelMeter", "Length", CUI_XML_NUMBER, NULL, "Bar length, default 160." },
    { "LevelMeter", "Horizontal", CUI_XML_BOOL, NULL, "Bars run left to right." },
    { "LevelMeter", "Scale", CUI_XML_BOOL, NULL, "dB labels beside the bars." },
    { "LevelMeter", "Segmented", CUI_XML_BOOL, NULL, "LED segments instead of a solid bar." },
    { "Tour", "Step", CUI_XML_NUMBER, NULL, "The step it is on; -1 keeps it closed." },
    { "Tour", "Width", CUI_XML_NUMBER, NULL, "Width of the step card, default 300." },
    { "TourStep", "Target", CUI_XML_TEXT, NULL, "Name of the element to point at." },
    { "TourStep", "Description", CUI_XML_TEXT, NULL, "The explanation under the title." },
    { "TourStep", "Placement", CUI_XML_CHOICE, "bottomstart|bottomend|topstart|topend|leftstart|rightstart|left|right", "Side of the target the card opens on." }
};

static bool cui__xml_tag_matches(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (cui__ascii_lower((unsigned char)*a) != cui__ascii_lower((unsigned char)*b)) return false;
        a++;
        b++;
    }
    return *a == *b;
}

int cui_xml_attribute_count(const char *tag) {
    int i, n = 0;
    for (i = 0; i < CUI_COUNTOF(cui__xml_attr_rows); i++) {
        const cui__xml_attr_row *row = &cui__xml_attr_rows[i];
        if (!row->tag || (tag && cui__xml_tag_matches(tag, row->tag))) n++;
    }
    return n;
}

bool cui_xml_attribute_info(const char *tag, int index, cui_xml_attribute_desc *info) {
    int i, n = 0;
    if (!info || index < 0) return false;
    for (i = 0; i < CUI_COUNTOF(cui__xml_attr_rows); i++) {
        const cui__xml_attr_row *row = &cui__xml_attr_rows[i];
        if (row->tag && !(tag && cui__xml_tag_matches(tag, row->tag))) continue;
        if (n == index) {
            info->name = row->name;
            info->kind = row->kind;
            info->choices = row->choices;
            info->group = row->tag ? row->tag : "Common";
            info->description = row->description;
            return true;
        }
        n++;
    }
    return false;
}

/* ------------------------------------------------------------------------ */
/* Small readers                                                            */
/* ------------------------------------------------------------------------ */

static const char *cui__xml_label(cui_xml *doc, cui_xml_node *n) {
    const char *text = cui__xml_attr_text(doc, n, "text", NULL);
    if (text) return text;
    return cui__xml_node_text_computed(doc, n);
}

/* An icon written by name (Icon="audio-lines"), or as a glyph. A name that is
 * not one of the 168 is used as written: a document may pass its own. */
static const char *cui__xml_icon_glyph(const char *name) {
    int i;
    if (!name || !*name) return name;
    for (i = 0; i < CUI_COUNTOF(cui__xml_icons); i++) {
        const char *a = name;
        const char *b = cui__xml_icons[i].name;
        while (*a && *b && cui__ascii_lower((unsigned char)*a) == cui__ascii_lower((unsigned char)*b)) {
            a++;
            b++;
        }
        if (!*a && !*b) return cui__xml_icons[i].glyph;
    }
    return name;
}

static const char *cui__xml_icon_named(cui_xml *doc, cui_xml_node *n, const char *attr) {
    const char *name = cui__xml_attr_text(doc, n, attr, NULL);
    return name ? cui__xml_icon_glyph(name) : NULL;
}

static const char *cui__xml_icon(cui_xml *doc, cui_xml_node *n, const char *fallback) {
    const char *name = cui__xml_attr_text(doc, n, "icon", NULL);
    if (!name || !*name) return fallback;
    return cui__xml_icon_glyph(name);
}

static int cui__xml_choice_of(cui_xml *doc, cui_xml_node *n, const char *attr,
                              const char *const *names, int count, int fallback) {
    return cui__xml_attr_choice(doc, n, attr, names, count, fallback);
}

static cui_font cui__xml_font_of(cui_xml *doc, cui_xml_node *n, int fallback) {
    static const char *const names[] = { "body", "medium", "display", "mono" };
    return (cui_font)cui__xml_choice_of(doc, n, "font", names, 4, fallback);
}

static cui_tone cui__xml_tone_attr(cui_xml *doc, cui_xml_node *n, const char *attr, cui_tone fallback) {
    static const char *const names[] = { "neutral", "info", "success", "warning", "danger", "violet", "teal", "rose", "accent" };
    return (cui_tone)cui__xml_choice_of(doc, n, attr, names, 9, (int)fallback);
}

static cui_tone cui__xml_tone_of(cui_xml *doc, cui_xml_node *n, cui_tone fallback) {
    return cui__xml_tone_attr(doc, n, "tone", fallback);
}

static cui_control_size cui__xml_control_size_of(cui_xml *doc, cui_xml_node *n, cui_control_size fallback) {
    static const char *const names[] = { "md", "sm", "lg" };
    return (cui_control_size)cui__xml_choice_of(doc, n, "size", names, 3, (int)fallback);
}

static cui_align cui__xml_align_of(cui_xml *doc, cui_xml_node *n, cui_align fallback) {
    static const char *const names[] = { "default", "start", "center", "end", "stretch" };
    return (cui_align)cui__xml_choice_of(doc, n, "align", names, 5, (int)fallback);
}

static cui_justify cui__xml_justify_of(cui_xml *doc, cui_xml_node *n, cui_justify fallback) {
    static const char *const names[] = { "start", "center", "end", "between", "around" };
    return (cui_justify)cui__xml_choice_of(doc, n, "justify", names, 5, (int)fallback);
}

static cui_text_align cui__xml_text_align_of(cui_xml *doc, cui_xml_node *n, cui_text_align fallback) {
    static const char *const names[] = { "start", "center", "right", "left", "end" };
    return (cui_text_align)cui__xml_choice_of(doc, n, "align", names, 5, (int)fallback);
}

static cui_button_variant cui__xml_variant_of(cui_xml *doc, cui_xml_node *n, cui_button_variant fallback) {
    static const char *const names[] = { "default", "secondary", "primary", "outline", "ghost", "danger", "link" };
    return (cui_button_variant)cui__xml_choice_of(doc, n, "variant", names, 7, (int)fallback);
}

static cui_placement cui__xml_placement_of(cui_xml *doc, cui_xml_node *n, cui_placement fallback) {
    static const char *const names[] = { "bottom", "bottomstart", "bottomend", "top", "topstart", "topend",
                                         "rightstart", "leftstart", "right", "left" };
    return (cui_placement)cui__xml_choice_of(doc, n, "placement", names, 10, (int)fallback);
}

static cui_spinner_style cui__xml_spinner_style_of(cui_xml *doc, cui_xml_node *n, cui_spinner_style fallback) {
    static const char *const names[] = { "arc", "comet", "dotmatrix", "filldrain", "radar", "ripple", "upload",
                                         "download", "transfer", "orbit", "bounce", "scan", "pulse", "spiral", "plasma" };
    return (cui_spinner_style)cui__xml_choice_of(doc, n, "style", names, 15, (int)fallback);
}

static cui_elevation cui__xml_elevation_of(cui_xml *doc, cui_xml_node *n, cui_elevation fallback) {
    static const char *const names[] = { "none", "resting", "raised", "floating" };
    return (cui_elevation)cui__xml_choice_of(doc, n, "elevation", names, 4, (int)fallback);
}

/* Fill="1" is a share of the free space (two elements with Fill="1" split it),
 * Fill="fill" is all of it, Fill="50%" half of the parent. */
static cui_size cui__xml_fill_size(cui_xml *doc, cui_xml_node *n, const char *attr) {
    const char *value = cui__xml_attr_text(doc, n, attr, NULL);
    if (!value || !*value) return CUI_FILL;
    if (cui__xml_tag_matches(value, "fill") || cui__xml_tag_matches(value, "grow")) return CUI_FILL;
    if (strchr(value, '%')) return CUI_PCT((float)atof(value));
    return CUI_GROW((float)atof(value));
}

/* Width, Height, Fill and FillV, in that order of precedence. */
static void cui__xml_sizes(cui_xml *doc, cui_xml_node *n, cui_size *w, cui_size *h) {
    *w = cui__xml_attr_size(doc, n, "width", CUI_FIT);
    *h = cui__xml_attr_size(doc, n, "height", CUI_FIT);
    if (cui__xml_attr_raw(n, "fill")) *w = cui__xml_fill_size(doc, n, "fill");
    if (cui__xml_attr_raw(n, "fillv")) *h = cui__xml_fill_size(doc, n, "fillv");
}

/* Children of a table: the <TableColumn>s, the <TableRow>s. */

/* A file named by Source, read once and kept in the element. */
typedef struct cui__xml_file_cache {
    char *text;
} cui__xml_file_cache;

static void cui__xml_free_cache(void *p) { free(p); }

/* A list written in one attribute (Icons="sun|moon", Labels="Jan|Feb"), read
 * once and kept in the element. Defined with the other drawing helpers. */
static const char **cui__xml_attr_list(cui_xml *doc, cui_xml_node *n, const char *attr, int *count, bool icons);

static const char *cui__xml_source_text(cui_xml *doc, cui_xml_node *n) {
    const char *source = cui__xml_attr_raw(n, "source");
    cui__xml_file_cache *cache = n->cache;
    const char *path;
    if (!source || !*source) return NULL;
    if (!cache) {
        char *text = NULL;
        path = cui_xml_resolve_path(doc, source);
        if (path && *path) text = cui__xml_read_file(path, NULL);
        cache = cui__xml_alloc(sizeof *cache);
        if (!cache) {
            free(text);
            return NULL;
        }
        cache->text = text;
        n->cache = cache;
        n->cache_free = cui__xml_free_cache;
    }
    return cache->text;
}

/* ---- keys of a Shortcut ---- */

static int cui__xml_key_named(const char *name) {
    static const struct {
        const char *name;
        int key;
    } keys[] = {
        { "space", CUI_KEY_SPACE }, { "escape", CUI_KEY_ESCAPE }, { "esc", CUI_KEY_ESCAPE },
        { "enter", CUI_KEY_ENTER }, { "return", CUI_KEY_ENTER }, { "tab", CUI_KEY_TAB },
        { "backspace", CUI_KEY_BACKSPACE }, { "delete", CUI_KEY_DELETE }, { "del", CUI_KEY_DELETE },
        { "left", CUI_KEY_LEFT }, { "right", CUI_KEY_RIGHT }, { "up", CUI_KEY_UP }, { "down", CUI_KEY_DOWN },
        { "home", CUI_KEY_HOME }, { "end", CUI_KEY_END },
        { "pageup", CUI_KEY_PAGE_UP }, { "pagedown", CUI_KEY_PAGE_DOWN },
        { "minus", CUI_KEY_MINUS }, { "equal", CUI_KEY_EQUAL }, { "comma", CUI_KEY_COMMA },
        { "period", CUI_KEY_PERIOD }, { "slash", CUI_KEY_SLASH },
        { ",", CUI_KEY_COMMA }, { ".", CUI_KEY_PERIOD }, { "/", CUI_KEY_SLASH },
        { "-", CUI_KEY_MINUS }, { "=", CUI_KEY_EQUAL }, { "+", CUI_KEY_EQUAL },
        { "f1", CUI_KEY_F1 }, { "f2", CUI_KEY_F2 }, { "f3", CUI_KEY_F3 }, { "f4", CUI_KEY_F4 },
        { "f5", CUI_KEY_F5 }, { "f6", CUI_KEY_F6 }, { "f7", CUI_KEY_F7 }, { "f8", CUI_KEY_F8 },
        { "f9", CUI_KEY_F9 }, { "f10", CUI_KEY_F10 }, { "f11", CUI_KEY_F11 }, { "f12", CUI_KEY_F12 }
    };
    int i;
    for (i = 0; i < CUI_COUNTOF(keys); i++)
        if (cui__xml_tag_matches(name, keys[i].name)) return keys[i].key;
    if (name[0] && !name[1]) {
        char c = name[0];
        if (c >= 'a' && c <= 'z') return CUI_KEY_A + (c - 'a');
        if (c >= 'A' && c <= 'Z') return CUI_KEY_A + (c - 'A');
        if (c >= '0' && c <= '9') return CUI_KEY_0 + (c - '0');
    }
    return CUI_KEY_NONE;
}

/* "Ctrl+Shift+S" -> mods and key. Returns false when it cannot be read. */
static bool cui__xml_parse_keys(const char *text, int *mods, int *key) {
    char part[64];
    *mods = 0;
    *key = CUI_KEY_NONE;
    if (!text) return false;
    while (*text) {
        size_t n = 0;
        while (*text == ' ') text++;
        while (text[n] && text[n] != '+' && text[n] != ' ') n++;
        if (n >= sizeof part) return false;
        memcpy(part, text, n);
        part[n] = 0;
        text += n;
        while (*text == ' ') text++;
        if (cui__xml_tag_matches(part, "ctrl") || cui__xml_tag_matches(part, "control") ||
            cui__xml_tag_matches(part, "cmd") || cui__xml_tag_matches(part, "command") ||
            cui__xml_tag_matches(part, "meta") || cui__xml_tag_matches(part, "super") ||
            cui__xml_tag_matches(part, "win"))
            *mods |= CUI_MOD_PRIMARY;
        else if (cui__xml_tag_matches(part, "shift")) *mods |= CUI_MOD_SHIFT;
        else if (cui__xml_tag_matches(part, "alt") || cui__xml_tag_matches(part, "option")) *mods |= CUI_MOD_ALT;
        else *key = cui__xml_key_named(part);
        if (*text == '+' || *text == ' ') text++;
    }
    return *key != CUI_KEY_NONE;
}

/* ------------------------------------------------------------------------ */
/* Drawing                                                                  */
/* ------------------------------------------------------------------------ */

void cui_xml_draw_children(cui_xml *doc, cui_xml_node *node, cui_ctx *ui) {
    if (!doc || !node || !ui) return;
    cui__xml_draw_kids(doc, node, ui, cui_is_disabled(ui));
}

void cui__xml_draw_kids(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    int i;
    for (i = 0; i < n->kid_count; i++) cui__xml_draw(doc, n->kids[i], ui, disabled);
}

static void cui__xml_box_opts(cui_xml *doc, cui_xml_node *n, cui_dir dir, cui_box *b) {
    memset(b, 0, sizeof *b);
    b->id = n->key;
    b->dir = dir;
    cui__xml_sizes(doc, n, &b->width, &b->height);
    b->pad = cui__xml_attr_num(doc, n, "pad", 0);
    if (!b->pad) b->pad = cui__xml_attr_num(doc, n, "padding", 0);
    b->pad_x = cui__xml_attr_num(doc, n, "padx", 0);
    b->pad_y = cui__xml_attr_num(doc, n, "pady", 0);
    b->pad_top = cui__xml_attr_num(doc, n, "padtop", 0);
    b->pad_right = cui__xml_attr_num(doc, n, "padright", 0);
    b->pad_bottom = cui__xml_attr_num(doc, n, "padbottom", 0);
    b->pad_left = cui__xml_attr_num(doc, n, "padleft", 0);
    {
        static const char *const dirs[] = { "column", "row", "stack", "grid" };
        int dir_index = cui__xml_attr_choice(doc, n, "direction", dirs, 4, -1);
        if (dir_index == 1) b->dir = CUI_ROW;
        else if (dir_index == 2) b->dir = CUI_STACK;
        else if (dir_index == 3) b->dir = CUI_GRID;
        else if (dir_index == 0) b->dir = CUI_COLUMN;
    }
    b->min_width = cui__xml_attr_num(doc, n, "minwidth", 0);
    b->max_width = cui__xml_attr_num(doc, n, "maxwidth", 0);
    b->min_height = cui__xml_attr_num(doc, n, "minheight", 0);
    b->max_height = cui__xml_attr_num(doc, n, "maxheight", 0);
    b->gap = cui__xml_attr_num(doc, n, "gap", cui__xml_attr_num(doc, n, "spacing", 0));
    b->columns = (int)cui__xml_attr_num(doc, n, "columns", 0);
    b->min_column_width = cui__xml_attr_num(doc, n, "mincolumnwidth", 0);
    b->wrap = cui__xml_attr_bool(doc, n, "wrap", false);
    b->align = cui__xml_align_of(doc, n, CUI_ALIGN_DEFAULT);
    b->justify = cui__xml_justify_of(doc, n, CUI_JUSTIFY_START);
    b->bg = cui__xml_attr_color(doc, n, "background", CUI_TRANSPARENT);
    if (!b->bg.a && !b->bg.r && !b->bg.g && !b->bg.b)
        b->bg = cui__xml_attr_color(doc, n, "bg", CUI_TRANSPARENT);
    b->border = cui__xml_attr_num(doc, n, "border", 0);
    b->border_color = cui__xml_attr_color(doc, n, "bordercolor", CUI_COLOR_BORDER);
    b->radius = cui__xml_attr_num(doc, n, "radius", 0);
    b->elevation = cui__xml_elevation_of(doc, n, CUI_ELEVATION_NONE);
    b->clip = cui__xml_attr_bool(doc, n, "clip", false);
    b->scroll_y = cui__xml_attr_bool(doc, n, "scroll", false);
    b->scroll_x = cui__xml_attr_bool(doc, n, "scrollx", false);
}

/* The id and rectangle of an element that is a container: its begin function
 * gives the one of the box, and the rectangle comes from the latest layout. */
static void cui__xml_record_id(cui_xml_node *n, cui_ctx *ui, cui_id id) {
    n->id = id;
    n->rect = cui_get_rect(ui, id);
}

static void cui__xml_record_self(cui_xml_node *n, cui_ctx *ui) {
    cui__xml_record_id(n, ui, cui_make_id(ui, n->key ? n->key : "cui_xml"));
}

static void cui__xml_draw_box(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, cui_dir dir, bool disabled) {
    cui_box b;
    cui_id id;
    cui__xml_box_opts(doc, n, dir, &b);
    id = cui_box_begin(ui, &b);
    cui__xml_record_id(n, ui, id);
    cui__xml_draw_kids(doc, n, ui, disabled);
    cui_box_end(ui);
}

static void cui__xml_draw_card(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_card_opts c;
    static const char *const dirs[] = { "column", "row" };
    memset(&c, 0, sizeof c);
    c.id = n->key;
    cui__xml_sizes(doc, n, &c.width, &c.height);
    c.pad = cui__xml_attr_num(doc, n, "pad", 0);
    c.pad_x = cui__xml_attr_num(doc, n, "padx", 0);
    c.pad_y = cui__xml_attr_num(doc, n, "pady", 0);
    c.min_width = cui__xml_attr_num(doc, n, "minwidth", 0);
    c.max_width = cui__xml_attr_num(doc, n, "maxwidth", 0);
    c.gap = cui__xml_attr_num(doc, n, "gap", 0);
    c.dir = cui__xml_choice_of(doc, n, "dir", dirs, 2, 0) ? CUI_ROW : CUI_COLUMN;
    c.align = cui__xml_align_of(doc, n, CUI_ALIGN_DEFAULT);
    c.wrap = cui__xml_attr_bool(doc, n, "wrap", false);
    c.hoverable = cui__xml_attr_bool(doc, n, "hoverable", false);
    if (c.hoverable) c.id = n->key;
    c.muted = cui__xml_attr_bool(doc, n, "muted", false);
    c.selected = cui__xml_attr_bool(doc, n, "selected", false);
    cui__xml_record_self(n, ui);
    if (cui_card_begin(ui, &c)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
    cui__xml_draw_kids(doc, n, ui, disabled);
    cui_card_end(ui);
}

static void cui__xml_draw_scroll(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_scroll_opts s;
    memset(&s, 0, sizeof s);
    s.id = n->key;
    cui__xml_sizes(doc, n, &s.width, &s.height);
    s.horizontal = cui__xml_attr_bool(doc, n, "horizontal", false) || cui__xml_attr_bool(doc, n, "scrollx", false);
    s.pad = cui__xml_attr_num(doc, n, "pad", 0);
    s.gap = cui__xml_attr_num(doc, n, "gap", 0);
    s.fade_edges = cui__xml_attr_bool(doc, n, "fade", false);
    cui__xml_record_self(n, ui);
    cui_scroll_begin(ui, &s);
    cui__xml_draw_kids(doc, n, ui, disabled);
    cui_scroll_end(ui);
}

static void cui__xml_draw_split(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_split_opts o;
    float *ratio = &n->f;
    int i, seen = 0;
    memset(&o, 0, sizeof o);
    o.vertical = cui__xml_attr_bool(doc, n, "vertical", false);
    cui__xml_sizes(doc, n, &o.width, &o.height);
    if (*ratio <= 0.0f || *ratio >= 1.0f) *ratio = cui__xml_attr_num(doc, n, "ratio", 0.5f);
    if (*ratio <= 0.0f || *ratio >= 1.0f) *ratio = 0.5f;
    o.min_first = cui__xml_attr_num(doc, n, "minfirst", 0);
    o.min_second = cui__xml_attr_num(doc, n, "minsecond", 0);
    cui__xml_record_self(n, ui);
    cui_split_begin(ui, n->key, ratio, &o);
    for (i = 0; i < n->kid_count; i++) {
        if (n->kids[i]->comment) continue;
        if (seen == 1) cui_split_next(ui);
        cui__xml_draw(doc, n->kids[i], ui, disabled);
        seen++;
    }
    cui_split_end(ui);
}

static void cui__xml_draw_text_kind(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, int tag) {
    const char *text = cui__xml_label(doc, n);
    switch (tag) {
    case CUI__XML_TAG_HEADING: {
        int level = (int)cui__xml_attr_num(doc, n, "level", 2);
        if (level < 1) level = 1;
        if (level > 4) level = 4;
        cui_heading(ui, text, level);
        break;
    }
    case CUI__XML_TAG_LABEL: cui_label(ui, text); break;
    case CUI__XML_TAG_SECONDARY: cui_secondary(ui, text); break;
    case CUI__XML_TAG_MUTED: cui_muted(ui, text); break;
    case CUI__XML_TAG_CODE: cui_code(ui, text); break;
    case CUI__XML_TAG_KBD: cui_kbd(ui, text); break;
    case CUI__XML_TAG_LINK:
        if (cui_link(ui, text)) {
            n->reported = 0;
            cui__xml_fire(doc, n, "onclicked");
        }
        break;
    case CUI__XML_TAG_ICON: {
        cui_color color = cui__xml_attr_color(doc, n, "color", cui_token_color(ui, CUI_TOKEN_FOREGROUND));
        cui_icon(ui, cui__xml_icon(doc, n, "circle"), cui__xml_attr_num(doc, n, "size", 18), color);
        break;
    }
    default: {
        cui_text_opts o;
        memset(&o, 0, sizeof o);
        o.font = cui__xml_font_of(doc, n, CUI_FONT_BODY);
        o.size = cui__xml_attr_num(doc, n, "size", 0);
        o.line_height = cui__xml_attr_num(doc, n, "lineheight", 0);
        o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
        o.align = cui__xml_text_align_of(doc, n, CUI_TEXT_START);
        o.nowrap = cui__xml_attr_bool(doc, n, "nowrap", false);
        o.tabular = cui__xml_attr_bool(doc, n, "tabular", false);
        o.width = cui__xml_attr_size(doc, n, "width", CUI_FIT);
        cui_text_ex(ui, text, &o);
        break;
    }
    }
}

static void cui__xml_draw_button(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled, bool icon_only) {
    cui_button_opts o;
    memset(&o, 0, sizeof o);
    o.variant = cui__xml_variant_of(doc, n, CUI_BUTTON_DEFAULT);
    o.size = cui__xml_control_size_of(doc, n, CUI_SIZE_MD);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.icon_right = cui__xml_icon_named(doc, n, "iconright");
    o.disabled = disabled || cui__xml_attr_bool(doc, n, "disabled", false);
    o.loading = cui__xml_attr_bool(doc, n, "loading", false);
    o.full_width = cui__xml_attr_bool(doc, n, "fullwidth", false);
    o.selected = cui__xml_attr_bool(doc, n, "selected", false);
    o.tooltip = cui__xml_attr_text(doc, n, "tooltip", NULL);
    o.shortcut = cui__xml_attr_text(doc, n, "shortcut", NULL);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FIT);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    if (icon_only) {
        if (cui_icon_button_ex(ui, o.icon ? o.icon : "circle", &o)) {
            n->reported = 0;
            cui__xml_fire(doc, n, "onclicked");
        }
        return;
    }
    if (cui_button_ex(ui, cui__xml_label(doc, n), &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
}

static void cui__xml_draw_check(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled, bool is_switch) {
    cui_check_opts o;
    bool *value = cui__xml_slot_bool(doc, n);
    bool changed;
    memset(&o, 0, sizeof o);
    o.description = cui__xml_attr_text(doc, n, "description", NULL);
    o.disabled = disabled || cui__xml_attr_bool(doc, n, "disabled", false);
    if (!n->seen && cui__xml_attr_raw(n, "checked"))
        *value = cui__xml_attr_bool(doc, n, "checked", false);
    changed = is_switch ? cui_switch_ex(ui, cui__xml_label(doc, n), value, &o)
                        : cui_checkbox_ex(ui, cui__xml_label(doc, n), value, &o);
    if (changed) cui__xml_changed_hook(doc, n);
}

static void cui__xml_draw_radio_group(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_check_opts o;
    int *selected = cui__xml_slot_int(doc, n);
    int i, index = 0;
    bool changed = false;
    memset(&o, 0, sizeof o);
    o.disabled = disabled || cui__xml_attr_bool(doc, n, "disabled", false);
    for (i = 0; i < n->kid_count; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment) continue;
        if (kid->tag_id != CUI__XML_TAG_RADIO) {
            cui__xml_draw(doc, kid, ui, disabled);
            continue;
        }
        if (!kid->shown) continue;
        o.description = cui__xml_attr_text(doc, kid, "description", NULL);
        if (cui_radio_ex(ui, cui__xml_label(doc, kid), selected, index, &o)) changed = true;
        index++;
    }
    if (changed) cui__xml_changed_hook(doc, n);
}

static void cui__xml_draw_slider(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_slider_opts o;
    float *value = cui__xml_slot_float(doc, n);
    float lo = cui__xml_attr_num(doc, n, "min", 0);
    float hi = cui__xml_attr_num(doc, n, "max", 1);
    memset(&o, 0, sizeof o);
    o.step = cui__xml_attr_num(doc, n, "step", 0);
    o.show_value = cui__xml_attr_bool(doc, n, "showvalue", false);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.disabled = disabled || cui__xml_attr_bool(doc, n, "disabled", false);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    if (!n->seen) *value = cui__xml_attr_num(doc, n, "value", *value);
    if (hi <= lo) hi = lo + 1;
    if (cui_slider_ex(ui, cui__xml_label(doc, n), value, lo, hi, &o)) cui__xml_changed_hook(doc, n);
}

static void cui__xml_draw_range_slider(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    float lo = n->f;
    float hi = (float)n->d;
    float min = cui__xml_attr_num(doc, n, "min", 0);
    float max = cui__xml_attr_num(doc, n, "max", 100);
    if (!n->seen) {
        lo = cui__xml_attr_num(doc, n, "low", min);
        hi = cui__xml_attr_num(doc, n, "high", max);
    }
    if (disabled) cui_push_disabled(ui, true);
    if (cui_range_slider(ui, cui__xml_label(doc, n), &lo, &hi, min, max)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
    n->f = lo;
    n->d = hi;
}

/* ---- inputs that edit in place, drag a number, take money, keys, emoji ---- */

static void cui__xml_draw_inline_edit(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_inline_edit_opts o;
    size_t cap = 0;
    char *buf = cui__xml_slot_text(doc, n, &cap, 0);
    if (!buf || !cap) return;
    if (!n->seen) {
        const char *value = cui__xml_attr_text(doc, n, "value", NULL);
        const char *label = cui__xml_label(doc, n);
        if (!value || !*value) value = label;
        if (value && *value && !buf[0]) snprintf(buf, cap, "%s", value);
    }
    memset(&o, 0, sizeof o);
    o.font = cui__xml_font_of(doc, n, CUI_FONT_MEDIUM);
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.placeholder = cui__xml_attr_text(doc, n, "placeholder", NULL);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_inline_edit_ex(ui, n->key, buf, cap, &o) & CUI_CHANGED) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_scrub(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_scrub_opts o;
    float *value = cui__xml_slot_float(doc, n);
    const char *label = cui__xml_label(doc, n);
    float lo = cui__xml_attr_num(doc, n, "min", 0);
    float hi = cui__xml_attr_num(doc, n, "max", 100);
    memset(&o, 0, sizeof o);
    o.step = cui__xml_attr_num(doc, n, "step", 0);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.suffix = cui__xml_attr_text(doc, n, "suffix", NULL);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    if (!n->seen) *value = cui__xml_attr_num(doc, n, "value", *value);
    if (hi <= lo) hi = lo + 1;
    if (disabled) cui_push_disabled(ui, true);
    if (cui_scrub_ex(ui, label && *label ? label : n->key, value, lo, hi, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_money(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_money_opts o;
    double *value = cui__xml_slot_double(doc, n);
    memset(&o, 0, sizeof o);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.symbol = cui__xml_attr_text(doc, n, "symbol", NULL);
    o.code = cui__xml_attr_text(doc, n, "code", NULL);
    o.decimals = (int)cui__xml_attr_num(doc, n, "decimals", 2);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.hint = cui__xml_attr_text(doc, n, "hint", NULL);
    if (!n->seen) *value = (double)cui__xml_attr_num(doc, n, "value", (float)*value);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_money_input_ex(ui, n->key, value, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* The keys a recorder holds, or the recent emoji and the tone a picker holds: the
 * element keeps them while the document lives, as the C examples keep theirs. */
typedef struct {
    cui_key_combo combo;
    char recent[256];
    int tone;
    bool init;
} cui__xml_held_state;

static cui__xml_held_state *cui__xml_held(cui_xml *doc, cui_xml_node *n) {
    cui__xml_held_state *st = n->cache;
    if (st) return st;
    st = cui__xml_alloc(sizeof *st);
    if (!st) return NULL;
    memset(st, 0, sizeof *st);
    st->tone = (int)cui__xml_attr_num(doc, n, "tone", 0);
    n->cache = st;
    n->cache_free = cui__xml_free_cache;
    return st;
}

static void cui__xml_draw_shortcut_recorder(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_shortcut_recorder_opts o;
    cui__xml_held_state *st = cui__xml_held(doc, n);
    if (!st) return;
    if (!st->init) {
        const char *keys = cui__xml_attr_text(doc, n, "shortcut", NULL);
        if (keys && *keys) {
            int key = (int)CUI_KEY_NONE;
            cui__xml_parse_keys(keys, &st->combo.mods, &key);
            st->combo.key = (cui_key)key;
        }
        st->init = true;
    }
    memset(&o, 0, sizeof o);
    o.placeholder = cui__xml_attr_text(doc, n, "placeholder", NULL);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    if (disabled) cui_push_disabled(ui, true);
    if (cui_shortcut_recorder_ex(ui, n->key, &st->combo, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_emoji_picker(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_emoji_picker_opts o;
    cui__xml_held_state *st = cui__xml_held(doc, n);
    const char *picked;
    if (!st) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.recent = st->recent;
    o.recent_cap = sizeof st->recent;
    o.tone = &st->tone;
    o.placeholder = cui__xml_attr_text(doc, n, "placeholder", NULL);
    o.hint = cui__xml_attr_text(doc, n, "hint", NULL);
    o.no_search = cui__xml_attr_bool(doc, n, "nosearch", false);
    if (disabled) cui_push_disabled(ui, true);
    picked = cui_emoji_picker_ex(ui, n->key, &o);
    if (disabled) cui_pop_disabled(ui);
    if (picked && *picked) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onchanged");
    }
}

/* ---- disclosure: cards on hover, confirmations, veils, countdowns -------- */

static void cui__xml_draw_hover_card(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_hover_card_opts o;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_num(doc, n, "width", 0);
    o.placement = cui__xml_placement_of(doc, n, CUI_PLACE_BOTTOM_START);
    o.delay = cui__xml_attr_num(doc, n, "delay", 0);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_hover_card_begin(ui, n->key, &o)) {
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_hover_card_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_confirm_popover(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_confirm_popover_opts o;
    memset(&o, 0, sizeof o);
    o.title = cui__xml_attr_text(doc, n, "title", NULL);
    o.description = cui__xml_attr_text(doc, n, "description", NULL);
    o.confirm_label = cui__xml_attr_text(doc, n, "confirmlabel", NULL);
    o.cancel_label = cui__xml_attr_text(doc, n, "cancellabel", NULL);
    o.danger = cui__xml_attr_bool(doc, n, "danger", false);
    o.placement = cui__xml_placement_of(doc, n, CUI_PLACE_BOTTOM_START);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_confirm_popover_ex(ui, n->key, &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_expandable_text(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_expandable_text_opts o;
    memset(&o, 0, sizeof o);
    o.lines = (int)cui__xml_attr_num(doc, n, "lines", 0);
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    o.fade_to = cui__xml_attr_color(doc, n, "fadeto", (cui_color){ 0, 0, 0, 0 });
    if (disabled) cui_push_disabled(ui, true);
    if (cui_expandable_text_ex(ui, n->key, cui__xml_label(doc, n), &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_expandable_card(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_expandable_card_opts o;
    bool *open = cui__xml_slot_bool(doc, n);
    memset(&o, 0, sizeof o);
    o.description = cui__xml_attr_text(doc, n, "description", NULL);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.badge = cui__xml_attr_text(doc, n, "badge", NULL);
    o.badge_tone = cui__xml_tone_attr(doc, n, "badgetone", CUI_TONE_NEUTRAL);
    if (!n->seen) *open = cui__xml_attr_bool(doc, n, "open", false);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_expandable_card_begin(ui, cui__xml_label(doc, n), open, &o)) {
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_expandable_card_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
}

/* A countdown runs from the second its element is first drawn, as the C examples
 * do it: the document says how much is left, the element keeps the moment it
 * counts down to. */
typedef struct { double target; bool init; } cui__xml_countdown_state;

static void cui__xml_draw_countdown(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_countdown_opts o;
    cui__xml_countdown_state *st = n->cache;
    memset(&o, 0, sizeof o);
    o.hide_days = cui__xml_attr_bool(doc, n, "hidedays", false);
    o.compact = cui__xml_attr_bool(doc, n, "compact", false);
    if (!st) {
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    if (!st->init) {
        st->target = cui_time(ui) + (double)cui__xml_attr_num(doc, n, "seconds", 0);
        st->init = true;
    }
    cui_countdown_ex(ui, n->key, st->target - cui_time(ui), &o);
}

/* Keep the completion state on the step, including toggles made by the widget. */
typedef struct { bool init; } cui__xml_step_state;

static void cui__xml_draw_checklist(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_checklist_opts o;
    cui_check_step steps[16];
    cui_xml_node *kids[16];
    int count = 0, i, acted;
    bool changed = false;
    memset(&o, 0, sizeof o);
    o.title = cui__xml_attr_text(doc, n, "title", NULL);
    o.description = cui__xml_attr_text(doc, n, "description", NULL);
    for (i = 0; i < n->kid_count && count < 16; i++) {
        cui_xml_node *kid = n->kids[i];
        cui__xml_step_state *st;
        if (kid->comment || kid->tag_id != CUI__XML_TAG_STEP) continue;
        st = kid->cache;
        if (!st) {
            st = cui__xml_alloc(sizeof *st);
            if (!st) return;
            memset(st, 0, sizeof *st);
            kid->cache = st;
            kid->cache_free = cui__xml_free_cache;
        }
        if (!st->init) {
            *cui__xml_slot_bool(doc, kid) = cui__xml_attr_bool(doc, kid, "done", false);
            st->init = true;
        }
        steps[count].title = cui__xml_label(doc, kid);
        steps[count].description = cui__xml_attr_text(doc, kid, "description", NULL);
        steps[count].action = cui__xml_attr_text(doc, kid, "action", NULL);
        steps[count].done = *cui__xml_slot_bool(doc, kid);
        kids[count] = kid;
        count++;
    }
    if (!count) return;
    if (disabled) cui_push_disabled(ui, true);
    acted = cui_checklist_ex(ui, n->key, steps, count, &o);
    if (disabled) cui_pop_disabled(ui);
    if (acted >= 0 && acted < count) steps[acted].done = true;
    for (i = 0; i < count; i++) {
        bool *done = cui__xml_slot_bool(doc, kids[i]);
        if (*done != steps[i].done) {
            *done = steps[i].done;
            changed = true;
        }
    }
    if (acted >= 0 && acted < count) {
        n->reported = acted;
        cui__xml_fire(doc, n, "onclicked");
    }
    if (changed) cui__xml_changed_hook(doc, n);
}

static void cui__xml_draw_veil(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_veil_opts o;
    bool *revealed = cui__xml_slot_bool(doc, n);
    memset(&o, 0, sizeof o);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.hint = cui__xml_attr_text(doc, n, "hint", NULL);
    o.toggle = cui__xml_attr_bool(doc, n, "toggle", false);
    o.radius = cui__xml_attr_num(doc, n, "radius", 0);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    if (!n->seen) *revealed = cui__xml_attr_bool(doc, n, "revealed", false);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_veil_begin_ex(ui, n->key, revealed, &o)) {
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_veil_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
}

/* ---- pickers: several choices, ranges, times, wheels, rulers ------------ */

static int cui__xml_split(cui_ctx *ui, const char *text, char sep, char **out, int cap);
static int cui__xml_levels_of(cui_ctx *ui, const char *text, float *out, int cap);
static int cui__xml_live_numbers(const char *text, float *out, int cap);

/* "2026-11-03" */
static cui_date cui__xml_date_value(const char *text) {
    cui_date d;
    memset(&d, 0, sizeof d);
    if (text) sscanf(text, "%d-%d-%d", &d.year, &d.month, &d.day);
    return d;
}

typedef struct {
    bool selected[16];
    cui_date_range range;
    int hour, minute;
    bool init;
} cui__xml_picker_state;

static cui__xml_picker_state *cui__xml_picker(cui_xml *doc, cui_xml_node *n, bool *fresh) {
    cui__xml_picker_state *st = n->cache;
    *fresh = false;
    if (st) return st;
    st = cui__xml_alloc(sizeof *st);
    if (!st) return NULL;
    memset(st, 0, sizeof *st);
    n->cache = st;
    n->cache_free = cui__xml_free_cache;
    *fresh = true;
    return st;
}

static void cui__xml_draw_multi_select(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_multi_select_opts o;
    char **items = cui__frame_alloc(ui, sizeof *items * 16);
    int count, i;
    bool fresh;
    cui__xml_picker_state *st = cui__xml_picker(doc, n, &fresh);
    if (!st) return;
    count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "items", NULL), '|', items, 16);
    if (!count) return;
    if (fresh) {
        char *on[16];
        int on_count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "on", NULL), ',', on, 16);
        for (i = 0; i < on_count; i++) {
            int index = atoi(on[i]);
            if (index >= 0 && index < 16) st->selected[index] = true;
        }
    }
    memset(&o, 0, sizeof o);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.placeholder = cui__xml_attr_text(doc, n, "placeholder", NULL);
    o.searchable = cui__xml_attr_bool(doc, n, "searchable", false);
    o.max_chips = (int)cui__xml_attr_num(doc, n, "maxchips", 0);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    if (disabled) cui_push_disabled(ui, true);
    if (cui_multi_select_ex(ui, n->key, (const char *const *)items, count, st->selected, &o))
        cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_date_range_picker(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_date_range_opts o;
    bool fresh;
    cui__xml_picker_state *st = cui__xml_picker(doc, n, &fresh);
    if (!st) return;
    if (fresh) {
        st->range.start = cui__xml_date_value(cui__xml_attr_text(doc, n, "from", NULL));
        st->range.end = cui__xml_date_value(cui__xml_attr_text(doc, n, "to", NULL));
    }
    memset(&o, 0, sizeof o);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.placeholder = cui__xml_attr_text(doc, n, "placeholder", NULL);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    o.no_presets = cui__xml_attr_bool(doc, n, "nopresets", false);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_date_range_picker_ex(ui, n->key, &st->range, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_range_calendar(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    bool fresh;
    int months = (int)cui__xml_attr_num(doc, n, "months", 2);
    cui__xml_picker_state *st = cui__xml_picker(doc, n, &fresh);
    if (!st) return;
    if (fresh) {
        st->range.start = cui__xml_date_value(cui__xml_attr_text(doc, n, "from", NULL));
        st->range.end = cui__xml_date_value(cui__xml_attr_text(doc, n, "to", NULL));
    }
    if (months < 1) months = 1;
    if (disabled) cui_push_disabled(ui, true);
    if (cui_range_calendar(ui, n->key, &st->range, months)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_time_picker(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_time_opts o;
    bool fresh;
    cui__xml_picker_state *st = cui__xml_picker(doc, n, &fresh);
    if (!st) return;
    if (fresh) {
        st->hour = (int)cui__xml_attr_num(doc, n, "hour", 0);
        st->minute = (int)cui__xml_attr_num(doc, n, "minute", 0);
    }
    memset(&o, 0, sizeof o);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.twelve_hour = cui__xml_attr_bool(doc, n, "twelvehour", false);
    o.minute_step = (int)cui__xml_attr_num(doc, n, "minutestep", 0);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    if (disabled) cui_push_disabled(ui, true);
    if (cui_time_picker_ex(ui, n->key, &st->hour, &st->minute, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_time_dial(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    bool fresh;
    float size = cui__xml_attr_num(doc, n, "size", 0);
    cui__xml_picker_state *st = cui__xml_picker(doc, n, &fresh);
    if (!st) return;
    if (fresh) {
        st->hour = (int)cui__xml_attr_num(doc, n, "hour", 0);
        st->minute = (int)cui__xml_attr_num(doc, n, "minute", 0);
    }
    if (disabled) cui_push_disabled(ui, true);
    if (cui_time_dial(ui, n->key, &st->hour, &st->minute, size)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_wheel_picker(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_wheel_opts o;
    char **items = cui__frame_alloc(ui, sizeof *items * 64);
    int *index = cui__xml_slot_int(doc, n);
    int count;
    if (!items) return;
    count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "items", NULL), '|', items, 64);
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.rows = (int)cui__xml_attr_num(doc, n, "rows", 0);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    o.loop = cui__xml_attr_bool(doc, n, "loop", false);
    o.join_left = cui__xml_attr_bool(doc, n, "joinleft", false);
    o.join_right = cui__xml_attr_bool(doc, n, "joinright", false);
    if (!n->seen) *index = (int)cui__xml_attr_num(doc, n, "value", *index);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_wheel_picker_ex(ui, n->key, (const char *const *)items, count, index, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_ruler_slider(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_ruler_opts o;
    float *value = cui__xml_slot_float(doc, n);
    const char *label = cui__xml_label(doc, n);
    float lo = cui__xml_attr_num(doc, n, "min", 0);
    float hi = cui__xml_attr_num(doc, n, "max", 100);
    memset(&o, 0, sizeof o);
    o.step = cui__xml_attr_num(doc, n, "step", 0);
    o.major_every = (int)cui__xml_attr_num(doc, n, "majorevery", 0);
    o.spacing = cui__xml_attr_num(doc, n, "spacing", 0);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.unit = cui__xml_attr_text(doc, n, "unit", NULL);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    if (!n->seen) *value = cui__xml_attr_num(doc, n, "value", *value);
    if (hi <= lo) hi = lo + 1;
    if (disabled) cui_push_disabled(ui, true);
    if (cui_ruler_slider_ex(ui, label && *label ? label : n->key, value, lo, hi, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* The two thumbs of a histogram slider live in the element, as the C examples
 * keep them in two floats of the page. */
typedef struct { float lo, hi; } cui__xml_range_state;

static void cui__xml_draw_histogram_slider(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_histogram_opts o;
    const float *bins = NULL;
    int bin_count = cui__xml_attr_numbers(doc, n, "values", &bins);
    float min = cui__xml_attr_num(doc, n, "min", 0);
    float max = cui__xml_attr_num(doc, n, "max", 100);
    const char *label = cui__xml_label(doc, n);
    cui__xml_range_state *st = n->cache;
    if (bin_count < 1) return;
    if (!st) {
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        st->lo = cui__xml_attr_num(doc, n, "low", min);
        st->hi = cui__xml_attr_num(doc, n, "high", max);
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    memset(&o, 0, sizeof o);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    if (max <= min) max = min + 1;
    if (disabled) cui_push_disabled(ui, true);
    if (cui_histogram_slider_ex(ui, label && *label ? label : n->key, bins, bin_count, &st->lo, &st->hi, min, max, &o))
        cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* ---- actions: split buttons, toggles, morphing buttons, reactions ------- */

/* Splits `text` on `sep` into copies that live until the frame ends. */
static int cui__xml_split(cui_ctx *ui, const char *text, char sep, char **out, int cap) {
    int n = 0;
    while (text && *text && n < cap) {
        const char *at = strchr(text, sep);
        size_t len = at ? (size_t)(at - text) : strlen(text);
        char *copy = cui__frame_alloc(ui, len + 1);
        if (!copy) break;
        memcpy(copy, text, len);
        copy[len] = 0;
        out[n++] = copy;
        text = at ? at + 1 : NULL;
    }
    return n;
}

static void cui__xml_draw_split_button(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_split_button_opts o;
    char **items = cui__frame_alloc(ui, sizeof *items * 16);
    int count, chosen, icon_count = 0;
    memset(&o, 0, sizeof o);
    o.variant = cui__xml_variant_of(doc, n, CUI_BUTTON_PRIMARY);
    o.size = cui__xml_control_size_of(doc, n, CUI_SIZE_MD);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.disabled = disabled || cui__xml_attr_bool(doc, n, "disabled", false);
    count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "items", NULL), '|', items, 16);
    if (!count) return;
    if (cui__xml_attr_raw(n, "icons")) {
        const char **icons = cui__xml_attr_list(doc, n, "icons", &icon_count, true);
        if (icons && icon_count == count) o.item_icons = icons;
    }
    chosen = cui_split_button_ex(ui, cui__xml_label(doc, n), (const char *const *)items, count, &o);
    if (chosen >= 0) {
        n->reported = chosen + 1;
        cui__xml_fire(doc, n, "onclicked");
    }
}

/* What a toggle group keeps: one flag per item, as the document first draws it. */
typedef struct {
    bool values[16];
    bool init;
} cui__xml_toggle_state;

static void cui__xml_draw_toggle_group(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_toggle_group_opts o;
    char **labels = cui__frame_alloc(ui, sizeof *labels * 16);
    int count, icon_count = 0, i;
    cui__xml_toggle_state *st = n->cache;
    count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "items", NULL), '|', labels, 16);
    if (!count) return;
    if (!st) {
        char *on[16];
        int on_count;
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
        on_count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "on", NULL), ',', on, 16);
        for (i = 0; i < on_count; i++) {
            int index = atoi(on[i]);
            if (index >= 0 && index < 16) st->values[index] = true;
        }
        st->init = true;
    }
    memset(&o, 0, sizeof o);
    o.exclusive = cui__xml_attr_bool(doc, n, "exclusive", false);
    o.icon_only = cui__xml_attr_bool(doc, n, "icononly", false);
    o.size = cui__xml_control_size_of(doc, n, CUI_SIZE_MD);
    if (cui__xml_attr_raw(n, "icons")) {
        const char **icons = cui__xml_attr_list(doc, n, "icons", &icon_count, true);
        if (icons && icon_count == count) o.icons = icons;
    }
    if (disabled) cui_push_disabled(ui, true);
    if (cui_toggle_group_ex(ui, n->key, (const char *const *)labels, count, st->values, &o))
        cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_confirm_button(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_confirm_opts o;
    memset(&o, 0, sizeof o);
    o.confirm_label = cui__xml_attr_text(doc, n, "confirmlabel", NULL);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.variant = cui__xml_variant_of(doc, n, CUI_BUTTON_SECONDARY);
    o.size = cui__xml_control_size_of(doc, n, CUI_SIZE_MD);
    o.timeout = cui__xml_attr_num(doc, n, "timeout", 0);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_confirm_button_ex(ui, cui__xml_label(doc, n), &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_status_button(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    static const char *const states[] = { "idle", "busy", "success", "error" };
    cui_status_button_opts o;
    int status = (int)cui__xml_choice_of(doc, n, "status", states, 4, CUI_STATUS_IDLE);
    memset(&o, 0, sizeof o);
    o.busy_label = cui__xml_attr_text(doc, n, "busylabel", NULL);
    o.success_label = cui__xml_attr_text(doc, n, "successlabel", NULL);
    o.error_label = cui__xml_attr_text(doc, n, "errorlabel", NULL);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.variant = cui__xml_variant_of(doc, n, CUI_BUTTON_PRIMARY);
    o.size = cui__xml_control_size_of(doc, n, CUI_SIZE_MD);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_status_button_ex(ui, cui__xml_label(doc, n), (cui_status)status, &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
    if (disabled) cui_pop_disabled(ui);
}

/* "icon:count[:on]" entries, as the reactions keep them. */
typedef struct {
    cui_reaction items[8];
    int count;
    bool init;
} cui__xml_reaction_state;

static void cui__xml_draw_reactions(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui__xml_reaction_state *st = n->cache;
    if (!st) {
        char *entries[8];
        int count, i;
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
        count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "items", NULL), '|', entries, 8);
        for (i = 0; i < count; i++) {
            char *fields[3];
            int field_count = cui__xml_split(ui, entries[i], ':', fields, 3);
            if (field_count < 1) continue;
            st->items[st->count].icon = cui__xml_icon_glyph(fields[0]);
            st->items[st->count].count = field_count > 1 ? atoi(fields[1]) : 0;
            st->items[st->count].active = field_count > 2 && (!strcmp(fields[2], "on") || !strcmp(fields[2], "1"));
            st->count++;
        }
        st->init = true;
    }
    if (!st->count) return;
    if (disabled) cui_push_disabled(ui, true);
    if (cui_reactions(ui, n->key, st->items, st->count) >= 0) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_theme_switcher(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    int *mode = cui__xml_slot_int(doc, n);
    if (!n->seen) *mode = (int)cui__xml_attr_num(doc, n, "mode", -1);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_theme_switcher(ui, mode, cui__xml_attr_bool(doc, n, "system", true))) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_theme_toggle(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    if (disabled) cui_push_disabled(ui, true);
    if (cui_theme_toggle(ui)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_user_menu(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_user_menu_opts o;
    memset(&o, 0, sizeof o);
    o.detail = cui__xml_attr_text(doc, n, "detail", NULL);
    o.status = cui__xml_attr_text(doc, n, "status", NULL);
    o.compact = cui__xml_attr_bool(doc, n, "compact", false);
    o.placement = cui__xml_placement_of(doc, n, CUI_PLACE_BOTTOM_START);
    o.width = cui__xml_attr_num(doc, n, "width", 0);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_user_menu_begin(ui, cui__xml_label(doc, n), &o)) {
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_user_menu_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
}

/* "icon:label:tone" entries for the swipe actions behind a row. */
static int cui__xml_swipe_actions(cui_ctx *ui, const char *text, cui_swipe_action *out, int cap) {
    static const char *const tones[] = { "neutral", "info", "success", "warning", "danger", "violet", "teal", "rose", "accent" };
    char *entries[8];
    int count = cui__xml_split(ui, text, '|', entries, 8), i;
    if (count > cap) count = cap;
    for (i = 0; i < count; i++) {
        char *fields[3];
        int field_count = cui__xml_split(ui, entries[i], ':', fields, 3);
        int tone = 0, k;
        out[i].icon = field_count > 0 ? cui__xml_icon_glyph(fields[0]) : CUI_ICON_CHECK;
        out[i].label = field_count > 1 ? fields[1] : "";
        if (field_count > 2) {
            for (k = 0; k < 9; k++)
                if (!strcmp(fields[2], tones[k])) tone = k;
        }
        out[i].tone = (cui_tone)tone;
    }
    return count;
}

static void cui__xml_draw_swipe_item(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_swipe_action *left = cui__frame_alloc(ui, sizeof *left * 4);
    cui_swipe_action *right = cui__frame_alloc(ui, sizeof *right * 4);
    int left_count, right_count, action;
    if (!left || !right) return;
    left_count = cui__xml_swipe_actions(ui, cui__xml_attr_text(doc, n, "left", NULL), left, 4);
    right_count = cui__xml_swipe_actions(ui, cui__xml_attr_text(doc, n, "right", NULL), right, 4);
    if (disabled) cui_push_disabled(ui, true);
    cui_swipe_item_begin(ui, n->key, left, left_count, right, right_count);
    cui__xml_draw_kids(doc, n, ui, disabled);
    action = cui_swipe_item_end(ui);
    if (disabled) cui_pop_disabled(ui);
    if (action >= 0) {
        n->reported = action;
        cui__xml_fire(doc, n, "onclicked");
    }
}

static void cui__xml_draw_speed_dial(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_speed_dial_opts o;
    cui_speed_action *actions = cui__frame_alloc(ui, sizeof *actions * 8);
    char *entries[8];
    int count, i, chosen;
    memset(&o, 0, sizeof o);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.placement = cui__xml_placement_of(doc, n, CUI_PLACE_BOTTOM_END);
    o.tooltip = cui__xml_attr_text(doc, n, "tooltip", NULL);
    count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "items", NULL), '|', entries, 8);
    if (!count) return;
    for (i = 0; i < count; i++) {
        char *fields[2];
        int field_count = cui__xml_split(ui, entries[i], ':', fields, 2);
        actions[i].icon = cui__xml_icon_glyph(fields[0]);
        actions[i].label = field_count > 1 ? fields[1] : "";
    }
    if (disabled) cui_push_disabled(ui, true);
    chosen = cui_speed_dial_ex(ui, n->key, actions, count, &o);
    if (disabled) cui_pop_disabled(ui);
    if (chosen >= 0) {
        n->reported = chosen + 1;
        cui__xml_fire(doc, n, "onclicked");
    }
}

/* ---- AI: a conversation, its trace, and what starts one ------------------ */

static void cui__xml_draw_chat_message(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_chat_opts o;
    memset(&o, 0, sizeof o);
    o.mine = cui__xml_attr_bool(doc, n, "mine", false);
    o.author = cui__xml_attr_text(doc, n, "author", NULL);
    o.time = cui__xml_attr_text(doc, n, "time", NULL);
    o.status = cui__xml_attr_text(doc, n, "status", NULL);
    o.grouped = cui__xml_attr_bool(doc, n, "grouped", false);
    o.last_in_group = cui__xml_attr_bool(doc, n, "lastingroup", false);
    o.markdown = cui__xml_attr_bool(doc, n, "markdown", false);
    cui_chat_message_ex(ui, cui__xml_label(doc, n), &o);
}

static void cui__xml_draw_reasoning(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_reasoning_opts o;
    memset(&o, 0, sizeof o);
    o.thinking = cui__xml_attr_bool(doc, n, "thinking", false);
    o.seconds = cui__xml_attr_num(doc, n, "seconds", 0);
    o.title = cui__xml_attr_text(doc, n, "title", NULL);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_reasoning_begin(ui, n->key, &o)) {
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_reasoning_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
}

static cui_task_status cui__xml_task_status_of(cui_xml *doc, cui_xml_node *n, cui_task_status fallback) {
    static const char *const names[] = { "pending", "running", "done", "failed", "skipped" };
    return (cui_task_status)cui__xml_choice_of(doc, n, "status", names, 5, (int)fallback);
}

static void cui__xml_draw_tool_call(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_tool_call_opts o;
    const char *name = cui__xml_label(doc, n);
    memset(&o, 0, sizeof o);
    o.status = cui__xml_task_status_of(doc, n, CUI_TASK_PENDING);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.detail = cui__xml_attr_text(doc, n, "detail", NULL);
    o.input = cui__xml_attr_text(doc, n, "input", NULL);
    o.output = cui__xml_attr_text(doc, n, "output", NULL);
    o.seconds = cui__xml_attr_num(doc, n, "seconds", 0);
    cui_tool_call_ex(ui, n->key, name && *name ? name : n->key, &o);
}

static void cui__xml_draw_agent_status(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_agent_status_opts o;
    memset(&o, 0, sizeof o);
    o.status = cui__xml_task_status_of(doc, n, CUI_TASK_RUNNING);
    o.seconds = cui__xml_attr_num(doc, n, "seconds", 0);
    o.icon = cui__xml_icon(doc, n, NULL);
    cui_agent_status_ex(ui, cui__xml_label(doc, n), &o);
}

static void cui__xml_draw_typewriter(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_text_fx_opts o;
    int replay = (int)cui__xml_attr_num(doc, n, "replay", 0);
    memset(&o, 0, sizeof o);
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.font = cui__xml_font_of(doc, n, CUI_FONT_BODY);
    o.speed = cui__xml_attr_num(doc, n, "speed", 0);
    o.delay = cui__xml_attr_num(doc, n, "delay", 0);
    o.caret = cui__xml_attr_bool(doc, n, "caret", false);
    o.nowrap = cui__xml_attr_bool(doc, n, "nowrap", false);
    if (replay) cui_push_id_int(ui, replay);
    cui_typewriter_ex(ui, n->key, cui__xml_label(doc, n), &o);
    if (replay) cui_pop_id(ui);
}

/* The sources an answer used: a card each, and the citations that point into
 * the list by its Name. */
static int cui__xml_sources_of(cui_xml *doc, cui_xml_node *list, cui_source *out, int cap) {
    int count = 0, i;
    if (!list) return 0;
    for (i = 0; i < list->kid_count && count < cap; i++) {
        cui_xml_node *kid = list->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_SOURCE) continue;
        out[count].title = cui__xml_attr_text(doc, kid, "title", NULL);
        out[count].url = cui__xml_attr_text(doc, kid, "url", NULL);
        out[count].snippet = cui__xml_attr_text(doc, kid, "snippet", NULL);
        count++;
    }
    return count;
}

static void cui__xml_draw_citation(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    const char *from = cui__xml_attr_text(doc, n, "from", NULL);
    int index = (int)cui__xml_attr_num(doc, n, "index", 0);
    int number = (int)cui__xml_attr_num(doc, n, "number", index + 1);
    cui_source source;
    cui_source *list = cui__frame_alloc(ui, sizeof *list * 16);
    memset(&source, 0, sizeof source);
    if (list && from && *from) {
        int count = cui__xml_sources_of(doc, cui_xml_find(doc, from), list, 16);
        if (index >= 0 && index < count) source = list[index];
    }
    if (cui_citation(ui, number, &source)) {
        n->reported = number;
        cui__xml_fire(doc, n, "onclicked");
    }
}

static void cui__xml_draw_source_list(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_source *list = cui__frame_alloc(ui, sizeof *list * 16);
    int count;
    if (!list) return;
    count = cui__xml_sources_of(doc, n, list, 16);
    if (!count) return;
    if (cui_source_list(ui, n->key, list, count) >= 0) cui__xml_changed_hook(doc, n);
}

static void cui__xml_draw_response_actions(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_response_actions_opts o;
    int *vote = cui__xml_slot_int(doc, n);
    int flags;
    memset(&o, 0, sizeof o);
    o.copy_text = cui__xml_attr_text(doc, n, "copytext", NULL);
    o.vote = vote;
    o.no_regenerate = cui__xml_attr_bool(doc, n, "noregenerate", false);
    o.no_share = cui__xml_attr_bool(doc, n, "noshare", false);
    flags = cui_response_actions_ex(ui, n->key, &o);
    if (flags) {
        n->reported = flags;
        cui__xml_fire(doc, n, "onclicked");
    }
}

static void cui__xml_draw_context_meter(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_context_opts o;
    cui_viz_item *parts = cui__frame_alloc(ui, sizeof *parts * 8);
    int count = 0, i;
    memset(&o, 0, sizeof o);
    if (!parts) return;
    for (i = 0; i < n->kid_count && count < 8; i++) {
        cui_xml_node *kid = n->kids[i];
        const char *label;
        if (kid->comment || kid->tag_id != CUI__XML_TAG_PART) continue;
        label = cui__xml_attr_text(doc, kid, "name", cui__xml_label(doc, kid));
        parts[count].label = label;
        parts[count].value = cui__xml_attr_num(doc, kid, "value", 0);
        parts[count].color = cui__xml_attr_color(doc, kid, "color", (cui_color){ 0, 0, 0, 0 });
        count++;
    }
    o.parts = count ? parts : NULL;
    o.part_count = count;
    o.unit = cui__xml_attr_text(doc, n, "unit", NULL);
    cui_context_meter_ex(ui, n->key, cui__xml_attr_num(doc, n, "used", 0), cui__xml_attr_num(doc, n, "limit", 1), &o);
}

/* "name:detail:badge:icon" entries for the composer's model picker. */
static int cui__xml_models_of(cui_ctx *ui, const char *text, cui_model *out, int cap) {
    char **entries = cui__frame_alloc(ui, sizeof *entries * 8);
    int count, i;
    if (!entries) return 0;
    count = cui__xml_split(ui, text, '|', entries, 8);
    if (count > cap) count = cap;
    for (i = 0; i < count; i++) {
        char *fields[4];
        int field_count = cui__xml_split(ui, entries[i], ':', fields, 4);
        out[i].name = field_count > 0 ? fields[0] : "";
        out[i].detail = field_count > 1 && *fields[1] ? fields[1] : NULL;
        out[i].badge = field_count > 2 && *fields[2] ? fields[2] : NULL;
        out[i].icon = field_count > 3 && *fields[3] ? cui__xml_icon_glyph(fields[3]) : NULL;
    }
    return count;
}

static void cui__xml_draw_chat_composer(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_composer_opts o;
    cui_model *models = cui__frame_alloc(ui, sizeof *models * 8);
    char **attachments = cui__frame_alloc(ui, sizeof *attachments * 8);
    int *model = cui__xml_slot_int(doc, n);
    size_t cap = 0;
    char *buf = cui__xml_slot_text(doc, n, &cap, (size_t)cui__xml_attr_num(doc, n, "maxlength", 512));
    int events, removed = -1;
    if (!models || !attachments || !buf || !cap) return;
    memset(&o, 0, sizeof o);
    o.placeholder = cui__xml_attr_text(doc, n, "placeholder", NULL);
    o.busy = cui__xml_attr_bool(doc, n, "busy", false);
    o.attachment_count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "attachments", NULL), '|', attachments, 8);
    o.attachments = (const char *const *)attachments;
    o.removed = &removed;
    o.model_count = cui__xml_models_of(ui, cui__xml_attr_text(doc, n, "models", NULL), models, 8);
    o.models = o.model_count ? models : NULL;
    o.model = o.model_count ? model : NULL;
    o.max_lines = (int)cui__xml_attr_num(doc, n, "maxlines", 0);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.mic = cui__xml_attr_bool(doc, n, "mic", false);
    o.recording = cui__xml_attr_bool(doc, n, "recording", false);
    if (o.mic) {
        float *levels = cui__frame_alloc(ui, sizeof *levels * 600);
        if (levels) {
            o.level_count = cui__xml_levels_of(ui, cui__xml_attr_text(doc, n, "levels", NULL), levels, 600);
            o.levels = o.level_count ? levels : NULL;
        }
    }
    o.record_seconds = cui__xml_attr_num(doc, n, "recordseconds", 0);
    if (o.model_count && !n->seen) *model = (int)cui__xml_attr_num(doc, n, "model", 0);
    if (disabled) cui_push_disabled(ui, true);
    events = cui_chat_composer_ex(ui, n->key, buf, cap, &o);
    if (disabled) cui_pop_disabled(ui);
    if (events) {
        if ((events & CUI_COMPOSER_REMOVE) && removed >= 0) {
            n->reported = removed;
            cui__xml_fire(doc, n, "onremoved");
        }
        n->reported = events;
        cui__xml_fire(doc, n, "onchanged");
    }
}

static void cui__xml_draw_agent_plan(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_agent_plan_opts o;
    cui_plan_step *steps = cui__frame_alloc(ui, sizeof *steps * 16);
    int count = 0, i;
    memset(&o, 0, sizeof o);
    if (!steps) return;
    o.title = cui__xml_attr_text(doc, n, "title", NULL);
    for (i = 0; i < n->kid_count && count < 16; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_PLANSTEP) continue;
        steps[count].title = cui__xml_label(doc, kid);
        steps[count].detail = cui__xml_attr_text(doc, kid, "detail", NULL);
        steps[count].status = cui__xml_task_status_of(doc, kid, CUI_TASK_PENDING);
        count++;
    }
    if (!count) return;
    cui_agent_plan_ex(ui, n->key, steps, count, &o);
}

static void cui__xml_draw_prompt_suggestions(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_prompt_suggestions_opts o;
    cui_prompt_suggestion *items = cui__frame_alloc(ui, sizeof *items * 8);
    int count = 0, i, chosen;
    memset(&o, 0, sizeof o);
    if (!items) return;
    o.columns = (int)cui__xml_attr_num(doc, n, "columns", 0);
    for (i = 0; i < n->kid_count && count < 8; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_SUGGESTION) continue;
        items[count].icon = cui__xml_icon(doc, kid, NULL);
        items[count].title = cui__xml_label(doc, kid);
        items[count].detail = cui__xml_attr_text(doc, kid, "detail", NULL);
        count++;
    }
    if (!count) return;
    chosen = cui_prompt_suggestions_ex(ui, n->key, items, count, &o);
    if (chosen >= 0) {
        n->reported = chosen;
        cui__xml_fire(doc, n, "onclicked");
    }
}

/* ---- creative controls: ratios, gradient tracks, palettes, seeds -------- */

static void cui__xml_draw_ratio_slider(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_ratio_slider_opts o;
    float *ratio = cui__xml_slot_float(doc, n);
    const char *first = cui__xml_attr_text(doc, n, "first", NULL);
    const char *second = cui__xml_attr_text(doc, n, "second", NULL);
    memset(&o, 0, sizeof o);
    o.tone = cui__xml_tone_of(doc, n, CUI_TONE_NEUTRAL);
    o.step = cui__xml_attr_num(doc, n, "step", 0);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    if (!n->seen) *ratio = cui__xml_attr_num(doc, n, "value", *ratio);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_ratio_slider_ex(ui, first && *first ? first : n->key, second && *second ? second : "", ratio, &o))
        cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_gradient_slider(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    static const char *const tracks[] = { "hue", "temperature", "opacity" };
    cui_gradient_slider_opts o;
    float *value = cui__xml_slot_float(doc, n);
    const char *label = cui__xml_label(doc, n);
    float lo = cui__xml_attr_num(doc, n, "min", 0);
    float hi = cui__xml_attr_num(doc, n, "max", 1);
    int flags;
    memset(&o, 0, sizeof o);
    o.track = (cui_gradient_track)cui__xml_choice_of(doc, n, "track", tracks, 3, 0);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    o.step = cui__xml_attr_num(doc, n, "step", 0);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.value = cui__xml_attr_text(doc, n, "valuetext", NULL);
    o.swatch = cui__xml_attr_bool(doc, n, "swatch", false);
    o.tool = cui__xml_icon(doc, n, NULL);
    o.tool_tooltip = cui__xml_attr_text(doc, n, "tooltip", NULL);
    o.tool_on = cui__xml_attr_bool(doc, n, "toolon", false);
    if (!n->seen) *value = cui__xml_attr_num(doc, n, "value", *value);
    if (hi <= lo) hi = lo + 1;
    if (disabled) cui_push_disabled(ui, true);
    flags = cui_gradient_slider_ex(ui, label && *label ? label : n->key, value, lo, hi, &o);
    if (disabled) cui_pop_disabled(ui);
    if (flags) {
        n->reported = flags;
        cui__xml_fire(doc, n, flags & CUI_WELL_ACTION ? "onclicked" : "onchanged");
    }
}

static void cui__xml_draw_step_slider(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_step_slider_opts o;
    int *value = cui__xml_slot_int(doc, n);
    const char *label = cui__xml_label(doc, n);
    int lo = (int)cui__xml_attr_num(doc, n, "min", 0);
    int hi = (int)cui__xml_attr_num(doc, n, "max", 4);
    int flags;
    memset(&o, 0, sizeof o);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.value = cui__xml_attr_text(doc, n, "valuetext", NULL);
    o.tool = cui__xml_icon(doc, n, NULL);
    o.tool_tooltip = cui__xml_attr_text(doc, n, "tooltip", NULL);
    o.tool_on = cui__xml_attr_bool(doc, n, "toolon", false);
    if (!n->seen) *value = (int)cui__xml_attr_num(doc, n, "value", *value);
    if (hi <= lo) hi = lo + 1;
    if (disabled) cui_push_disabled(ui, true);
    flags = cui_step_slider_ex(ui, label && *label ? label : n->key, value, lo, hi, &o);
    if (disabled) cui_pop_disabled(ui);
    if (flags) {
        n->reported = flags;
        cui__xml_fire(doc, n, flags & CUI_WELL_ACTION ? "onclicked" : "onchanged");
    }
}

/* A palette and its locks live in the element: colors, what is locked, and the
 * seed the shuffle moves on. */
typedef struct {
    cui_color colors[16];
    bool locked[16];
    int count;
    uint32_t seed;
    bool init;
} cui__xml_palette_state;

static void cui__xml_draw_palette_strip(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_palette_strip_opts o;
    cui__xml_palette_state *st = n->cache;
    const char *label = cui__xml_label(doc, n);
    int flags;
    if (!st) {
        char **entries = cui__frame_alloc(ui, sizeof *entries * 16);
        char **locked = cui__frame_alloc(ui, sizeof *locked * 16);
        int count, locks, i;
        if (!entries || !locked) return;
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "colors", NULL), '|', entries, 16);
        locks = cui__xml_split(ui, cui__xml_attr_text(doc, n, "locked", NULL), ',', locked, 16);
        for (i = 0; i < count; i++) {
            unsigned v = (unsigned)strtoul(entries[i], NULL, 16);
            st->colors[i].r = (uint8_t)((v >> 16) & 0xFF);
            st->colors[i].g = (uint8_t)((v >> 8) & 0xFF);
            st->colors[i].b = (uint8_t)(v & 0xFF);
            st->colors[i].a = 255;
            st->locked[i] = i < locks && atoi(locked[i]) != 0;
        }
        st->count = count;
        st->seed = (uint32_t)cui__xml_attr_num(doc, n, "seed", 0);
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    if (!st->count) return;
    memset(&o, 0, sizeof o);
    o.seed = &st->seed;
    o.value = cui__xml_attr_text(doc, n, "valuetext", NULL);
    o.no_shuffle = cui__xml_attr_bool(doc, n, "noshuffle", false);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    if (disabled) cui_push_disabled(ui, true);
    flags = cui_palette_strip_ex(ui, label && *label ? label : n->key, st->colors, st->locked, st->count, &o);
    if (disabled) cui_pop_disabled(ui);
    if (flags) {
        n->reported = flags;
        cui__xml_fire(doc, n, flags & CUI_WELL_ACTION ? "onclicked" : "onchanged");
    }
}

static void cui__xml_draw_aspect_picker(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_aspect_picker_opts o;
    int *index = cui__xml_slot_int(doc, n);
    const char *label = cui__xml_label(doc, n);
    memset(&o, 0, sizeof o);
    o.custom = cui__xml_attr_bool(doc, n, "custom", false);
    o.custom_label = cui__xml_attr_text(doc, n, "customlabel", NULL);
    o.base = (int)cui__xml_attr_num(doc, n, "base", 1080);
    o.value = cui__xml_attr_text(doc, n, "valuetext", NULL);
    if (!n->seen) *index = (int)cui__xml_attr_num(doc, n, "value", *index);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_aspect_picker_ex(ui, label && *label ? label : n->key, index, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_option_action(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_option_action_opts o;
    char **items = cui__frame_alloc(ui, sizeof *items * 8);
    int *index = cui__xml_slot_int(doc, n);
    const char *label = cui__xml_label(doc, n);
    int count, flags;
    if (!items) return;
    count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "items", NULL), '|', items, 8);
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.tooltip = cui__xml_attr_text(doc, n, "tooltip", NULL);
    o.action_on = cui__xml_attr_bool(doc, n, "actionon", false);
    o.action_disabled = cui__xml_attr_bool(doc, n, "actiondisabled", false);
    o.value = cui__xml_attr_text(doc, n, "valuetext", NULL);
    if (!n->seen) *index = (int)cui__xml_attr_num(doc, n, "value", *index);
    if (disabled) cui_push_disabled(ui, true);
    flags = cui_option_action_ex(ui, label && *label ? label : n->key, (const char *const *)items, count, index,
                                 cui__xml_icon(doc, n, "circle"), &o);
    if (disabled) cui_pop_disabled(ui);
    if (flags) {
        n->reported = flags;
        cui__xml_fire(doc, n, flags & CUI_WELL_ACTION ? "onclicked" : "onchanged");
    }
}

static void cui__xml_draw_seed_field(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_seed_field_opts o;
    int *seed = cui__xml_slot_int(doc, n);
    bool *locked = cui__xml_slot_bool(doc, n);
    const char *label = cui__xml_label(doc, n);
    int flags;
    memset(&o, 0, sizeof o);
    o.value = cui__xml_attr_text(doc, n, "valuetext", NULL);
    o.random_text = cui__xml_attr_text(doc, n, "randomtext", NULL);
    o.fixed_text = cui__xml_attr_text(doc, n, "fixedtext", NULL);
    if (!n->seen) {
        *seed = (int)cui__xml_attr_num(doc, n, "value", *seed);
        *locked = cui__xml_attr_bool(doc, n, "locked", false);
    }
    if (disabled) cui_push_disabled(ui, true);
    flags = cui_seed_field_ex(ui, label && *label ? label : n->key, (uint32_t *)seed, locked, &o);
    if (disabled) cui_pop_disabled(ui);
    if (flags) {
        n->reported = flags;
        cui__xml_fire(doc, n, flags & CUI_WELL_ACTION ? "onclicked" : "onchanged");
    }
}

/* ---- motion: loaders, text effects, the dock, tiles, tabs, the player ---- */

static void cui__xml_text_fx_opts(cui_xml *doc, cui_xml_node *n, cui_text_fx_opts *o) {
    memset(o, 0, sizeof *o);
    o->font = cui__xml_font_of(doc, n, CUI_FONT_BODY);
    o->size = cui__xml_attr_num(doc, n, "size", 0);
    o->speed = cui__xml_attr_num(doc, n, "speed", 0);
    o->delay = cui__xml_attr_num(doc, n, "delay", 0);
    o->caret = cui__xml_attr_bool(doc, n, "caret", false);
    o->nowrap = cui__xml_attr_bool(doc, n, "nowrap", false);
}

static void cui__xml_draw_loader(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    static const char *const styles[] = { "dots", "bars", "pulse", "orbit", "grid" };
    cui_loader_opts o;
    memset(&o, 0, sizeof o);
    o.style = (cui_loader_style)cui__xml_choice_of(doc, n, "style", styles, 5, 0);
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    cui_loader_ex(ui, &o);
}

static void cui__xml_draw_shimmer_text(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_text_fx_opts o;
    cui__xml_text_fx_opts(doc, n, &o);
    cui_shimmer_text_ex(ui, cui__xml_label(doc, n), &o);
}

static void cui__xml_draw_text_reveal(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_text_fx_opts o;
    int replay = (int)cui__xml_attr_num(doc, n, "replay", 0);
    cui__xml_text_fx_opts(doc, n, &o);
    if (replay) cui_push_id_int(ui, replay);
    cui_text_reveal_ex(ui, n->key, cui__xml_label(doc, n), &o);
    if (replay) cui_pop_id(ui);
}

static void cui__xml_draw_word_rotate(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_text_fx_opts o;
    char **words = cui__frame_alloc(ui, sizeof *words * 16);
    int count;
    if (!words) return;
    count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "words", NULL), '|', words, 16);
    if (!count) return;
    cui__xml_text_fx_opts(doc, n, &o);
    cui_word_rotate_ex(ui, n->key, (const char *const *)words, count, cui__xml_attr_num(doc, n, "interval", 2), &o);
}

static void cui__xml_draw_gradient_text(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_gradient_text_opts o;
    memset(&o, 0, sizeof o);
    o.font = cui__xml_font_of(doc, n, CUI_FONT_BODY);
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.from = cui__xml_attr_color(doc, n, "from", (cui_color){ 0, 0, 0, 0 });
    o.to = cui__xml_attr_color(doc, n, "to", (cui_color){ 0, 0, 0, 0 });
    o.animate = cui__xml_attr_bool(doc, n, "animate", false);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FIT);
    cui_gradient_text_ex(ui, cui__xml_label(doc, n), &o);
}

static void cui__xml_draw_spotlight(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_spotlight_opts o;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    o.height = cui__xml_attr_size(doc, n, "height", (cui_size){ 0 });
    o.pad = cui__xml_attr_num(doc, n, "pad", 0);
    o.gap = cui__xml_attr_num(doc, n, "gap", 0);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    o.beam = cui__xml_attr_bool(doc, n, "beam", false);
    if (disabled) cui_push_disabled(ui, true);
    cui_spotlight_begin(ui, n->key, &o);
    cui__xml_draw_kids(doc, n, ui, disabled);
    cui_spotlight_end(ui);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_dock(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_dock_opts o;
    int i;
    memset(&o, 0, sizeof o);
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.magnify = cui__xml_attr_num(doc, n, "magnify", 0);
    if (disabled) cui_push_disabled(ui, true);
    cui_dock_begin(ui, n->key, &o);
    for (i = 0; i < n->kid_count; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment) continue;
        if (kid->tag_id == CUI__XML_TAG_DOCKSEPARATOR) {
            cui_dock_separator(ui);
            continue;
        }
        if (kid->tag_id != CUI__XML_TAG_DOCKITEM || !kid->shown) {
            cui__xml_draw(doc, kid, ui, disabled);
            continue;
        }
        {
            cui_dock_item_opts d;
            memset(&d, 0, sizeof d);
            d.color = cui__xml_attr_color(doc, kid, "color", (cui_color){ 0, 0, 0, 0 });
            d.running = cui__xml_attr_bool(doc, kid, "running", false);
            d.badge = (int)cui__xml_attr_num(doc, kid, "badge", 0);
            if (cui_dock_item_ex(ui, cui__xml_icon(doc, kid, "circle"), cui__xml_label(doc, kid), &d)) {
                kid->reported = 0;
                cui__xml_fire(doc, kid, "onclicked");
            }
        }
    }
    cui_dock_end(ui);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_control_tile(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_tile_opts o;
    bool *on = cui__xml_slot_bool(doc, n);
    memset(&o, 0, sizeof o);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.detail = cui__xml_attr_text(doc, n, "detail", NULL);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    o.wide = cui__xml_attr_bool(doc, n, "wide", false);
    if (!n->seen) *on = cui__xml_attr_bool(doc, n, "on", false);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_control_tile_ex(ui, cui__xml_label(doc, n), on, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_tab_bar(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_tab_bar_opts o;
    cui_tab_bar_item *items = cui__frame_alloc(ui, sizeof *items * 8);
    int *selected = cui__xml_slot_int(doc, n);
    int count = 0, i;
    if (!items) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    o.labels_on_selected_only = cui__xml_attr_bool(doc, n, "labelsonselectedonly", false);
    for (i = 0; i < n->kid_count && count < 8; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_TABITEM) continue;
        items[count].icon = cui__xml_icon(doc, kid, "circle");
        items[count].label = cui__xml_label(doc, kid);
        items[count].badge = (int)cui__xml_attr_num(doc, kid, "badge", 0);
        count++;
    }
    if (!count) return;
    if (!n->seen) *selected = (int)cui__xml_attr_num(doc, n, "selected", *selected);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_tab_bar_ex(ui, n->key, items, count, selected, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* The player's own state: what is playing, where it is, and its artwork tile. */
typedef struct {
    cui_now_playing np;
    bool init;
} cui__xml_player_state;

static void cui__xml_draw_now_playing(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_now_playing_opts o;
    cui__xml_player_state *st = n->cache;
    int events;
    if (!st) {
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        st->np.title = cui__xml_attr_text(doc, n, "title", "");
        st->np.artist = cui__xml_attr_text(doc, n, "artist", "");
        st->np.art_color = cui__xml_attr_color(doc, n, "artcolor", (cui_color){ 0, 0, 0, 0 });
        st->np.position = cui__xml_attr_num(doc, n, "position", 0);
        st->np.duration = cui__xml_attr_num(doc, n, "duration", 0);
        st->np.playing = cui__xml_attr_bool(doc, n, "playing", false);
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    o.compact = cui__xml_attr_bool(doc, n, "compact", false);
    if (disabled) cui_push_disabled(ui, true);
    events = cui_now_playing_ex(ui, n->key, &st->np, &o);
    if (disabled) cui_pop_disabled(ui);
    if (st->np.playing) {
        st->np.position += cui_delta_time(ui);
        if (st->np.position > st->np.duration) st->np.position = 0;
    }
    if (events) {
        n->reported = events;
        cui__xml_fire(doc, n, "onchanged");
    }
}

static void cui__xml_draw_waveform(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_waveform_opts o;
    const float *peaks = NULL;
    int count = cui__xml_attr_numbers(doc, n, "values", &peaks);
    float *progress = cui__xml_slot_float(doc, n);
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.bar = cui__xml_attr_num(doc, n, "bar", 0);
    o.gap = cui__xml_attr_num(doc, n, "gap", 0);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    if (count < 1) return;
    if (!n->seen) *progress = cui__xml_attr_num(doc, n, "progress", *progress);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_waveform_ex(ui, n->key, peaks, count, progress, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* ---- more charts: parts, cells, nodes and flows -------------------------- */

/* <Part Text="Layout" Value="55"/> children, as the charts that take items
 * read them. */
static int cui__xml_parts(cui_xml *doc, cui_xml_node *n, cui_viz_item *out, int cap) {
    int count = 0, i;
    for (i = 0; i < n->kid_count && count < cap; i++) {
        cui_xml_node *kid = n->kids[i];
        const char *label;
        if (kid->comment || kid->tag_id != CUI__XML_TAG_PART) continue;
        label = cui__xml_attr_text(doc, kid, "name", NULL);
        if (!label || !*label) label = cui__xml_label(doc, kid);
        out[count].label = label;
        out[count].value = cui__xml_attr_num(doc, kid, "value", 0);
        out[count].color = cui__xml_attr_color(doc, kid, "color", (cui_color){ 0, 0, 0, 0 });
        count++;
    }
    return count;
}

static void cui__xml_draw_treemap(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_treemap_opts o;
    cui_viz_item *items = cui__frame_alloc(ui, sizeof *items * 24);
    int count;
    if (!items) return;
    count = cui__xml_parts(doc, n, items, 24);
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    cui_treemap_ex(ui, n->key, items, count, &o);
}

static void cui__xml_draw_funnel(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_funnel_opts o;
    cui_viz_item *items = cui__frame_alloc(ui, sizeof *items * 12);
    int count;
    if (!items) return;
    count = cui__xml_parts(doc, n, items, 12);
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    cui_funnel_ex(ui, n->key, items, count, &o);
}

static void cui__xml_draw_split_bar(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_split_bar_opts o;
    cui_viz_item *items = cui__frame_alloc(ui, sizeof *items * 12);
    int count;
    if (!items) return;
    count = cui__xml_parts(doc, n, items, 12);
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.no_legend = cui__xml_attr_bool(doc, n, "nolegend", false);
    cui_split_bar_ex(ui, n->key, items, count, &o);
}

static void cui__xml_draw_waffle(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_waffle_opts o;
    cui_viz_item *items = cui__frame_alloc(ui, sizeof *items * 12);
    int count;
    if (!items) return;
    count = cui__xml_parts(doc, n, items, 12);
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.cell = cui__xml_attr_num(doc, n, "cell", 0);
    o.columns = (int)cui__xml_attr_num(doc, n, "columns", 0);
    o.rows = (int)cui__xml_attr_num(doc, n, "rows", 0);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    cui_waffle_ex(ui, n->key, items, count, &o);
}

static void cui__xml_draw_bar_list(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_bar_list_opts o;
    cui_viz_item *items = cui__frame_alloc(ui, sizeof *items * 24);
    int count;
    if (!items) return;
    count = cui__xml_parts(doc, n, items, 24);
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.max_rows = (int)cui__xml_attr_num(doc, n, "maxrows", 0);
    o.keep_order = cui__xml_attr_bool(doc, n, "keeporder", false);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    cui_bar_list_ex(ui, n->key, items, count, &o);
}

static void cui__xml_draw_activity_rings(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_rings_opts o;
    cui_viz_item *items = cui__frame_alloc(ui, sizeof *items * 8);
    int count;
    if (!items) return;
    count = cui__xml_parts(doc, n, items, 8);
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.thickness = cui__xml_attr_num(doc, n, "thickness", 0);
    o.center = cui__xml_attr_text(doc, n, "center", NULL);
    o.caption = cui__xml_attr_text(doc, n, "caption", NULL);
    o.legend = cui__xml_attr_bool(doc, n, "legend", false);
    cui_activity_rings_ex(ui, n->key, items, count, &o);
}

static void cui__xml_draw_heatmap(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_heatmap_opts o;
    const float *values = NULL;
    int rows = (int)cui__xml_attr_num(doc, n, "rows", 0);
    int cols = (int)cui__xml_attr_num(doc, n, "cols", 0);
    int count = cui__xml_attr_numbers(doc, n, "values", &values);
    int row_count = 0, col_count = 0;
    const char **row_labels = NULL;
    const char **col_labels = NULL;
    memset(&o, 0, sizeof o);
    if (rows < 1 || cols < 1 || count < rows * cols) return;
    /* Two lists on one element: cui__xml_attr_list keeps one per node, so these
     * are split into frame memory instead. */
    {
        char **list = cui__frame_alloc(ui, sizeof *list * 64);
        if (list) row_count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "rowlabels", NULL), '|', list, 64);
        row_labels = (const char **)list;
        list = cui__frame_alloc(ui, sizeof *list * 64);
        if (list) col_count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "collabels", NULL), '|', list, 64);
        col_labels = (const char **)list;
    }
    o.row_labels = row_count == rows ? row_labels : NULL;
    o.col_labels = col_count == cols ? col_labels : NULL;
    /* Labels every Nth row or column, the way the C examples pass an array with
     * holes: the others get none. */
    {
        int row_step = (int)cui__xml_attr_num(doc, n, "rowlabelstep", 0);
        int col_step = (int)cui__xml_attr_num(doc, n, "collabelstep", 0);
        if (row_step > 1 && row_labels && row_count && (rows + row_step - 1) / row_step == row_count) {
            const char **spread = cui__frame_alloc(ui, sizeof *spread * (size_t)rows);
            if (spread) {
                int k;
                for (k = 0; k < rows; k++) spread[k] = k % row_step == 0 ? row_labels[k / row_step] : NULL;
                o.row_labels = spread;
            }
        }
        if (col_step > 1 && col_labels && col_count && (cols + col_step - 1) / col_step == col_count) {
            const char **spread = cui__frame_alloc(ui, sizeof *spread * (size_t)cols);
            if (spread) {
                int k;
                for (k = 0; k < cols; k++) spread[k] = k % col_step == 0 ? col_labels[k / col_step] : NULL;
                o.col_labels = spread;
            }
        }
    }
    o.cell = cui__xml_attr_num(doc, n, "cell", 0);
    o.min = cui__xml_attr_num(doc, n, "min", 0);
    o.max = cui__xml_attr_num(doc, n, "max", 0);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    cui_heatmap_ex(ui, n->key, values, rows, cols, &o);
}

static void cui__xml_draw_calendar_heatmap(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_calendar_heatmap_opts o;
    const float *values = NULL;
    int days = (int)cui__xml_attr_num(doc, n, "days", 0);
    int count = cui__xml_attr_numbers(doc, n, "values", &values);
    cui_date start;
    memset(&o, 0, sizeof o);
    if (days < 1 || count < days) return;
    o.cell = cui__xml_attr_num(doc, n, "cell", 0);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.unit = cui__xml_attr_text(doc, n, "unit", NULL);
    {
        const char *from = cui__xml_attr_text(doc, n, "start", NULL);
        if (from && *from) start = cui__xml_date_value(from);
        else start = cui_date_from_days(cui_date_to_days(cui_date_today()) - (days - 1));
    }
    cui_calendar_heatmap_ex(ui, n->key, start, values, days, &o);
}

static cui_gauge_style cui__xml_gauge_style_of(cui_xml *doc, cui_xml_node *n) {
    static const char *const names[] = { "auto", "arc", "segments", "ring" };
    return (cui_gauge_style)cui__xml_choice_of(doc, n, "style", names, 4, 0);
}

/* Bands as "to:tone:name" entries, e.g. "60:success:Normal|85:warning:Busy". */
static int cui__xml_gauge_bands(cui_ctx *ui, const char *text, cui_gauge_band *out, int cap) {
    static const char *const tones[] = { "neutral", "info", "success", "warning", "danger", "violet", "teal", "rose", "accent" };
    char **entries = cui__frame_alloc(ui, sizeof *entries * 16);
    int count, i;
    if (!entries) return 0;
    count = cui__xml_split(ui, text, '|', entries, 16);
    if (count > cap) count = cap;
    for (i = 0; i < count; i++) {
        char *fields[3];
        int field_count = cui__xml_split(ui, entries[i], ':', fields, 3);
        int tone = 0, k;
        out[i].to = field_count > 0 ? (float)atof(fields[0]) : 0;
        if (field_count > 1) {
            for (k = 0; k < 9; k++)
                if (!strcmp(fields[1], tones[k])) tone = k;
        }
        out[i].tone = (cui_tone)tone;
        out[i].name = field_count > 2 && *fields[2] ? fields[2] : NULL;
    }
    return count;
}

static void cui__xml_draw_radial_gauge(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_radial_gauge_opts o;
    cui_gauge_band *bands = cui__frame_alloc(ui, sizeof *bands * 16);
    memset(&o, 0, sizeof o);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.unit = cui__xml_attr_text(doc, n, "unit", NULL);
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.sweep = cui__xml_attr_num(doc, n, "sweep", 0);
    o.ticks = (int)cui__xml_attr_num(doc, n, "ticks", 0);
    o.target = cui__xml_attr_num(doc, n, "target", 0);
    o.show_target = cui__xml_attr_bool(doc, n, "showtarget", false);
    o.style = cui__xml_gauge_style_of(doc, n);
    o.detail = cui__xml_attr_text(doc, n, "detail", NULL);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    if (bands) {
        o.band_count = cui__xml_gauge_bands(ui, cui__xml_attr_text(doc, n, "bands", NULL), bands, 16);
        o.bands = o.band_count ? bands : NULL;
    }
    cui_radial_gauge_ex(ui, n->key, cui__xml_attr_num(doc, n, "value", 0), cui__xml_attr_num(doc, n, "min", 0),
                        cui__xml_attr_num(doc, n, "max", 100), &o);
}

static void cui__xml_draw_radar(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_radar_opts o;
    cui_series *series = cui__frame_alloc(ui, sizeof *series * 8);
    const char **axes = NULL;
    int axis_count = 0, count = 0, i;
    memset(&o, 0, sizeof o);
    if (!series) return;
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.max = cui__xml_attr_num(doc, n, "max", 0);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.no_legend = cui__xml_attr_bool(doc, n, "nolegend", false);
    if (cui__xml_attr_raw(n, "axes")) axes = cui__xml_attr_list(doc, n, "axes", &axis_count, false);
    if (!axes || !axis_count) return;
    for (i = 0; i < n->kid_count && count < 8; i++) {
        cui_xml_node *kid = n->kids[i];
        const float *values = NULL;
        int value_count;
        if (kid->comment || kid->tag_id != CUI__XML_TAG_SERIES) continue;
        value_count = cui__xml_attr_numbers(doc, kid, "values", &values);
        if (value_count < axis_count) continue;
        series[count].name = cui__xml_label(doc, kid);
        series[count].values = values;
        series[count].count = value_count;
        series[count].color = cui__xml_attr_color(doc, kid, "color", (cui_color){ 0, 0, 0, 0 });
        count++;
    }
    if (!count) return;
    cui_radar_chart_ex(ui, n->key, series, count, axes, axis_count, &o);
}

static void cui__xml_draw_scatter(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_scatter_opts o;
    cui_scatter_series *series = cui__frame_alloc(ui, sizeof *series * 8);
    int count = 0, i;
    memset(&o, 0, sizeof o);
    if (!series) return;
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.x_label = cui__xml_attr_text(doc, n, "xlabel", NULL);
    o.y_label = cui__xml_attr_text(doc, n, "ylabel", NULL);
    o.y_format = cui__xml_attr_text(doc, n, "yformat", NULL);
    for (i = 0; i < n->kid_count && count < 8; i++) {
        cui_xml_node *kid = n->kids[i];
        const char *points;
        cui_vec2 *vecs;
        float *sizes;
        char **pairs;
        int pair_count, k, made = 0;
        if (kid->comment || kid->tag_id != CUI__XML_TAG_SERIES) continue;
        points = cui__xml_attr_text(doc, kid, "points", NULL);
        if (!points || !*points) continue;
        pairs = cui__frame_alloc(ui, sizeof *pairs * 256);
        vecs = cui__frame_alloc(ui, sizeof *vecs * 256);
        sizes = cui__frame_alloc(ui, sizeof *sizes * 256);
        if (!pairs || !vecs || !sizes) break;
        pair_count = cui__xml_split(ui, points, ';', pairs, 256);
        for (k = 0; k < pair_count && made < 256; k++) {
            double x = 0, y = 0;
            if (sscanf(pairs[k], "%lf,%lf", &x, &y) != 2) continue;
            vecs[made].x = (float)x;
            vecs[made].y = (float)y;
            made++;
        }
        if (!made) continue;
        {
            const float *given = NULL;
            int size_count = cui__xml_attr_numbers(doc, kid, "sizes", &given);
            series[count].sizes = size_count >= made ? given : NULL;
        }
        series[count].name = cui__xml_label(doc, kid);
        series[count].points = vecs;
        series[count].count = made;
        series[count].color = cui__xml_attr_color(doc, kid, "color", (cui_color){ 0, 0, 0, 0 });
        count++;
    }
    if (!count) return;
    cui_scatter_chart_ex(ui, n->key, series, count, &o);
}

/* Nodes named by |, then the flows as "from-to:value" entries. */
static void cui__xml_draw_sankey(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_sankey_opts o;
    char **nodes = cui__frame_alloc(ui, sizeof *nodes * 32);
    char **flows = cui__frame_alloc(ui, sizeof *flows * 32);
    cui_sankey_link *links = cui__frame_alloc(ui, sizeof *links * 32);
    int node_count, flow_count, made = 0, i;
    memset(&o, 0, sizeof o);
    if (!nodes || !flows || !links) return;
    node_count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "nodes", NULL), '|', nodes, 32);
    flow_count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "links", NULL), '|', flows, 32);
    if (node_count < 1 || flow_count < 1) return;
    for (i = 0; i < flow_count; i++) {
        int from = 0, to = 0;
        float value = 0;
        if (sscanf(flows[i], "%d-%d:%f", &from, &to, &value) != 3) continue;
        links[made].from = from;
        links[made].to = to;
        links[made].value = value;
        made++;
    }
    if (!made) return;
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.node_width = cui__xml_attr_num(doc, n, "nodewidth", 0);
    cui_sankey_ex(ui, n->key, (const char *const *)nodes, node_count, links, made, &o);
}

/* ---- planning: a kanban board, a week, a gantt and a month --------------- */

typedef struct {
    cui_xml_node *node;
    cui_xml_node *column;
} cui__xml_kanban_card;

typedef struct {
    int count;
    cui__xml_kanban_card cards[];
} cui__xml_kanban_board;

static cui__xml_kanban_board *cui__xml_kanban_cards(cui_xml_node *n) {
    int count = 0;
    for (int i = 0; i < n->kid_count; i++) {
        cui_xml_node *column = n->kids[i];
        if (column->tag_id != CUI__XML_TAG_KANBANCOLUMN) continue;
        count += column->kid_count;
    }
    cui__xml_kanban_board *board = cui__xml_alloc(sizeof *board + (size_t)count * sizeof board->cards[0]);
    if (!board) return NULL;
    board->count = 0;
    for (int i = 0; i < n->kid_count; i++) {
        cui_xml_node *column = n->kids[i];
        if (column->tag_id != CUI__XML_TAG_KANBANCOLUMN) continue;
        for (int k = 0; k < column->kid_count; k++) {
            cui_xml_node *card = column->kids[k];
            board->cards[board->count++] = (cui__xml_kanban_card){ card, column };
        }
    }
    return board;
}

static void cui__xml_kanban_move(cui_xml *doc, cui__xml_kanban_board *board, const cui_kanban_move *move,
                                  cui_xml_node *from_column, cui_xml_node *to_column) {
    int from = -1, to = board->count, seen = 0;
    for (int i = 0; i < board->count; i++) {
        cui__xml_kanban_card *entry = &board->cards[i];
        if (entry->node->tag_id != CUI__XML_TAG_KANBANCARD || !entry->node->shown) continue;
        if (entry->column == from_column && (int)cui__xml_attr_num(doc, entry->node, "card", 0) == move->card) {
            from = i;
            continue;
        }
        if (entry->column == to_column && seen <= move->to_index) {
            to = seen == move->to_index ? i : i + 1;
            seen++;
        }
    }
    if (from < 0) return;
    cui__xml_kanban_card card = { board->cards[from].node, to_column };
    if (to > from) to--;
    if (to > from) memmove(&board->cards[from], &board->cards[from + 1], (size_t)(to - from) * sizeof card);
    if (to < from) memmove(&board->cards[to + 1], &board->cards[to], (size_t)(from - to) * sizeof card);
    board->cards[to] = card;
}

static void cui__xml_draw_kanban(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_kanban_move move;
    cui__xml_kanban_board *board = n->cache;
    cui_xml_node **columns = cui__frame_alloc(ui, (size_t)n->kid_count * sizeof *columns);
    int count = 0;
    if (!columns) return;
    if (!board) {
        board = cui__xml_kanban_cards(n);
        if (!board) return;
        n->cache = board;
        n->cache_free = cui__xml_free_cache;
    }
    if (disabled) cui_push_disabled(ui, true);
    cui_kanban_begin(ui, n->key);
    for (int i = 0; i < n->kid_count; i++) {
        cui_xml_node *column = n->kids[i];
        cui_kanban_column_opts o;
        int cards = 0;
        if (column->tag_id != CUI__XML_TAG_KANBANCOLUMN || !column->shown) {
            cui__xml_draw(doc, column, ui, disabled);
            continue;
        }
        columns[count++] = column;
        for (int k = 0; k < board->count; k++)
            if (board->cards[k].column == column && board->cards[k].node->shown &&
                board->cards[k].node->tag_id == CUI__XML_TAG_KANBANCARD) cards++;
        memset(&o, 0, sizeof o);
        o.dot = cui__xml_attr_color(doc, column, "dot", (cui_color){ 0, 0, 0, 0 });
        o.limit = (int)cui__xml_attr_num(doc, column, "limit", 0);
        o.width = cui__xml_attr_num(doc, column, "width", 0);
        if (cui_kanban_column_begin_ex(ui, cui__xml_label(doc, column), cards, &o)) {
            column->reported = 0;
            cui__xml_fire(doc, column, "onclicked");
        }
        for (int k = 0; k < board->count; k++) {
            cui_xml_node *card = board->cards[k].node;
            bool clicked;
            if (board->cards[k].column != column) continue;
            if (card->tag_id != CUI__XML_TAG_KANBANCARD || !card->shown) {
                cui__xml_draw(doc, card, ui, disabled);
                continue;
            }
            clicked = cui_kanban_card_begin(ui, (int)cui__xml_attr_num(doc, card, "card", 0));
            cui__xml_record_id(card, ui, cui_last_id(ui));
            if (clicked) {
                card->reported = 0;
                cui__xml_fire(doc, card, "onclicked");
            }
            cui__xml_draw_kids(doc, card, ui, disabled);
            cui_kanban_card_end(ui);
        }
        cui_kanban_column_end(ui);
    }
    if (cui_kanban_end(ui, &move)) {
        if (cui__xml_attr_bool(doc, n, "automove", false) &&
            move.from_column >= 0 && move.from_column < count && move.to_column >= 0 && move.to_column < count)
            cui__xml_kanban_move(doc, board, &move, columns[move.from_column], columns[move.to_column]);
        n->reported = move.card;
        cui__xml_fire(doc, n, "onchanged");
    }
    if (disabled) cui_pop_disabled(ui);
}

/* What a week, a gantt and a month hold lives in the element: the user drags
 * them, and a document has no other place to keep the result. */
typedef struct {
    cui_cal_event events[32];
    cui_gantt_task tasks[32];
    cui_month_event month[32];
    cui_date week, origin, month_start, picked;
    int count;
    bool init;
} cui__xml_plan_state;

static cui_date cui__xml_monday_of_today(void) {
    int td = cui_date_to_days(cui_date_today());
    int monday = td - ((td + 3) % 7 + 7) % 7;
    return cui_date_from_days(monday);
}

static void cui__xml_draw_week_calendar(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_week_opts o;
    cui__xml_plan_state *st = n->cache;
    int i, clicked;
    if (!st) {
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        st->week = cui__xml_monday_of_today();
        for (i = 0; i < n->kid_count && st->count < 32; i++) {
            cui_xml_node *kid = n->kids[i];
            if (kid->comment || kid->tag_id != CUI__XML_TAG_CALEVENT) continue;
            st->events[st->count].title = cui__xml_label(doc, kid);
            st->events[st->count].detail = cui__xml_attr_text(doc, kid, "detail", NULL);
            st->events[st->count].day = (int)cui__xml_attr_num(doc, kid, "day", 0);
            st->events[st->count].start = cui__xml_attr_num(doc, kid, "start", 0);
            st->events[st->count].end = cui__xml_attr_num(doc, kid, "end", 0);
            st->events[st->count].tone = cui__xml_tone_of(doc, kid, CUI_TONE_NEUTRAL);
            st->count++;
        }
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    if (!st->count) return;
    memset(&o, 0, sizeof o);
    o.start_hour = cui__xml_attr_num(doc, n, "starthour", 0);
    o.end_hour = cui__xml_attr_num(doc, n, "endhour", 0);
    o.hour_height = cui__xml_attr_num(doc, n, "hourheight", 0);
    o.height = cui__xml_attr_size(doc, n, "height", CUI_FILL);
    o.read_only = cui__xml_attr_bool(doc, n, "readonly", false);
    if (disabled) cui_push_disabled(ui, true);
    clicked = cui_week_calendar_ex(ui, n->key, st->week, st->events, st->count, &o);
    if (disabled) cui_pop_disabled(ui);
    if (clicked >= 0) {
        n->reported = clicked;
        cui__xml_fire(doc, n, "onclicked");
    }
}

static void cui__xml_draw_gantt(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_gantt_opts o;
    cui__xml_plan_state *st = n->cache;
    int i, days = (int)cui__xml_attr_num(doc, n, "days", 0), clicked;
    if (!st) {
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        st->origin = cui_date_from_days(cui_date_to_days(cui__xml_monday_of_today()) - 7);
        for (i = 0; i < n->kid_count && st->count < 32; i++) {
            cui_xml_node *kid = n->kids[i];
            if (kid->comment || kid->tag_id != CUI__XML_TAG_TASK) continue;
            st->tasks[st->count].name = cui__xml_label(doc, kid);
            st->tasks[st->count].owner = cui__xml_attr_text(doc, kid, "owner", NULL);
            st->tasks[st->count].start = (int)cui__xml_attr_num(doc, kid, "start", 0);
            st->tasks[st->count].end = (int)cui__xml_attr_num(doc, kid, "end", 0);
            st->tasks[st->count].progress = cui__xml_attr_num(doc, kid, "progress", 0);
            st->tasks[st->count].tone = cui__xml_tone_of(doc, kid, CUI_TONE_NEUTRAL);
            st->tasks[st->count].depends_on = (int)cui__xml_attr_num(doc, kid, "dependson", -1);
            st->tasks[st->count].milestone = cui__xml_attr_bool(doc, kid, "milestone", false);
            st->tasks[st->count].state = CUI_GANTT_AUTO;
            st->count++;
        }
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    if (!st->count || days < 1) return;
    memset(&o, 0, sizeof o);
    o.day_width = cui__xml_attr_num(doc, n, "daywidth", 0);
    o.row_height = cui__xml_attr_num(doc, n, "rowheight", 0);
    o.height = cui__xml_attr_size(doc, n, "height", CUI_FILL);
    o.today = (int)cui__xml_attr_num(doc, n, "today", 0);
    o.read_only = cui__xml_attr_bool(doc, n, "readonly", false);
    if (disabled) cui_push_disabled(ui, true);
    clicked = cui_gantt_ex(ui, n->key, st->tasks, st->count, st->origin, days, &o);
    if (disabled) cui_pop_disabled(ui);
    if (clicked >= 0) {
        n->reported = clicked;
        cui__xml_fire(doc, n, "onclicked");
    }
}

static void cui__xml_draw_month_calendar(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_month_opts o;
    cui__xml_plan_state *st = n->cache;
    cui_month_result r;
    int i;
    if (!st) {
        cui_date now = cui_date_today();
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        st->month_start = (cui_date){ now.year, now.month, 1 };
        for (i = 0; i < n->kid_count && st->count < 32; i++) {
            cui_xml_node *kid = n->kids[i];
            int span;
            if (kid->comment || kid->tag_id != CUI__XML_TAG_MONTHEVENT) continue;
            span = (int)cui__xml_attr_num(doc, kid, "span", 0);
            st->month[st->count].title = cui__xml_label(doc, kid);
            st->month[st->count].start = (cui_date){ st->month_start.year, st->month_start.month,
                                                     (int)cui__xml_attr_num(doc, kid, "day", 1) };
            st->month[st->count].end = span > 0
                                           ? cui_date_from_days(cui_date_to_days(st->month[st->count].start) + span)
                                           : (cui_date){ 0, 0, 0 };
            st->month[st->count].time = cui__xml_attr_num(doc, kid, "time", 0);
            st->month[st->count].all_day = cui__xml_attr_bool(doc, kid, "allday", false);
            st->month[st->count].tone = cui__xml_tone_of(doc, kid, CUI_TONE_NEUTRAL);
            st->count++;
        }
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    if (!st->count) return;
    memset(&o, 0, sizeof o);
    o.selected = &st->picked;
    o.first_weekday = (int)cui__xml_attr_num(doc, n, "firstweekday", 0);
    o.cell_height = cui__xml_attr_num(doc, n, "cellheight", 0);
    o.hour12 = cui__xml_attr_bool(doc, n, "hour12", false);
    if (disabled) cui_push_disabled(ui, true);
    r = cui_month_calendar_ex(ui, n->key, &st->month_start, st->month, st->count, &o);
    if (disabled) cui_pop_disabled(ui);
    if (r.event >= 0) {
        n->reported = r.event;
        cui__xml_fire(doc, n, "onclicked");
    }
}

/* ---- voice: notes, waveforms and a spoken session ------------------------ */

/* One level per entry, read afresh: a voice moves with the frame. */
static int cui__xml_levels_of(cui_ctx *ui, const char *text, float *out, int cap) {
    int count = cui__xml_live_numbers(text, out, cap), i;
    for (i = 0; i < count; i++) if (out[i] > 1.0f) out[i] = -1.0f;   /* CUI_VOICE_SILENT passes as -1 */
    return count;
}

static int cui__xml_owners_of(cui_ctx *ui, const char *text, uint8_t *out, int cap) {
    float numbers[64];
    int count = cui__xml_live_numbers(text, numbers, cap < 64 ? cap : 64), i;
    (void)ui;
    for (i = 0; i < count; i++) out[i] = (uint8_t)(numbers[i] < 0 ? 0 : (int)numbers[i]);
    return count;
}

typedef struct {
    cui_voice_note note;
    float levels[256];
    int count;
    bool init;
} cui__xml_voice_note_state;

static void cui__xml_draw_voice_note(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_voice_note_opts o;
    cui__xml_voice_note_state *st = n->cache;
    int events;
    if (!st) {
        const float *values = NULL;
        int count;
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        count = cui__xml_attr_numbers(doc, n, "levels", &values);
        if (count > 256) count = 256;
        memcpy(st->levels, values, sizeof(st->levels[0]) * (size_t)count);
        st->count = count;
        st->note.levels = st->levels;
        st->note.count = count;
        st->note.duration = cui__xml_attr_num(doc, n, "duration", 0);
        st->note.position = cui__xml_attr_num(doc, n, "position", 0);
        st->note.speed = cui__xml_attr_num(doc, n, "speed", 1);
        st->note.playing = cui__xml_attr_bool(doc, n, "playing", false);
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    memset(&o, 0, sizeof o);
    o.mine = cui__xml_attr_bool(doc, n, "mine", false);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    if (disabled) cui_push_disabled(ui, true);
    events = cui_voice_note_ex(ui, n->key, &st->note, &o);
    if (disabled) cui_pop_disabled(ui);
    if (st->note.playing) {
        st->note.position += cui_delta_time(ui) * (st->note.speed > 0 ? st->note.speed : 1);
        if (st->note.position >= st->note.duration) {
            st->note.position = 0;
            st->note.playing = false;
        }
    }
    if (events) {
        n->reported = events;
        cui__xml_fire(doc, n, "onchanged");
    }
}

static void cui__xml_draw_voice_wave(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_voice_wave_opts o;
    float *levels = cui__frame_alloc(ui, sizeof *levels * 600);
    uint8_t *owners = cui__frame_alloc(ui, 600);
    float *progress = cui__xml_slot_float(doc, n);
    int count, owner_count = 0;
    if (!levels || !owners) return;
    count = cui__xml_levels_of(ui, cui__xml_attr_text(doc, n, "levels", NULL), levels, 600);
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.clip = cui__xml_attr_bool(doc, n, "clip", false);
    o.progress = cui__xml_attr_bool(doc, n, "seek", false) ? progress : NULL;
    o.owner = cui__xml_attr_bool(doc, n, "reply", false) ? CUI_VOICE_REPLY : CUI_VOICE_YOU;
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    o.pitch = cui__xml_attr_num(doc, n, "pitch", 0);
    o.step = cui__xml_attr_num(doc, n, "step", 0);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    if (cui__xml_attr_raw(n, "owners")) {
        owner_count = cui__xml_owners_of(ui, cui__xml_attr_text(doc, n, "owners", NULL), owners, count);
        if (owner_count == count) o.owners = owners;
    }
    if (!n->seen && o.progress) *progress = cui__xml_attr_num(doc, n, "progress", *progress);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_voice_wave_ex(ui, n->key, levels, count, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_voice_session(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    static const char *const states[] = { "idle", "listening", "thinking", "speaking", "muted" };
    cui_voice_session_opts o;
    float *levels = cui__frame_alloc(ui, sizeof *levels * 600);
    uint8_t *owners = cui__frame_alloc(ui, 600);
    bool *muted = cui__xml_slot_bool(doc, n);
    int count, owner_count = 0, flags;
    if (!levels || !owners) return;
    count = cui__xml_levels_of(ui, cui__xml_attr_text(doc, n, "levels", NULL), levels, 600);
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.transcript = cui__xml_attr_text(doc, n, "transcript", NULL);
    o.upcoming = cui__xml_attr_text(doc, n, "upcoming", NULL);
    o.speaker = cui__xml_attr_bool(doc, n, "reply", false) ? CUI_VOICE_REPLY : CUI_VOICE_YOU;
    o.you = cui__xml_attr_text(doc, n, "you", NULL);
    o.assistant = cui__xml_attr_text(doc, n, "assistant", NULL);
    o.muted = cui__xml_attr_bool(doc, n, "showmute", true) ? muted : NULL;
    o.no_end = cui__xml_attr_bool(doc, n, "noend", false);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    if (cui__xml_attr_raw(n, "owners")) {
        owner_count = cui__xml_owners_of(ui, cui__xml_attr_text(doc, n, "owners", NULL), owners, count);
        if (owner_count == count) o.owners = owners;
    }
    if (!n->seen) *muted = cui__xml_attr_bool(doc, n, "muted", false);
    if (disabled) cui_push_disabled(ui, true);
    flags = cui_voice_session_ex(ui, n->key, (cui_voice_state)cui__xml_choice_of(doc, n, "state", states, 5, 1),
                                 levels, count, &o);
    if (disabled) cui_pop_disabled(ui);
    if (flags) {
        n->reported = flags;
        cui__xml_fire(doc, n, "onchanged");
    }
}

/* ---- social: a divider, a typing line, a counter, comments, presence ---- */

static void cui__xml_draw_chat_divider(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_chat_divider(ui, cui__xml_label(doc, n));
}

static void cui__xml_draw_typing_indicator(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_typing_indicator(ui, cui__xml_label(doc, n));
}

static void cui__xml_draw_counter(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_counter_opts o;
    memset(&o, 0, sizeof o);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.font = cui__xml_font_of(doc, n, CUI_FONT_DISPLAY);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    o.group_thousands = cui__xml_attr_bool(doc, n, "groupthousands", false);
    cui_counter_ex(ui, n->key, cui__xml_attr_num(doc, n, "value", 0), &o);
}

/* A comment keeps its likes and its heart in the element: a click sticks. */
typedef struct {
    int likes;
    bool liked;
    bool init;
} cui__xml_comment_state;

static void cui__xml_draw_comment(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_comment_opts o;
    cui__xml_comment_state *st = n->cache;
    int flags;
    if (!st) {
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        st->likes = (int)cui__xml_attr_num(doc, n, "likes", 0);
        st->liked = cui__xml_attr_bool(doc, n, "liked", false);
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    memset(&o, 0, sizeof o);
    o.time = cui__xml_attr_text(doc, n, "time", NULL);
    o.depth = (int)cui__xml_attr_num(doc, n, "depth", 0);
    o.likes = &st->likes;
    o.liked = &st->liked;
    o.edited = cui__xml_attr_bool(doc, n, "edited", false);
    o.badge = cui__xml_attr_text(doc, n, "badge", NULL);
    flags = cui_comment_ex(ui, n->key, cui__xml_attr_text(doc, n, "author", ""), cui__xml_label(doc, n), &o);
    if (flags) {
        n->reported = flags;
        cui__xml_fire(doc, n, "onclicked");
    }
}

/* "#rrggbb", or a theme color name, in a list entry such as the presence's. */
static cui_color cui__xml_color_word(const char *text) {
    cui_color c = { 0, 0, 0, 0 };
    unsigned v = 0;
    size_t i;
    if (!text || text[0] != '#') return c;
    for (i = 1; text[i] && i <= 6; i++) {
        int d;
        int ch = (unsigned char)text[i];
        if (ch >= '0' && ch <= '9') d = ch - '0';
        else if (ch >= 'a' && ch <= 'f') d = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F') d = ch - 'A' + 10;
        else return c;
        v = (v << 4) | (unsigned)d;
    }
    if (i != 7) return c;
    c.r = (uint8_t)((v >> 16) & 0xFF);
    c.g = (uint8_t)((v >> 8) & 0xFF);
    c.b = (uint8_t)(v & 0xFF);
    c.a = 255;
    return c;
}

/* "Name:#rrggbb[:away]" entries, as the presence draws them. */
static void cui__xml_draw_presence(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_presence_user *users = cui__frame_alloc(ui, sizeof *users * 16);
    char **entries = cui__frame_alloc(ui, sizeof *entries * 16);
    int count, i;
    if (!users || !entries) return;
    count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "users", NULL), '|', entries, 16);
    if (!count) return;
    for (i = 0; i < count; i++) {
        char *fields[3];
        int field_count = cui__xml_split(ui, entries[i], ':', fields, 3);
        users[i].name = field_count > 0 ? fields[0] : "";
        users[i].color = field_count > 1 ? cui__xml_color_word(fields[1]) : (cui_color){ 0, 0, 0, 0 };
        users[i].idle = field_count > 2 && !strcmp(fields[2], "away");
    }
    cui_presence(ui, n->key, users, count, (int)cui__xml_attr_num(doc, n, "visible", count),
                 cui__xml_attr_num(doc, n, "size", 0));
}

/* "Label:Value:Delta[:lower]" entries, as the ticker scrolls them. */
static void cui__xml_draw_ticker(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_ticker_opts o;
    cui_ticker_item *items = cui__frame_alloc(ui, sizeof *items * 16);
    char **entries = cui__frame_alloc(ui, sizeof *entries * 16);
    int count, i;
    if (!items || !entries) return;
    count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "items", NULL), '|', entries, 16);
    if (!count) return;
    for (i = 0; i < count; i++) {
        char *fields[4];
        int field_count = cui__xml_split(ui, entries[i], ':', fields, 4);
        items[i].label = field_count > 0 ? fields[0] : "";
        items[i].value = field_count > 1 && *fields[1] ? fields[1] : NULL;
        items[i].delta = field_count > 2 ? (float)atof(fields[2]) : 0;
        items[i].lower_is_better = field_count > 3 && !strcmp(fields[3], "lower");
    }
    memset(&o, 0, sizeof o);
    o.speed = cui__xml_attr_num(doc, n, "speed", 0);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    cui_ticker_ex(ui, n->key, items, count, &o);
}

static void cui__xml_draw_card_stack(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_card_stack_opts o;
    int count = (int)cui__xml_attr_num(doc, n, "count", 0), depth = 0, i, direction;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", (cui_size){ 0 });
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.visible = (int)cui__xml_attr_num(doc, n, "visible", 0);
    if (count < 1) return;
    if (disabled) cui_push_disabled(ui, true);
    cui_card_stack_begin(ui, n->key, count, &o);
    for (i = 0; i < n->kid_count; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_CARDSTACKITEM || !kid->shown) {
            cui__xml_draw(doc, kid, ui, disabled);
            continue;
        }
        cui_card_stack_item_begin(ui, depth);
        cui__xml_draw_kids(doc, kid, ui, disabled);
        cui_card_stack_item_end(ui);
        depth++;
    }
    direction = cui_card_stack_end(ui);
    if (disabled) cui_pop_disabled(ui);
    if (direction) {
        n->reported = direction;
        cui__xml_fire(doc, n, "onchanged");
    }
}

/* ---- a filter bar and a tree table -------------------------------------- */

typedef struct {
    uint32_t selected[8];
    char search[128];
    bool init;
} cui__xml_filter_state;

/* <Field Text="Team" Icon="users" Options="Engine|Design|Platform"/> children;
 * what is chosen and what is typed stay in the element. */
static void cui__xml_draw_filter_bar(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_filter_field *fields = cui__frame_alloc(ui, sizeof *fields * 8);
    char **values = cui__frame_alloc(ui, sizeof *values * 8 * 32);
    cui__xml_filter_state *st = n->cache;
    int count = 0, i;
    if (!fields || !values) return;
    if (!st) {
        const char *selected = cui__xml_attr_text(doc, n, "selected", NULL);
        char **masks = cui__frame_alloc(ui, sizeof *masks * 8);
        int mask_count = masks ? cui__xml_split(ui, selected, ',', masks, 8) : 0;
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        for (i = 0; i < mask_count && i < 8; i++) st->selected[i] = (uint32_t)strtoul(masks[i], NULL, 10);
        {
            const char *text = cui__xml_attr_text(doc, n, "search", NULL);
            if (text) snprintf(st->search, sizeof st->search, "%s", text);
        }
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    for (i = 0; i < n->kid_count && count < 8; i++) {
        cui_xml_node *kid = n->kids[i];
        const char *options;
        if (kid->comment || kid->tag_id != CUI__XML_TAG_FIELD) continue;
        options = cui__xml_attr_text(doc, kid, "options", NULL);
        fields[count].name = cui__xml_label(doc, kid);
        fields[count].icon = cui__xml_icon(doc, kid, NULL);
        fields[count].options = (const char *const *)(values + count * 32);
        fields[count].option_count = cui__xml_split(ui, options, '|', values + count * 32, 32);
        if (fields[count].option_count < 1) continue;
        count++;
    }
    if (!count) return;
    if (disabled) cui_push_disabled(ui, true);
    if (cui_filter_bar(ui, n->key, fields, count, st->selected, st->search, sizeof st->search))
        cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

typedef struct {
    bool open[32];
    bool init[32];
} cui__xml_tree_state;

/* <TreeRow Text="src" Depth="0" Open="true" Icon="folder" Cells="42|1.2 MB"/>
 * children of <TreeTable Columns="Name:fill|Files:80|Size:100"/>: rows are drawn
 * while every row above them with a smaller Depth is open. */
static void cui__xml_draw_tree_table(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_table_column columns[8];
    cui__xml_tree_state *st = n->cache;
    char **specs = cui__frame_alloc(ui, sizeof *specs * 8);
    int column_count, i;
    if (!specs) return;
    column_count = cui__xml_split(ui, cui__xml_attr_text(doc, n, "columns", NULL), '|', specs, 8);
    if (column_count < 1) return;
    for (i = 0; i < column_count; i++) {
        char *parts[3];
        int part_count = cui__xml_split(ui, specs[i], ':', parts, 3);
        columns[i].title = part_count > 0 ? parts[0] : "";
        columns[i].width = part_count > 1 && strcmp(parts[1], "fill") ? CUI_PX((float)atof(parts[1])) : CUI_FILL;
        columns[i].align = part_count > 2 && !strcmp(parts[2], "right") ? CUI_TEXT_RIGHT : CUI_TEXT_LEFT;
        columns[i].sortable = false;
    }
    if (!st) {
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    if (disabled) cui_push_disabled(ui, true);
    if (cui_table_begin_ex(ui, n->key, columns, column_count, NULL)) {
        int depth_open[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
        int seen = 0;
        for (i = 0; i < n->kid_count; i++) {
            cui_xml_node *row = n->kids[i];
            int depth, d, visible = 1, has_children = 0;
            char **cells;
            int cell_count, k;
            if (row->comment || row->tag_id != CUI__XML_TAG_TREEROW) {
                cui__xml_draw(doc, row, ui, disabled);
                continue;
            }
            depth = (int)cui__xml_attr_num(doc, row, "depth", 0);
            if (depth < 0) depth = 0;
            if (depth > 7) depth = 7;
            for (d = 0; d < depth; d++)
                if (!depth_open[d]) visible = 0;
            if (seen < 32) {
                if (!st->init[seen]) {
                    st->open[seen] = cui__xml_attr_bool(doc, row, "open", false);
                    st->init[seen] = true;
                }
            }
            for (k = i + 1; k < n->kid_count; k++) {
                cui_xml_node *next = n->kids[k];
                if (next->comment || next->tag_id != CUI__XML_TAG_TREEROW) continue;
                if ((int)cui__xml_attr_num(doc, next, "depth", 0) > depth) has_children = 1;
                break;
            }
            if (visible) {
                cells = cui__frame_alloc(ui, sizeof *cells * 8);
                cell_count = cells ? cui__xml_split(ui, cui__xml_attr_text(doc, row, "cells", NULL), '|', cells, 8)
                                   : 0;
                cui_table_row(ui, false);
                cui_table_cell(ui);
                cui_tree_cell(ui, cui__xml_label(doc, row), depth, has_children,
                              has_children && seen < 32 ? &st->open[seen] : NULL, cui__xml_icon(doc, row, NULL));
                for (k = 0; k < cell_count; k++) cui_table_cell_text(ui, cells[k]);
                depth_open[depth] = has_children && seen < 32 ? st->open[seen] : 1;
            } else {
                depth_open[depth] = has_children && seen < 32 ? st->open[seen] : 1;
            }
            seen++;
        }
        cui_table_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
}

/* ---- developer tools: problems, pairs, jobs, a query, a schedule, keys --- */

/* <Diagnostic Text File Line Column Source Code Severity="error"/> children. */
static void cui__xml_draw_problems(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    static const char *const severities[] = { "error", "warning", "info", "hint" };
    cui_problems_opts o;
    cui_diagnostic *items = cui__frame_alloc(ui, sizeof *items * 32);
    int count = 0, i;
    if (!items) return;
    memset(&o, 0, sizeof o);
    for (i = 0; i < n->kid_count && count < 32; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_DIAGNOSTIC) continue;
        items[count].severity = (cui_severity)cui__xml_choice_of(doc, kid, "severity", severities, 4, 0);
        items[count].message = cui__xml_label(doc, kid);
        items[count].file = cui__xml_attr_text(doc, kid, "file", NULL);
        items[count].line = (int)cui__xml_attr_num(doc, kid, "line", 0);
        items[count].column = (int)cui__xml_attr_num(doc, kid, "column", 0);
        items[count].source = cui__xml_attr_text(doc, kid, "source", NULL);
        items[count].code = cui__xml_attr_text(doc, kid, "code", NULL);
        count++;
    }
    if (!count) return;
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    cui_problems_ex(ui, n->key, items, count, &o);
}

typedef struct {
    cui_kv_pair pairs[16];
    int count;
    bool init;
} cui__xml_kv_state;

/* <Pair Key="Accept" Value="application/json" Enabled="true" Secret="false"/> */
static void cui__xml_draw_kv_editor(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_kv_editor_opts o;
    cui__xml_kv_state *st = n->cache;
    int i;
    if (!st) {
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        for (i = 0; i < n->kid_count && st->count < 16; i++) {
            cui_xml_node *kid = n->kids[i];
            const char *key, *value;
            if (kid->comment || kid->tag_id != CUI__XML_TAG_PAIR) continue;
            key = cui__xml_attr_text(doc, kid, "key", NULL);
            value = cui__xml_attr_text(doc, kid, "value", NULL);
            snprintf(st->pairs[st->count].key, sizeof st->pairs[st->count].key, "%s", key ? key : "");
            snprintf(st->pairs[st->count].value, sizeof st->pairs[st->count].value, "%s", value ? value : "");
            st->pairs[st->count].enabled = cui__xml_attr_bool(doc, kid, "enabled", true);
            st->pairs[st->count].secret = cui__xml_attr_bool(doc, kid, "secret", false);
            st->count++;
        }
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    if (st->count < 1) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_kv_editor_ex(ui, n->key, st->pairs, &st->count, 16, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* <Stage Text="Build"/> children name the stages; <Job Text Stage Status Duration
 * AllowFailure/> children are the jobs. */
static void cui__xml_draw_pipeline(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    static const char *const statuses[] = { "pending", "running", "passed", "failed", "skipped", "canceled" };
    cui_pipeline_opts o;
    const char **stages = cui__frame_alloc(ui, sizeof *stages * 8);
    cui_pipeline_job *jobs = cui__frame_alloc(ui, sizeof *jobs * 32);
    int *selected = cui__xml_slot_int(doc, n);
    int stage_count = 0, job_count = 0, i;
    if (!stages || !jobs) return;
    for (i = 0; i < n->kid_count && stage_count < 8; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_STAGE) continue;
        stages[stage_count++] = cui__xml_label(doc, kid);
    }
    for (i = 0; i < n->kid_count && job_count < 32; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_JOB) continue;
        jobs[job_count].name = cui__xml_label(doc, kid);
        jobs[job_count].stage = (int)cui__xml_attr_num(doc, kid, "stage", 0);
        jobs[job_count].status = (cui_job_status)cui__xml_choice_of(doc, kid, "status", statuses, 6, 0);
        jobs[job_count].duration = cui__xml_attr_num(doc, kid, "duration", 0);
        jobs[job_count].allow_failure = cui__xml_attr_bool(doc, kid, "allowfailure", false);
        job_count++;
    }
    if (!stage_count || !job_count) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.selected = selected;
    if (!n->seen) *selected = (int)cui__xml_attr_num(doc, n, "selected", -1);
    if (disabled) cui_push_disabled(ui, true);
    cui_pipeline_ex(ui, n->key, stages, stage_count, jobs, job_count, &o);
    if (disabled) cui_pop_disabled(ui);
}

/* <Field Text Type="text|number|choice|bool" Options="a|b"/> and
 * <Condition Group="and|any" Field="0" Op="0" Value="Open"/>. */
static void cui__xml_draw_query_builder(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    static const char *const types[] = { "text", "number", "choice", "bool" };
    static const char *const groups[] = { "all", "any" };
    cui_query_opts qopts;
    cui_query_field *fields = cui__frame_alloc(ui, sizeof *fields * 8);
    char **choices = cui__frame_alloc(ui, sizeof *choices * 8 * 16);
    cui_query_node *nodes = cui__frame_alloc(ui, sizeof *nodes * 24);
    int field_count = 0, node_count = 0, i;
    if (!fields || !choices || !nodes) return;
    for (i = 0; i < n->kid_count && field_count < 8; i++) {
        cui_xml_node *kid = n->kids[i];
        const char *options;
        if (kid->comment || kid->tag_id != CUI__XML_TAG_FIELD) continue;
        options = cui__xml_attr_text(doc, kid, "options", NULL);
        fields[field_count].name = cui__xml_label(doc, kid);
        fields[field_count].type = (cui_query_type)cui__xml_choice_of(doc, kid, "type", types, 4, 0);
        fields[field_count].choices = (const char *const *)(choices + field_count * 16);
        fields[field_count].choice_count = cui__xml_split(ui, options, '|', choices + field_count * 16, 16);
        if (fields[field_count].choice_count < 1) {
            fields[field_count].choices = NULL;
            fields[field_count].choice_count = 0;
        }
        field_count++;
    }
    for (i = 0; i < n->kid_count && node_count < 24; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_CONDITION) continue;
        memset(&nodes[node_count], 0, sizeof nodes[node_count]);
        nodes[node_count].depth = (int)cui__xml_attr_num(doc, kid, "depth", 0);
        nodes[node_count].group = cui__xml_attr_raw(kid, "field") == NULL;
        {
            const char *group = cui__xml_attr_text(doc, kid, "group", NULL);
            if (group) {
                nodes[node_count].group = true;
                nodes[node_count].any = cui__xml_choice_of(doc, kid, "group", groups, 2, 0) == 1;
            }
        }
        nodes[node_count].field = (int)cui__xml_attr_num(doc, kid, "field", 0);
        nodes[node_count].op = (int)cui__xml_attr_num(doc, kid, "op", 0);
        {
            const char *value = cui__xml_attr_text(doc, kid, "value", NULL);
            if (value) snprintf(nodes[node_count].value, sizeof nodes[node_count].value, "%s", value);
        }
        node_count++;
    }
    if (!field_count || !node_count) return;
    memset(&qopts, 0, sizeof qopts);
    if (disabled) cui_push_disabled(ui, true);
    cui_query_builder_ex(ui, n->key, fields, field_count, nodes, &node_count, 24, &qopts);
    if (disabled) cui_pop_disabled(ui);
    if (cui__xml_attr_bool(doc, n, "sentence", false)) {
        char text[256];
        cui_query_text(fields, field_count, nodes, node_count, text, sizeof text);
        cui_text(ui, text, .size = 12, .color = CUI_COLOR_TEXT_MUTED);
    }
}

typedef struct {
    char cron[64];
    bool init;
} cui__xml_schedule_state;

static void cui__xml_draw_schedule_picker(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_schedule_opts o;
    cui__xml_schedule_state *st = n->cache;
    if (!st) {
        const char *cron = cui__xml_attr_text(doc, n, "cron", NULL);
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        snprintf(st->cron, sizeof st->cron, "%s", cron ? cron : "");
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    memset(&o, 0, sizeof o);
    o.now = (double)cui__xml_attr_num(doc, n, "now", 0);
    o.utc = cui__xml_attr_bool(doc, n, "utc", false);
    o.next_count = (int)cui__xml_attr_num(doc, n, "nextcount", 0);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_schedule_picker_ex(ui, n->key, st->cron, sizeof st->cron, &o).changed) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* "primary+s" or "f5": the key names cui_key_name prints. */
static cui_key cui__xml_key_of(const char *text) {
    size_t len = text ? strlen(text) : 0;
    if (len == 1) {
        char c = text[0] >= 'a' && text[0] <= 'z' ? (char)(text[0] - 'a' + 'A') : text[0];
        if (c >= 'A' && c <= 'Z') return (cui_key)(CUI_KEY_A + (c - 'A'));
        if (c >= '0' && c <= '9') return (cui_key)(CUI_KEY_0 + (c - '0'));
    }
    if (len > 1 && (text[0] == 'f' || text[0] == 'F')) {
        int number = atoi(text + 1);
        if (number >= 1 && number <= 12) return (cui_key)(CUI_KEY_F1 + (number - 1));
    }
    if (!strcmp(text, "slash")) return CUI_KEY_SLASH;
    if (!strcmp(text, "comma")) return CUI_KEY_COMMA;
    if (!strcmp(text, "enter")) return CUI_KEY_ENTER;
    if (!strcmp(text, "escape")) return CUI_KEY_ESCAPE;
    if (!strcmp(text, "space")) return CUI_KEY_SPACE;
    if (!strcmp(text, "tab")) return CUI_KEY_TAB;
    return CUI_KEY_A;
}

/* <Binding Text="Save" Keys="primary+s" Category="0"/> children. */
static void cui__xml_draw_keymap(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_keymap_opts o;
    cui_keymap_binding *binds = cui__frame_alloc(ui, sizeof *binds * 64);
    const char **categories = cui__frame_alloc(ui, sizeof *categories * 8);
    int *mods = cui__xml_slot_int(doc, n);
    int count = 0, category_count = 0, i;
    if (!binds || !categories) return;
    for (i = 0; i < n->kid_count && count < 64; i++) {
        cui_xml_node *kid = n->kids[i];
        const char *keys;
        char **parts;
        int part_count, k;
        if (kid->comment || kid->tag_id != CUI__XML_TAG_BINDING) continue;
        keys = cui__xml_attr_text(doc, kid, "keys", NULL);
        parts = cui__frame_alloc(ui, sizeof *parts * 4);
        if (!parts) return;
        memset(&binds[count], 0, sizeof binds[count]);
        part_count = cui__xml_split(ui, keys, '+', parts, 4);
        for (k = 0; k < part_count; k++) {
            if (!strcmp(parts[k], "primary")) binds[count].combo.mods |= CUI_MOD_PRIMARY;
            else if (!strcmp(parts[k], "shift")) binds[count].combo.mods |= CUI_MOD_SHIFT;
            else if (!strcmp(parts[k], "alt")) binds[count].combo.mods |= CUI_MOD_ALT;
            else if (!strcmp(parts[k], "ctrl")) binds[count].combo.mods |= CUI_MOD_CTRL;
            else binds[count].combo.key = cui__xml_key_of(parts[k]);
        }
        binds[count].action = cui__xml_label(doc, kid);
        binds[count].category = (int)cui__xml_attr_num(doc, kid, "category", 0);
        count++;
    }
    for (i = 0; i < n->kid_count && category_count < 8; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_CATEGORY) continue;
        categories[category_count++] = cui__xml_label(doc, kid);
    }
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.categories = category_count ? categories : NULL;
    o.category_count = category_count;
    o.mods = mods;
    o.no_function_row = cui__xml_attr_bool(doc, n, "nofunctionrow", false);
    if (!n->seen) *mods = (int)cui__xml_attr_num(doc, n, "mods", CUI_MOD_PRIMARY);
    if (disabled) cui_push_disabled(ui, true);
    cui_keymap_ex(ui, n->key, binds, count, &o);
    if (disabled) cui_pop_disabled(ui);
}

/* ---- more developer tools: tests, frames, requests, coverage, flags, API -- */

static void cui__xml_draw_test_explorer(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    static const char *const statuses[] = { "pending", "running", "passed", "failed", "skipped", "canceled" };
    cui_test_explorer_opts o;
    cui_test_item *items = cui__frame_alloc(ui, sizeof *items * 48);
    int count = 0, i;
    if (!items) return;
    for (i = 0; i < n->kid_count && count < 48; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_TEST) continue;
        items[count].name = cui__xml_label(doc, kid);
        items[count].depth = (int)cui__xml_attr_num(doc, kid, "depth", 0);
        items[count].status = (cui_job_status)cui__xml_choice_of(doc, kid, "status", statuses, 6, 0);
        items[count].duration = cui__xml_attr_num(doc, kid, "duration", -1);
        items[count].message = cui__xml_attr_text(doc, kid, "message", NULL);
        count++;
    }
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    if (disabled) cui_push_disabled(ui, true);
    cui_test_explorer_ex(ui, n->key, items, count, &o);
    if (disabled) cui_pop_disabled(ui);
}

/* <Frame Text="total" File="src/cart.js" Line="29" Column="3" Source="l1|l2" First="1"/> */
static void cui__xml_draw_stack_trace(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_stack_trace_opts o;
    cui_stack_frame *frames = cui__frame_alloc(ui, sizeof *frames * 32);
    int count = 0, i;
    if (!frames) return;
    for (i = 0; i < n->kid_count && count < 32; i++) {
        cui_xml_node *kid = n->kids[i];
        const char *source;
        if (kid->comment || kid->tag_id != CUI__XML_TAG_FRAME) continue;
        memset(&frames[count], 0, sizeof frames[count]);
        frames[count].function = cui__xml_label(doc, kid);
        frames[count].file = cui__xml_attr_text(doc, kid, "file", NULL);
        frames[count].line = (int)cui__xml_attr_num(doc, kid, "line", 0);
        frames[count].column = (int)cui__xml_attr_num(doc, kid, "column", 0);
        frames[count].external = cui__xml_attr_bool(doc, kid, "library", false);
        frames[count].source_first = (int)cui__xml_attr_num(doc, kid, "first", 0);
        source = cui__xml_attr_text(doc, kid, "source", NULL);
        if (source && *source) {
            char **lines = cui__frame_alloc(ui, sizeof *lines * 16);
            if (lines) {
                frames[count].source = (const char *const *)lines;
                frames[count].source_count = cui__xml_split(ui, source, '|', lines, 16);
            }
        }
        count++;
    }
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.title = cui__xml_attr_text(doc, n, "title", NULL);
    o.message = cui__xml_attr_text(doc, n, "message", NULL);
    o.lang = cui__xml_attr_text(doc, n, "lang", NULL);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    if (disabled) cui_push_disabled(ui, true);
    cui_stack_trace_ex(ui, n->key, frames, count, &o);
    if (disabled) cui_pop_disabled(ui);
}

typedef struct {
    char url[256];
    cui_kv_pair pairs[16];
    int count;
    bool init;
} cui__xml_request_state;

/* <RequestBar Name Method="1" Url="??/>: the method and the URL stay in the element. */
static void cui__xml_draw_request_bar(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_request_bar_opts o;
    cui__xml_request_state *st = n->cache;
    int *method = cui__xml_slot_int(doc, n);
    cui_request_bar_result r;
    if (!st) {
        const char *url = cui__xml_attr_text(doc, n, "url", NULL);
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        snprintf(st->url, sizeof st->url, "%s", url ? url : "");
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    if (!n->seen) *method = (int)cui__xml_attr_num(doc, n, "method", 0);
    if (disabled) cui_push_disabled(ui, true);
    r = cui_request_bar_ex(ui, n->key, method, st->url, sizeof st->url, &o);
    if (disabled) cui_pop_disabled(ui);
    if (r.changed || r.send) cui__xml_changed_hook(doc, n);
}

static void cui__xml_draw_response_status(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_response_status(ui, (int)cui__xml_attr_num(doc, n, "code", 0), cui__xml_attr_num(doc, n, "time", 0),
                        cui__xml_attr_num(doc, n, "size", 0));
}

/* <UrlParams Name/>: the query of the URL above, kept in the element. */
typedef struct {
    cui_kv_pair pairs[16];
    int count;
    bool init;
} cui__xml_params_state;

static void cui__xml_draw_url_params(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui__xml_params_state *st = n->cache;
    cui_kv_editor_opts o;
    if (!st) {
        const char *url = cui__xml_attr_text(doc, n, "url", NULL);
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        st->count = cui_url_params(url ? url : "", st->pairs, 16);
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    if (st->count < 1) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_kv_editor_ex(ui, n->key, st->pairs, &st->count, 16, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_coverage(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_coverage_opts o;
    cui_coverage_file *files = cui__frame_alloc(ui, sizeof *files * 32);
    int count = 0, i;
    if (!files) return;
    for (i = 0; i < n->kid_count && count < 32; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_COVERAGEFILE) continue;
        files[count].path = cui__xml_label(doc, kid);
        files[count].lines = (int)cui__xml_attr_num(doc, kid, "lines", 0);
        files[count].lines_hit = (int)cui__xml_attr_num(doc, kid, "lineshit", 0);
        files[count].branches = (int)cui__xml_attr_num(doc, kid, "branches", 0);
        files[count].branches_hit = (int)cui__xml_attr_num(doc, kid, "brancheshit", 0);
        files[count].functions = (int)cui__xml_attr_num(doc, kid, "functions", 0);
        files[count].functions_hit = (int)cui__xml_attr_num(doc, kid, "functionshit", 0);
        count++;
    }
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.low = cui__xml_attr_num(doc, n, "low", 0);
    o.high = cui__xml_attr_num(doc, n, "high", 0);
    if (disabled) cui_push_disabled(ui, true);
    cui_coverage_ex(ui, n->key, files, count, &o);
    if (disabled) cui_pop_disabled(ui);
}

/* "1,1,0" or "on,on,off" per environment. */
static int cui__xml_flags_of(cui_ctx *ui, const char *text, bool *on, int *rollout, int cap) {
    char **entries = cui__frame_alloc(ui, sizeof *entries * 8);
    int count, i;
    if (!entries) return 0;
    count = cui__xml_split(ui, text, ',', entries, cap < 8 ? cap : 8);
    for (i = 0; i < count; i++) {
        const char *entry = entries[i];
        if (!strcmp(entry, "on")) {
            on[i] = true;
            rollout[i] = 100;
        } else if (!strcmp(entry, "off")) {
            on[i] = false;
            rollout[i] = 0;
        } else {
            int percent = atoi(entry);
            on[i] = percent > 0;
            rollout[i] = percent;
        }
    }
    return count;
}

typedef struct {
    cui_feature_flag flags[16];
    int count;
    bool init;
} cui__xml_flags_state;

static void cui__xml_draw_feature_flags(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_feature_flags_opts o;
    cui__xml_flags_state *st = n->cache;
    const char **envs = cui__frame_alloc(ui, sizeof *envs * 8);
    int env_count = 0, i;
    if (!envs) return;
    for (i = 0; i < n->kid_count && env_count < 8; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_ENV) continue;
        envs[env_count++] = cui__xml_label(doc, kid);
    }
    if (!st) {
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        for (i = 0; i < n->kid_count && st->count < 16; i++) {
            cui_xml_node *kid = n->kids[i];
            if (kid->comment || kid->tag_id != CUI__XML_TAG_FLAG) continue;
            st->flags[st->count].key = cui__xml_label(doc, kid);
            st->flags[st->count].description = cui__xml_attr_text(doc, kid, "description", NULL);
            {
                char **states = cui__frame_alloc(ui, sizeof *states * 8);
                char **shares = cui__frame_alloc(ui, sizeof *shares * 8);
                int state_count = states ? cui__xml_split(ui, cui__xml_attr_text(doc, kid, "envs", NULL), ',', states, CUI_FLAG_ENVS) : 0;
                int share_count = shares ? cui__xml_split(ui, cui__xml_attr_text(doc, kid, "rollouts", NULL), ',', shares, CUI_FLAG_ENVS) : 0;
                int e;
                for (e = 0; e < state_count; e++) {
                    const char *word = states[e];
                    if (!strcmp(word, "on")) st->flags[st->count].on[e] = true;
                    else if (!strcmp(word, "off")) st->flags[st->count].on[e] = false;
                    else st->flags[st->count].on[e] = atoi(word) > 0;
                }
                for (e = 0; e < share_count; e++) st->flags[st->count].rollout[e] = atoi(shares[e]);
            }
            st->count++;
        }
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    if (!st->count || !env_count) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_feature_flags_ex(ui, n->key, st->flags, st->count, envs, env_count, &o).flag >= 0)
        cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* <Endpoint Method Path Text Category Deprecated="false"> with <Param> children. */
static void cui__xml_draw_api_explorer(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_api_explorer_opts o;
    cui_api_endpoint *endpoints = cui__frame_alloc(ui, sizeof *endpoints * 32);
    int *selected = cui__xml_slot_int(doc, n);
    int count = 0, i;
    if (!endpoints) return;
    for (i = 0; i < n->kid_count && count < 32; i++) {
        cui_xml_node *kid = n->kids[i];
        cui_api_param *params;
        int param_count = 0, k;
        if (kid->comment || kid->tag_id != CUI__XML_TAG_ENDPOINT) continue;
        memset(&endpoints[count], 0, sizeof endpoints[count]);
        endpoints[count].method = cui__xml_attr_text(doc, kid, "method", "GET");
        endpoints[count].path = cui__xml_attr_text(doc, kid, "path", "/");
        endpoints[count].summary = cui__xml_label(doc, kid);
        endpoints[count].tag = cui__xml_attr_text(doc, kid, "category", NULL);
        endpoints[count].deprecated = cui__xml_attr_bool(doc, kid, "deprecated", false);
        params = cui__frame_alloc(ui, sizeof *params * 8);
        if (params) {
            for (k = 0; k < kid->kid_count && param_count < 8; k++) {
                cui_xml_node *p = kid->kids[k];
                if (p->comment || p->tag_id != CUI__XML_TAG_PARAM) continue;
                params[param_count].name = cui__xml_label(doc, p);
                params[param_count].in = cui__xml_attr_text(doc, p, "in", "query");
                params[param_count].type = cui__xml_attr_text(doc, p, "type", "string");
                params[param_count].required = cui__xml_attr_bool(doc, p, "required", false);
                params[param_count].description = cui__xml_attr_text(doc, p, "description", NULL);
                param_count++;
            }
            endpoints[count].params = param_count ? params : NULL;
            endpoints[count].param_count = param_count;
        }
        count++;
    }
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.selected = selected;
    if (!n->seen) *selected = (int)cui__xml_attr_num(doc, n, "selected", -1);
    if (disabled) cui_push_disabled(ui, true);
    cui_api_explorer_ex(ui, n->key, endpoints, count, &o);
    if (disabled) cui_pop_disabled(ui);
}

typedef struct {
    char pattern[256];
    char text[2048];
    char seen_pattern[256];
    char seen_text[2048];
    bool init;
} cui__xml_regex_state;

static void cui__xml_draw_regex_tester(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_regex_tester_opts o;
    cui__xml_regex_state *st = n->cache;
    if (!st) {
        const char *pattern = cui__xml_attr_text(doc, n, "pattern", NULL);
        const char *text = cui__xml_attr_text(doc, n, "text", NULL);
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        snprintf(st->pattern, sizeof st->pattern, "%s", pattern ? pattern : "");
        snprintf(st->text, sizeof st->text, "%s", text ? text : "");
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    if (disabled) cui_push_disabled(ui, true);
    cui_regex_tester_ex(ui, n->key, st->pattern, sizeof st->pattern, st->text, sizeof st->text, &o);
    if (disabled) cui_pop_disabled(ui);
    if (strcmp(st->seen_pattern, st->pattern) || strcmp(st->seen_text, st->text)) {
        snprintf(st->seen_pattern, sizeof st->seen_pattern, "%s", st->pattern);
        snprintf(st->seen_text, sizeof st->seen_text, "%s", st->text);
        cui__xml_changed_hook(doc, n);
    }
}

/* ---- canvas: a node graph, its rail, live cursors and a format bar ------- */

/* The editor and the rail work on one canvas, so its pan, zoom, grid and node
 * positions live in the context, not in one element. */
typedef struct {
    cui_node_view view;
    bool grid, layers;
    float node_x[8], node_y[8];
    bool init;
} cui__xml_canvas_state;

#define CUI__XML_CANVAS_ID 0xCA5A5u

static cui__xml_canvas_state *cui__xml_canvas(cui_ctx *ui) {
    cui__xml_canvas_state *st = cui_state(ui, CUI__XML_CANVAS_ID, sizeof *st);
    int i;
    if (!st) return NULL;
    if (!st->init) {
        static const float start[3][2] = { { 36, 64 }, { 36, 206 }, { 330, 124 } };
        st->view = (cui_node_view){ 10, 6, 1 };
        st->grid = true;
        st->layers = false;
        for (i = 0; i < 3; i++) {
            st->node_x[i] = start[i][0];
            st->node_y[i] = start[i][1];
        }
        st->init = true;
    }
    return st;
}

/* A rect written "x,y,w,h". */
static cui_rect cui__xml_rect_of(const char *text) {
    cui_rect r = { 0, 0, 0, 0 };
    if (text) sscanf(text, "%f,%f,%f,%f", &r.x, &r.y, &r.w, &r.h);
    return r;
}

static void cui__xml_draw_node_editor(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_node_editor_opts o;
    cui__xml_canvas_state *st = cui__xml_canvas(ui);
    int index = 0, i;
    if (!st) return;
    memset(&o, 0, sizeof o);
    o.height = cui__xml_attr_size(doc, n, "height", CUI_PX(360));
    o.min_zoom = cui__xml_attr_num(doc, n, "minzoom", 0);
    o.max_zoom = cui__xml_attr_num(doc, n, "maxzoom", 0);
    o.no_grid = !st->grid || cui__xml_attr_bool(doc, n, "nogrid", false);
    o.minimap = cui__xml_attr_bool(doc, n, "minimap", false);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_node_editor_begin_ex(ui, n->key, &st->view, &o)) {
        for (i = 0; i < n->kid_count; i++) {
            cui_xml_node *kid = n->kids[i];
            cui_node_opts no;
            cui_xml_node *ring = NULL;
            int k;
            if (kid->comment || kid->tag_id != CUI__XML_TAG_NODE) {
                if (kid->tag_id == CUI__XML_TAG_NODELINK) continue;   /* links come after the nodes */
                cui__xml_draw(doc, kid, ui, disabled);
                continue;
            }
            if (index >= 8) index = 0;
            memset(&no, 0, sizeof no);
            no.width = cui__xml_attr_num(doc, kid, "width", 0);
            no.icon = cui__xml_icon(doc, kid, NULL);
            cui_node_begin_ex(ui, index + 1, &st->node_x[index], &st->node_y[index], cui__xml_label(doc, kid), &no);
            for (k = 0; k < kid->kid_count; k++) {
                cui_xml_node *part = kid->kids[k];
                cui_pin_opts pin;
                memset(&pin, 0, sizeof pin);
                pin.type = (int)cui__xml_attr_num(doc, part, "type", 0);
                if (part->tag_id == CUI__XML_TAG_NODEINPUT)
                    cui_node_input_ex(ui, (int)cui__xml_attr_num(doc, part, "pin", 0), cui__xml_label(doc, part), &pin);
                else if (part->tag_id == CUI__XML_TAG_NODEOUTPUT)
                    cui_node_output_ex(ui, (int)cui__xml_attr_num(doc, part, "pin", 0), cui__xml_label(doc, part), &pin);
                else if (part->tag_id == CUI__XML_TAG_NODECONTENT) {
                    cui_node_content_begin(ui);
                    cui__xml_draw_kids(doc, part, ui, disabled);
                    cui_node_content_end(ui);
                } else if (part->tag_id == CUI__XML_TAG_SELECTIONRING) {
                    ring = part;
                } else if (!part->comment) {
                    cui__xml_draw(doc, part, ui, disabled);
                }
            }
            cui_node_end(ui);
            if (ring) {
                cui_selection_ring_opts ro;
                memset(&ro, 0, sizeof ro);
                ro.name = cui__xml_attr_text(doc, ring, "name", NULL);
                cui_selection_ring_ex(ui, cui_color_of(ui, cui__xml_attr_color(doc, ring, "color", CUI_COLOR_WARNING)), &ro);
            }
            index++;
        }
        for (i = 0; i < n->kid_count; i++) {
            cui_xml_node *kid = n->kids[i];
            if (kid->comment || kid->tag_id != CUI__XML_TAG_NODELINK) continue;
            cui_node_link(ui, (int)cui__xml_attr_num(doc, kid, "link", 0), (int)cui__xml_attr_num(doc, kid, "from", 0),
                          (int)cui__xml_attr_num(doc, kid, "to", 0));
        }
        cui_node_editor_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
}

typedef struct {
    bool toggles[8];
    bool init[8];
} cui__xml_canvas_rail_state;

static void cui__xml_draw_canvas_controls(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_canvas_controls_opts o;
    cui__xml_canvas_state *st = cui__xml_canvas(ui);
    cui__xml_canvas_rail_state *rail = n->cache;
    cui_canvas_toggle *toggles = cui__frame_alloc(ui, sizeof *toggles * 8);
    int count = 0, i;
    if (!st || !toggles) return;
    if (!rail) {
        rail = cui__xml_alloc(sizeof *rail);
        if (!rail) return;
        memset(rail, 0, sizeof *rail);
        n->cache = rail;
        n->cache_free = cui__xml_free_cache;
    }
    for (i = 0; i < n->kid_count && count < 8; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment || kid->tag_id != CUI__XML_TAG_CANVASTOGGLE) continue;
        if (!rail->init[count]) {
            rail->toggles[count] = cui__xml_attr_bool(doc, kid, "on", false);
            rail->init[count] = true;
        }
        toggles[count].icon = cui__xml_icon(doc, kid, NULL);
        toggles[count].label = cui__xml_attr_text(doc, kid, "text", NULL);
        toggles[count].on = &rail->toggles[count];
        count++;
    }
    memset(&o, 0, sizeof o);
    o.content = cui__xml_rect_of(cui__xml_attr_text(doc, n, "content", NULL));
    o.toggles = count ? toggles : NULL;
    o.toggle_count = count;
    o.no_fit = cui__xml_attr_bool(doc, n, "nofit", false);
    o.start_side = cui__xml_attr_bool(doc, n, "startside", false);
    if (disabled) cui_push_disabled(ui, true);
    cui_canvas_controls_ex(ui, n->key, &st->view, &o);
    if (disabled) cui_pop_disabled(ui);
    /* the rail's toggles drive the editor's grid and the layers switch */
    st->grid = rail->toggles[0];
}

/* "Paul:warning:speaking|Kate:info|Mario:rose": a name, a tone, and what the
 * person is doing. */
static int cui__xml_people_of(cui_ctx *ui, const char *text, cui_live_user *out, int cap) {
    static const char *const tones[] = { "neutral", "info", "success", "warning", "danger", "violet", "teal", "rose", "accent" };
    char **entries = cui__frame_alloc(ui, sizeof *entries * 8);
    int count, i;
    if (!entries) return 0;
    count = cui__xml_split(ui, text, '|', entries, cap < 8 ? cap : 8);
    for (i = 0; i < count; i++) {
        char *fields[3];
        int field_count = cui__xml_split(ui, entries[i], ':', fields, 3);
        int k, tone = 0;
        memset(&out[i], 0, sizeof out[i]);
        out[i].name = field_count > 0 ? fields[0] : "";
        if (field_count > 1) {
            for (k = 0; k < 9; k++)
                if (!strcmp(fields[1], tones[k])) tone = k;
        }
        out[i].tone = (cui_tone)tone;
        if (field_count > 2) out[i].speaking = !strcmp(fields[2], "speaking") || !strcmp(fields[2], "says");
    }
    return count;
}

static void cui__xml_draw_live_cursors(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_live_cursors_opts o;
    cui__xml_canvas_state *st = cui__xml_canvas(ui);
    cui_live_user *people = cui__frame_alloc(ui, sizeof *people * 8);
    int count, k;
    double now;
    if (!people) return;
    count = cui__xml_people_of(ui, cui__xml_attr_text(doc, n, "users", NULL), people, 8);
    if (!count) return;
    now = cui_time(ui);
    for (k = 0; k < count; k++) {
        unsigned step = (unsigned)(now / (1.6 + 0.4 * k));
        unsigned h = step * 2654435761u + (unsigned)k * 40503u;
        people[k].x = 60 + (float)(h % 420);
        people[k].y = 60 + (float)((h >> 12) % 220);
        if (k == 1) people[k].says = (step % 3) ? "Bumping steps to 40?" : NULL;
    }
    memset(&o, 0, sizeof o);
    o.view = st ? &st->view : NULL;
    o.no_glide = cui__xml_attr_bool(doc, n, "noglide", false);
    cui_live_cursors_ex(ui, n->key, people, count, &o);
}

static void cui__xml_draw_presence_pill(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_presence_pill_opts o;
    cui_live_user *people = cui__frame_alloc(ui, sizeof *people * 8);
    int count;
    if (!people) return;
    count = cui__xml_people_of(ui, cui__xml_attr_text(doc, n, "users", NULL), people, 8);
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.max_visible = (int)cui__xml_attr_num(doc, n, "visible", 0);
    cui_presence_pill_ex(ui, n->key, people, count, &o);
}

typedef struct {
    cui_text_format format;
    bool init;
} cui__xml_format_state;

/* <FormatBar Tone="rose" Bold="true" Style="0" Size="44" Align="left" List="none"/> */
static void cui__xml_draw_format_bar(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    static const char *const aligns[] = { "left", "center", "right" };
    static const char *const lists[] = { "none", "bullet", "number" };
    cui__xml_format_state *st = n->cache;
    int flags;
    if (!st) {
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        st->format.color = (cui_tone)cui__xml_tone_attr(doc, n, "tone", CUI_TONE_NEUTRAL);
        st->format.bold = cui__xml_attr_bool(doc, n, "bold", false);
        st->format.underline = cui__xml_attr_bool(doc, n, "underline", false);
        st->format.link = cui__xml_attr_bool(doc, n, "link", false);
        st->format.style = (int)cui__xml_attr_num(doc, n, "style", 0);
        st->format.size = cui__xml_attr_num(doc, n, "size", 0);
        st->format.align = (cui_format_align)cui__xml_choice_of(doc, n, "align", aligns, 3, 0);
        st->format.list = (cui_format_list)cui__xml_choice_of(doc, n, "list", lists, 3, 0);
        st->init = true;
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    if (disabled) cui_push_disabled(ui, true);
    flags = cui_format_bar_ex(ui, n->key, &st->format, NULL);
    if (disabled) cui_pop_disabled(ui);
    if (flags) {
        n->reported = flags;
        cui__xml_fire(doc, n, "onclicked");
    }
}

static void cui__xml_draw_text_field(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled, int kind) {
    cui_input_opts o;
    size_t cap = 0;
    char *buf;
    int result;
    memset(&o, 0, sizeof o);
    cap = (size_t)cui__xml_attr_num(doc, n, "maxlength", 0);
    buf = cui__xml_slot_text(doc, n, &cap, cap);
    if (!buf || !cap) return;
    if (!n->seen) {
        const char *value = cui__xml_attr_text(doc, n, "value", NULL);
        if (value && !buf[0]) snprintf(buf, cap, "%s", value);
    }
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.placeholder = cui__xml_attr_text(doc, n, "placeholder", NULL);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.hint = cui__xml_attr_text(doc, n, "hint", NULL);
    o.error = cui__xml_attr_text(doc, n, "error", NULL);
    o.password = cui__xml_attr_bool(doc, n, "password", false);
    o.disabled = disabled || cui__xml_attr_bool(doc, n, "disabled", false);
    o.clearable = cui__xml_attr_bool(doc, n, "clearable", false);
    o.autofocus = cui__xml_attr_bool(doc, n, "autofocus", false);
    o.read_only = cui__xml_attr_bool(doc, n, "readonly", false);
    o.size = cui__xml_control_size_of(doc, n, CUI_SIZE_MD);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.prefix = cui__xml_attr_text(doc, n, "prefix", NULL);
    o.suffix = cui__xml_attr_text(doc, n, "suffix", NULL);
    if (kind == 1) result = cui_textarea_ex(ui, n->key, buf, cap, &o);
    else if (kind == 2) result = cui_search_ex(ui, n->key, buf, cap, &o);
    else result = cui_input_ex(ui, n->key, buf, cap, &o);
    if (result & CUI_CHANGED) cui__xml_changed_hook(doc, n);
    if (result & CUI_SUBMITTED) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onsubmitted");
    }
}

static void cui__xml_draw_number(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_number_opts o;
    double *value = cui__xml_slot_double(doc, n);
    double lo = (double)cui__xml_attr_num(doc, n, "min", 0);
    double hi = (double)cui__xml_attr_num(doc, n, "max", 100);
    memset(&o, 0, sizeof o);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.step = cui__xml_attr_num(doc, n, "step", 1);
    o.decimals = (int)cui__xml_attr_num(doc, n, "decimals", 0);
    o.disabled = disabled || cui__xml_attr_bool(doc, n, "disabled", false);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FIT);
    o.suffix = cui__xml_attr_text(doc, n, "suffix", NULL);
    if (!n->seen) *value = (double)cui__xml_attr_num(doc, n, "value", (float)*value);
    if (hi <= lo) hi = lo + 1;
    if (cui_number_ex(ui, n->key, value, lo, hi, &o)) cui__xml_changed_hook(doc, n);
}

static void cui__xml_draw_select(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_select_opts o;
    const char **items = NULL;
    int count = cui__xml_attr_items(doc, n, &items);
    int *index = cui__xml_slot_int(doc, n);
    memset(&o, 0, sizeof o);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.placeholder = cui__xml_attr_text(doc, n, "placeholder", NULL);
    o.searchable = cui__xml_attr_bool(doc, n, "searchable", false);
    o.disabled = disabled || cui__xml_attr_bool(doc, n, "disabled", false);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    if (!count) return;

    if (cui_select_ex(ui, n->key, items, count, index, &o)) cui__xml_changed_hook(doc, n);
}

static void cui__xml_draw_segmented(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_segmented_opts o;
    const char **items = NULL;
    int count = cui__xml_attr_items(doc, n, &items);
    int *index = cui__xml_slot_int(doc, n);
    int icon_count = 0;
    memset(&o, 0, sizeof o);
    o.size = cui__xml_control_size_of(doc, n, CUI_SIZE_MD);
    o.full_width = cui__xml_attr_bool(doc, n, "fullwidth", false);
    if (cui__xml_attr_raw(n, "icons")) {
        const char **icons = cui__xml_attr_list(doc, n, "icons", &icon_count, true);
        if (icon_count == count) o.icons = icons;
    }
    if (!count) return;

    if (disabled) cui_push_disabled(ui, true);
    if (cui_segmented_ex(ui, n->key, items, count, index, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_tabs(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_tabs_opts o;
    const char **items = NULL;
    int count = cui__xml_attr_items(doc, n, &items);
    int *index = cui__xml_slot_int(doc, n);
    int icon_count = 0;
    static const char *const variants[] = { "underline", "pill" };
    memset(&o, 0, sizeof o);
    o.variant = (cui_tabs_variant)cui__xml_choice_of(doc, n, "variant", variants, 2, 0);
    o.full_width = cui__xml_attr_bool(doc, n, "fullwidth", false);
    if (cui__xml_attr_raw(n, "icons")) {
        const char **icons = cui__xml_attr_list(doc, n, "icons", &icon_count, true);
        if (icon_count == count) o.icons = icons;
    }
    if (!count) return;

    if (disabled) cui_push_disabled(ui, true);
    if (cui_tabs_ex(ui, n->key, items, count, index, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_nav_item(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_nav_opts o;
    memset(&o, 0, sizeof o);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.badge = cui__xml_attr_text(doc, n, "badge", NULL);
    o.selected = cui__xml_attr_bool(doc, n, "selected", false);
    o.group = cui__xml_attr_text(doc, n, "group", NULL);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_nav_item_ex(ui, cui__xml_label(doc, n), &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_pagination(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    int *page = cui__xml_slot_int(doc, n);
    int pages = (int)cui__xml_attr_num(doc, n, "pagecount", 5);
    if (pages < 1) pages = 1;
    if (disabled) cui_push_disabled(ui, true);
    if (cui_pagination(ui, n->key, page, pages)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* ------------------------------------------------------------------------ */
/* Files, signatures and avatars                                            */
/* ------------------------------------------------------------------------ */

static void cui__xml_draw_dropzone(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_dropzone_opts o;
    const char *label = cui__xml_label(doc, n);
    int dropped;
    memset(&o, 0, sizeof o);
    o.title = cui__xml_attr_text(doc, n, "title", label && *label ? label : NULL);
    o.description = cui__xml_attr_text(doc, n, "description", NULL);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.accept = cui__xml_attr_text(doc, n, "accept", NULL);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.compact = cui__xml_attr_bool(doc, n, "compact", false);
    if (disabled) cui_push_disabled(ui, true);
    dropped = cui_dropzone_ex(ui, n->key, &o);
    if (disabled) cui_pop_disabled(ui);
    if (dropped > 0) {
        n->reported = dropped;
        cui__xml_fire(doc, n, "onchanged");
    } else if (dropped < 0) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
}

static void cui__xml_draw_signature_pad(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_signature_opts o;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.placeholder = cui__xml_attr_text(doc, n, "placeholder", NULL);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_signature_pad_ex(ui, n->key, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* The picture the pad took: the element keeps it until the document is freed. */
typedef struct { cui_texture image; } cui__xml_upload_image;

static void cui__xml_upload_image_free(cui_ctx *ui, void *data) {
    cui__xml_upload_image *s = data;
    if (s->image) cui_image_destroy(ui, s->image);
}

static void cui__xml_draw_avatar_upload(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_id id = cui_make_id(ui, n->key);
    cui__xml_upload_image *s = cui_state(ui, id, sizeof *s);
    int result;
    if (!s) return;
    cui_state_on_free(ui, id, cui__xml_upload_image_free);
    if (disabled) cui_push_disabled(ui, true);
    result = cui_avatar_upload(ui, n->key, cui__xml_label(doc, n), &s->image, cui__xml_attr_num(doc, n, "size", 88));
    if (disabled) cui_pop_disabled(ui);
    if (result > 0) cui__xml_changed_hook(doc, n);
    else if (result < 0) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
}

/* ---- table of contents --------------------------------------------------- */

/* <Toc Scroll="reader"> follows the scroll area named reader: on a click it
 * scrolls it, and it marks the section in view. The scroll is found by that
 * name, and its id (which the document knows) is what the component needs: the
 * document's own scopes would hash the name to something else. */
static void cui__xml_draw_toc(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_toc_item items[64];
    const char *target = cui__xml_attr_text(doc, n, "scroll", NULL);
    cui_xml_node *scroll;
    cui_id sid;
    int count = 0, i;
    if (!target || !*target) return;
    scroll = cui_xml_find(doc, target);
    if (!scroll) return;
    sid = cui_xml_node_id(scroll);
    if (!sid) return;
    for (i = 0; i < n->kid_count && count < 64; i++) {
        cui_xml_node *kid = n->kids[i];
        const char *label;
        if (kid->tag_id != CUI__XML_TAG_TOCITEM || !kid->shown) continue;
        label = cui__xml_label(doc, kid);
        items[count].title = label ? label : "";
        items[count].anchor = cui__xml_attr_text(doc, kid, "anchor", items[count].title);
        items[count].level = (int)cui__xml_attr_num(doc, kid, "level", 0);
        count++;
    }
    if (!count) return;
    if (disabled) cui_push_disabled(ui, true);
    cui_toc_at(ui, sid, items, count);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_toc_anchor(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    const char *anchor = cui__xml_attr_text(doc, n, "anchor", NULL);
    const char *label = cui__xml_label(doc, n);
    if (!anchor || !*anchor) anchor = label;
    if (!anchor || !*anchor) return;
    cui_toc_anchor(ui, anchor);
}

static void cui__xml_draw_progress(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_progress_opts o;
    memset(&o, 0, sizeof o);
    o.indeterminate = cui__xml_attr_bool(doc, n, "indeterminate", false);
    o.tone = cui__xml_tone_of(doc, n, CUI_TONE_NEUTRAL);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.show_value = cui__xml_attr_bool(doc, n, "showvalue", false);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.dither = cui__xml_attr_bool(doc, n, "dither", false);
    cui_progress_ex(ui, cui__xml_attr_num(doc, n, "value", 0), &o);
}

static void cui__xml_draw_badge(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_badge_opts o;
    memset(&o, 0, sizeof o);
    o.tone = cui__xml_tone_of(doc, n, CUI_TONE_NEUTRAL);
    o.dot = cui__xml_attr_bool(doc, n, "dot", false);
    o.solid = cui__xml_attr_bool(doc, n, "solid", false);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.size = cui__xml_control_size_of(doc, n, CUI_SIZE_MD);
    cui_badge_ex(ui, cui__xml_label(doc, n), &o);
}

static void cui__xml_draw_avatar(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_avatar_opts o;
    memset(&o, 0, sizeof o);
    o.size = cui__xml_attr_num(doc, n, "size", 32);
    o.status = cui__xml_attr_text(doc, n, "status", NULL);
    o.square = cui__xml_attr_bool(doc, n, "square", false);
    cui_avatar_ex(ui, cui__xml_label(doc, n), &o);
}

static void cui__xml_draw_metric(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_metric_opts o;
    const float *spark = NULL;
    memset(&o, 0, sizeof o);
    o.delta = cui__xml_attr_num(doc, n, "delta", 0);
    o.delta_label = cui__xml_attr_text(doc, n, "deltalabel", NULL);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FIT);
    o.lower_is_better = cui__xml_attr_bool(doc, n, "lowerisbetter", false);
    o.spark_count = cui__xml_attr_numbers(doc, n, "values", &spark);
    o.spark = spark;
    cui_metric_ex(ui, cui__xml_attr_text(doc, n, "label", ""), cui__xml_attr_text(doc, n, "value", ""), &o);
}

static void cui__xml_draw_alert(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_alert_opts o;
    bool *open = cui__xml_slot_bool(doc, n);
    memset(&o, 0, sizeof o);
    o.description = cui__xml_attr_text(doc, n, "description", NULL);
    o.tone = cui__xml_tone_of(doc, n, CUI_TONE_INFO);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.dismissible = cui__xml_attr_bool(doc, n, "dismissible", false);
    if (!n->seen) *open = cui__xml_attr_bool(doc, n, "open", true);
    if (!*open) {
        n->shown = false;
        return;
    }
    cui__xml_record_self(n, ui);
    if (!cui_alert_ex(ui, cui__xml_label(doc, n), &o)) {
        *open = false;
        n->shown = false;
        cui__xml_fire(doc, n, "onchanged");
    }
}

static void cui__xml_draw_banner(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_banner_opts o;
    bool *open = cui__xml_slot_bool(doc, n);
    memset(&o, 0, sizeof o);
    o.tone = cui__xml_tone_of(doc, n, CUI_TONE_INFO);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.action = cui__xml_attr_text(doc, n, "action", NULL);
    o.dismissible = cui__xml_attr_bool(doc, n, "dismissible", false);
    if (!n->seen) *open = cui__xml_attr_bool(doc, n, "open", true);
    if (!*open) {
        n->shown = false;
        return;
    }
    cui__xml_record_self(n, ui);
    if (cui_banner_ex(ui, cui__xml_label(doc, n), open, &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onaction");
    }
    if (!*open) n->shown = false;
}

static void cui__xml_draw_empty_state(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_empty_opts o;
    memset(&o, 0, sizeof o);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.description = cui__xml_attr_text(doc, n, "description", NULL);
    o.action = cui__xml_attr_text(doc, n, "action", NULL);
    if (cui_empty_state_ex(ui, cui__xml_label(doc, n), &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onaction");
    }
}

static void cui__xml_draw_tree(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_tree_opts o;
    bool *open = cui__xml_slot_bool(doc, n);
    memset(&o, 0, sizeof o);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.leaf = cui__xml_attr_bool(doc, n, "leaf", false);
    o.selected = cui__xml_attr_bool(doc, n, "selected", false);
    if (!n->seen) *open = cui__xml_attr_bool(doc, n, "open", false);
    if (disabled) cui_push_disabled(ui, true);
    cui__xml_record_self(n, ui);
    if (cui_tree_begin(ui, cui__xml_label(doc, n), open, &o)) {
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_tree_end(ui);
    }
    if (cui_tree_clicked(ui)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_accordion(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    bool *open = cui__xml_slot_bool(doc, n);
    if (!n->seen) *open = cui__xml_attr_bool(doc, n, "open", false);
    if (disabled) cui_push_disabled(ui, true);
    cui__xml_record_self(n, ui);
    if (cui_accordion_begin(ui, cui__xml_label(doc, n), open)) {
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_accordion_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_list_item(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_list_item_opts o;
    memset(&o, 0, sizeof o);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.subtitle = cui__xml_attr_text(doc, n, "subtitle", NULL);
    o.trailing = cui__xml_attr_text(doc, n, "trailing", NULL);
    o.selected = cui__xml_attr_bool(doc, n, "selected", false);
    o.avatar = cui__xml_attr_text(doc, n, "avatar", NULL);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_list_item_ex(ui, cui__xml_label(doc, n), &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
    if (disabled) cui_pop_disabled(ui);
}

/* ---- table ---- */

static const char *cui__xml_table_sort_value(cui_xml *doc, cui_xml_node *row, int column) {
    for (int i = 0; i < row->kid_count; i++) {
        cui_xml_node *cell = row->kids[i];
        if (cell->tag_id == CUI__XML_TAG_CELL && column-- == 0)
            return cui__xml_attr_text(doc, cell, "sortvalue", cui__xml_label(doc, cell));
    }
    return "";
}

typedef struct {
    int index;
    const char *value;
} cui__xml_table_sort_row;

static void cui__xml_draw_table(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_table_column *cols;
    int col_count = 0, i;
    cui_table_opts o;
    bool open;
    if (!n->cache) {
        for (i = 0; i < n->kid_count; i++)
            if (n->kids[i]->tag_id == CUI__XML_TAG_TABLECOLUMN) col_count++;
        cols = cui__xml_alloc(sizeof *cols * (size_t)(col_count ? col_count : 1));
        if (!cols) return;
        memset(cols, 0, sizeof *cols * (size_t)(col_count ? col_count : 1));
        col_count = 0;
        for (i = 0; i < n->kid_count; i++) {
            cui_xml_node *kid = n->kids[i];
            if (kid->tag_id != CUI__XML_TAG_TABLECOLUMN) continue;
            cols[col_count].title = cui__xml_attr_raw(kid, "text");
            cols[col_count].width = cui__xml_attr_size(doc, kid, "width", CUI_FILL);
            cols[col_count].align = cui__xml_text_align_of(doc, kid, CUI_TEXT_START);
            cols[col_count].sortable = cui__xml_attr_bool(doc, kid, "sortable", false);
            col_count++;
        }
        n->cache = cols;
        n->cache_free = cui__xml_free_cache;
    }
    cols = n->cache;
    {
        /* count again: the cache holds the columns, not the count */
        col_count = 0;
        for (i = 0; i < n->kid_count; i++)
            if (n->kids[i]->tag_id == CUI__XML_TAG_TABLECOLUMN) col_count++;
    }
    memset(&o, 0, sizeof o);
    o.height = cui__xml_attr_size(doc, n, "height", CUI_FIT);
    o.striped = cui__xml_attr_bool(doc, n, "striped", false);
    o.compact = cui__xml_attr_bool(doc, n, "compact", false);
    o.selectable = cui__xml_attr_bool(doc, n, "selectable", false);
    if (cui__xml_attr_raw(n, "sortcolumn")) {
        /* a header that shows which column the rows are ordered by; a click
         * writes the new order here, for OnSort to read */
        cui_table_sort *sort = n->cache2;
        if (!sort) {
            sort = cui__xml_alloc(sizeof *sort);
            if (sort) {
                memset(sort, 0, sizeof *sort);
                sort->column = (int)cui__xml_attr_num(doc, n, "sortcolumn", 0);
                sort->descending = cui__xml_attr_bool(doc, n, "sortdescending", false);
                n->cache2 = sort;
                n->cache2_free = cui__xml_free_cache;
            }
        }
        o.sort = sort;
    }
    if (disabled) cui_push_disabled(ui, true);
    cui__xml_record_self(n, ui);
    open = cui_table_begin_ex(ui, n->key, cols, col_count, &o);
    if (open) {
        cui__xml_table_sort_row *order = NULL;
        if (o.sort && cui__xml_attr_bool(doc, n, "autosort", false))
            order = cui__frame_alloc(ui, (size_t)n->kid_count * sizeof *order);
        if (order) {
            for (i = 0; i < n->kid_count; i++) {
                const char *value = cui__xml_table_sort_value(doc, n->kids[i], o.sort->column);
                char *key = value ? cui__frame_strdup(ui, value, -1) : NULL;
                int j = i;
                while (j > 0) {
                    int cmp = strcmp(order[j - 1].value, key ? key : "");
                    if (o.sort->descending ? cmp >= 0 : cmp <= 0) break;
                    order[j] = order[j - 1];
                    j--;
                }
                order[j] = (cui__xml_table_sort_row){ i, key ? key : "" };
            }
        }
        for (i = 0; i < n->kid_count; i++) {
            int row_index = order ? order[i].index : i;
            cui_xml_node *row = n->kids[row_index];
            int c;
            if (row->tag_id != CUI__XML_TAG_TABLEROW || !row->shown) continue;
            if (cui_table_row(ui, cui__xml_attr_bool(doc, row, "selected", false))) {
                n->reported = row_index;
                cui__xml_fire(doc, row, "onclicked");
            }
            for (c = 0; c < row->kid_count; c++) {
                cui_xml_node *cell = row->kids[c];
                const char *text;
                if (cell->tag_id != CUI__XML_TAG_CELL) continue;
                text = cui__xml_label(doc, cell);
                /* cui_table_cell_text starts the cell itself: a cell of text
                 * must not start it twice, or it would take two columns */
                if (cell->kid_count) {
                    cui_table_cell(ui);
                    cui__xml_draw_kids(doc, cell, ui, disabled);
                } else {
                    cui_table_cell_text(ui, text);
                }
            }
        }
        cui_table_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
}

/* ---- charts ---- */

static void cui__xml_draw_series_chart(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool bars) {
    cui_series *series;
    cui_chart_opts o;
    int count = 0, i;
    if (!n->cache) {
        for (i = 0; i < n->kid_count; i++)
            if (n->kids[i]->tag_id == CUI__XML_TAG_SERIES) count++;
        series = cui__xml_alloc(sizeof *series * (size_t)(count ? count : 1));
        if (!series) return;
        memset(series, 0, sizeof *series * (size_t)(count ? count : 1));
        count = 0;
        for (i = 0; i < n->kid_count; i++) {
            cui_xml_node *kid = n->kids[i];
            const float *values = NULL;
            int value_count;
            if (kid->tag_id != CUI__XML_TAG_SERIES) continue;
            value_count = cui__xml_attr_numbers(doc, kid, "values", &values);
            series[count].name = cui__xml_attr_raw(kid, "text");
            series[count].values = values;
            series[count].count = value_count;
            series[count].color = cui__xml_attr_color(doc, kid, "color", (cui_color){ 0, 0, 0, 0 });
            count++;
        }
        n->cache = series;
        n->cache_free = cui__xml_free_cache;
    }
    series = n->cache;
    count = 0;
    for (i = 0; i < n->kid_count; i++)
        if (n->kids[i]->tag_id == CUI__XML_TAG_SERIES) count++;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.height = cui__xml_attr_num(doc, n, "height", 220);
    if (cui__xml_attr_raw(n, "labels")) {
        int label_count = 0;
        const char **labels = cui__xml_attr_list(doc, n, "labels", &label_count, false);
        if (label_count) {
            o.labels = labels;
            o.label_count = label_count;
        }
    }
    o.area = cui__xml_attr_bool(doc, n, "area", false);
    o.smooth = cui__xml_attr_bool(doc, n, "smooth", false);
    o.no_grid = cui__xml_attr_bool(doc, n, "nogrid", false);
    o.no_legend = cui__xml_attr_bool(doc, n, "nolegend", false);
    o.stacked = cui__xml_attr_bool(doc, n, "stacked", false);
    o.dither = cui__xml_attr_bool(doc, n, "dither", false);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    if (bars) cui_bar_chart_ex(ui, n->key, series, count, &o);
    else cui_line_chart_ex(ui, n->key, series, count, &o);
}

/* ---- donut ---- */

typedef struct cui__xml_donut_cache {
    float *values;
    const char **labels;
    int count;
} cui__xml_donut_cache;

static void cui__xml_free_donut(void *p) {
    cui__xml_donut_cache *cache = p;
    free(cache->values);
    free(cache->labels);
    free(cache);
}

static void cui__xml_draw_donut(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui__xml_donut_cache *cache = n->cache;
    cui_chart_opts o;
    int i;
    if (!cache) {
        int count = 0;
        for (i = 0; i < n->kid_count; i++)
            if (n->kids[i]->tag_id == CUI__XML_TAG_SLICE) count++;
        cache = cui__xml_alloc(sizeof *cache);
        if (!cache) return;
        memset(cache, 0, sizeof *cache);
        cache->values = cui__xml_alloc(sizeof *cache->values * (size_t)(count ? count : 1));
        cache->labels = cui__xml_alloc(sizeof *cache->labels * (size_t)(count ? count : 1));
        if (!cache->values || !cache->labels) {
            cui__xml_free_donut(cache);
            return;
        }
        for (i = 0; i < n->kid_count; i++) {
            cui_xml_node *kid = n->kids[i];
            if (kid->tag_id != CUI__XML_TAG_SLICE) continue;
            cache->values[cache->count] = cui__xml_attr_num(doc, kid, "value", 1);
            cache->labels[cache->count] = cui__xml_attr_raw(kid, "text");
            cache->count++;
        }
        n->cache = cache;
        n->cache_free = cui__xml_free_donut;
    }
    if (!cache->count) return;
    memset(&o, 0, sizeof o);
    /* the donut fits its content (the ring and the legend) unless a width is
     * written: with no width the row stays adaptive, as a call in C is */
    if (cui__xml_attr_raw(n, "width")) o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.height = cui__xml_attr_num(doc, n, "height", 220);
    o.no_legend = cui__xml_attr_bool(doc, n, "nolegend", false);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    cui_donut_chart_ex(ui, n->key, cache->values, cache->labels, cache->count, &o);
}

static void cui__xml_draw_pattern_meter(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_meter_part *parts;
    cui_pattern_meter_opts o;
    int count = 0, i;
    if (!n->cache) {
        for (i = 0; i < n->kid_count; i++)
            if (n->kids[i]->tag_id == CUI__XML_TAG_PART) count++;
        parts = cui__xml_alloc(sizeof *parts * (size_t)(count ? count : 1));
        if (!parts) return;
        memset(parts, 0, sizeof *parts * (size_t)(count ? count : 1));
        count = 0;
        for (i = 0; i < n->kid_count; i++) {
            cui_xml_node *kid = n->kids[i];
            if (kid->tag_id != CUI__XML_TAG_PART) continue;
            parts[count].name = cui__xml_attr_raw(kid, "text");
            parts[count].value = cui__xml_attr_num(doc, kid, "value", 0);
            parts[count].density = cui__xml_attr_num(doc, kid, "density", 0);
            parts[count].color = cui__xml_attr_color(doc, kid, "color", (cui_color){ 0, 0, 0, 0 });
            count++;
        }
        n->cache = parts;
        n->cache_free = cui__xml_free_cache;
    }
    parts = n->cache;
    count = 0;
    for (i = 0; i < n->kid_count; i++)
        if (n->kids[i]->tag_id == CUI__XML_TAG_PART) count++;
    memset(&o, 0, sizeof o);
    o.total = cui__xml_attr_num(doc, n, "total", 0);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.unit = cui__xml_attr_text(doc, n, "unit", NULL);
    o.free_label = cui__xml_attr_text(doc, n, "freelabel", NULL);
    o.grayscale = cui__xml_attr_bool(doc, n, "grayscale", false);
    o.no_legend = cui__xml_attr_bool(doc, n, "nolegend", false);
    o.height = cui__xml_attr_num(doc, n, "height", 18);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    n->reported = cui_pattern_meter_ex(ui, n->key, parts, count, &o);
}

/* ---- pictures ---- */

typedef struct cui__xml_image_cache {
    cui_ctx *ui;
    cui_texture texture;
    int width, height;
} cui__xml_image_cache;

static void cui__xml_free_image(void *p) {
    cui__xml_image_cache *cache = p;
    if (cache->texture && cache->ui) cui_image_destroy(cache->ui, cache->texture);
    free(cache);
}

static void cui__xml_draw_image(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui__xml_image_cache *cache = n->cache;
    const char *source = cui__xml_attr_raw(n, "source");
    if (!cache) {
        cache = cui__xml_alloc(sizeof *cache);
        if (!cache) return;
        memset(cache, 0, sizeof *cache);
        cache->ui = ui;
        n->cache = cache;
        n->cache_free = cui__xml_free_image;
        if (source && *source) {
            const char *path = cui_xml_resolve_path(doc, source);
            cache->texture = cui_image_load(ui, path, &cache->width, &cache->height);
            if (!cache->texture) cui__xml_warn(doc, n, "Image could not read \"%s\"", source);
        }
    }
    if (!cache->texture) return;
    {
        cui_image_opts o;
        memset(&o, 0, sizeof o);
        o.width = cui__xml_attr_size(doc, n, "width", CUI_FIT);
        o.height = cui__xml_attr_size(doc, n, "height", CUI_FIT);
        o.radius = cui__xml_attr_num(doc, n, "radius", 0);
        o.tint = cui__xml_attr_color(doc, n, "tint", (cui_color){ 0, 0, 0, 0 });
        cui_image_ex(ui, cache->texture, &o);
    }
}

/* ---- document tabs ---- */

static void cui__xml_draw_doc_tabs(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_doc_tab *tabs;
    int count = 0, i, *selected = cui__xml_slot_int(doc, n);
    cui_doc_tabs_opts o;
    cui_doc_tabs_result result;
    if (!n->cache) {
        for (i = 0; i < n->kid_count; i++)
            if (n->kids[i]->tag_id == CUI__XML_TAG_TAB) count++;
        tabs = cui__xml_alloc(sizeof *tabs * (size_t)(count ? count : 1));
        if (!tabs) return;
        memset(tabs, 0, sizeof *tabs * (size_t)(count ? count : 1));
        count = 0;
        for (i = 0; i < n->kid_count; i++) {
            cui_xml_node *kid = n->kids[i];
            if (kid->tag_id != CUI__XML_TAG_TAB) continue;
            tabs[count].title = cui__xml_attr_raw(kid, "text");
            tabs[count].icon = cui__xml_icon_named(doc, kid, "icon");
            tabs[count].tooltip = cui__xml_attr_raw(kid, "tooltip");
            tabs[count].dirty = cui__xml_attr_bool(doc, kid, "dirty", false);
            count++;
        }
        n->cache = tabs;
        n->cache_free = cui__xml_free_cache;
    }
    tabs = n->cache;
    count = 0;
    for (i = 0; i < n->kid_count; i++)
        if (n->kids[i]->tag_id == CUI__XML_TAG_TAB) count++;
    memset(&o, 0, sizeof o);
    o.shortcuts = cui__xml_attr_bool(doc, n, "shortcuts", false);
    o.no_close = cui__xml_attr_bool(doc, n, "noclose", false);
    o.no_reorder = cui__xml_attr_bool(doc, n, "noreorder", false);
    cui__xml_record_self(n, ui);
    result = cui_doc_tabs_ex(ui, n->key, tabs, count, selected, &o);
    if (result.changed) cui__xml_changed_hook(doc, n);
    if (result.close >= 0) {
        n->reported = result.close;
        cui__xml_fire(doc, n, "onclosed");
    }
}

/* ---- toolbar, status bar, menu bar ---- */

static void cui__xml_draw_toolbar_button(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled, bool primary) {
    cui_tool_opts o;
    memset(&o, 0, sizeof o);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.tooltip = cui__xml_attr_text(doc, n, "tooltip", NULL);
    o.shortcut = cui__xml_attr_text(doc, n, "shortcut", NULL);
    o.disabled = disabled || cui__xml_attr_bool(doc, n, "disabled", false);
    o.tone = cui__xml_tone_of(doc, n, CUI_TONE_NEUTRAL);
    o.flyout = cui__xml_attr_bool(doc, n, "flyout", false);
    if (cui__xml_attr_bool(doc, n, "toggle", false)) {
        o.toggle = cui__xml_slot_bool(doc, n);
        if (!n->seen) *o.toggle = cui__xml_attr_bool(doc, n, "on", false);
    }
    if (primary) {
        if (cui_toolbar_primary_ex(ui, cui__xml_icon(doc, n, "check"), &o)) {
            n->reported = 0;
            cui__xml_fire(doc, n, "onclicked");
        }
        return;
    }
    if (cui_toolbar_button_ex(ui, cui__xml_icon(doc, n, NULL), &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
}

static void cui__xml_draw_menu_bar(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_menu_bar_opts o;
    int i;
    memset(&o, 0, sizeof o);
    o.no_border = cui__xml_attr_bool(doc, n, "noborder", false);
    cui__xml_record_self(n, ui);
    cui_menu_bar_begin(ui, n->key, &o);
    for (i = 0; i < n->kid_count; i++) {
        cui_xml_node *menu = n->kids[i];
        cui_menu_bar_menu_opts m;
        if (menu->tag_id != CUI__XML_TAG_MENUBARMENU || !menu->shown) {
            cui__xml_draw(doc, menu, ui, disabled);
            continue;
        }
        memset(&m, 0, sizeof m);
        m.width = cui__xml_attr_num(doc, menu, "width", 0);
        m.disabled = disabled || cui__xml_attr_bool(doc, menu, "disabled", false);
        if (cui_menu_bar_menu_begin(ui, cui__xml_label(doc, menu), &m)) {
            cui__xml_draw_kids(doc, menu, ui, disabled);
            cui_menu_bar_menu_end(ui);
        }
    }
    cui_menu_bar_end(ui);
}

static void cui__xml_draw_menu(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_menu_opts o;
    memset(&o, 0, sizeof o);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.variant = cui__xml_variant_of(doc, n, CUI_BUTTON_DEFAULT);
    o.placement = cui__xml_placement_of(doc, n, CUI_PLACE_BOTTOM_START);
    o.width = cui__xml_attr_num(doc, n, "width", 0);
    o.icon_only = cui__xml_attr_bool(doc, n, "icononly", false);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_menu_begin(ui, cui__xml_label(doc, n), &o)) {
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_menu_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_menu_item(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_menu_item_opts o;
    memset(&o, 0, sizeof o);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.shortcut = cui__xml_attr_text(doc, n, "shortcut", NULL);
    o.danger = cui__xml_attr_bool(doc, n, "danger", false);
    o.disabled = disabled || cui__xml_attr_bool(doc, n, "disabled", false);
    o.checked = cui__xml_attr_bool(doc, n, "checked", false);
    if (cui_menu_item_ex(ui, cui__xml_label(doc, n), &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
}

static void cui__xml_draw_status_item(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, int kind) {
    cui_status_item_opts o;
    memset(&o, 0, sizeof o);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.tone = cui__xml_tone_of(doc, n, CUI_TONE_NEUTRAL);
    o.tooltip = cui__xml_attr_text(doc, n, "tooltip", NULL);
    o.disabled = cui__xml_attr_bool(doc, n, "disabled", false);
    if (kind == 0) {
        cui_status_bar_text_ex(ui, cui__xml_label(doc, n), &o);
    } else if (cui_status_bar_item_ex(ui, cui__xml_label(doc, n), &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
}

/* ---- dialog ---- */

static void cui__xml_draw_dialog(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_dialog_opts o;
    bool *open = cui__xml_slot_bool(doc, n);
    memset(&o, 0, sizeof o);
    o.description = cui__xml_attr_text(doc, n, "description", NULL);
    o.width = cui__xml_attr_num(doc, n, "width", 440);
    o.no_close_button = cui__xml_attr_bool(doc, n, "noclosebutton", false);
    o.persistent = cui__xml_attr_bool(doc, n, "persistent", false);
    o.draggable = cui__xml_attr_bool(doc, n, "draggable", false);
    o.resizable = cui__xml_attr_bool(doc, n, "resizable", false);
    if (!n->seen) *open = cui__xml_attr_bool(doc, n, "open", false);
    if (!*open) {
        n->shown = false;
        return;
    }
    cui__xml_record_self(n, ui);
    if (cui_dialog_begin(ui, cui__xml_attr_text(doc, n, "title", cui__xml_label(doc, n)), open, &o)) {
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_dialog_end(ui);
    }
    if (!*open) n->shown = false;
}

/* ---- carousel ---- */

static void cui__xml_draw_carousel(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    int *index = cui__xml_slot_int(doc, n);
    int count = 0, i;
    cui_carousel_opts o;
    for (i = 0; i < n->kid_count; i++)
        if (n->kids[i]->tag_id == CUI__XML_TAG_SLIDE) count++;
    if (!count) return;
    memset(&o, 0, sizeof o);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.auto_advance = cui__xml_attr_num(doc, n, "autoadvance", 0);
    o.loop = !cui__xml_attr_bool(doc, n, "noloop", false);
    if (!n->seen) *index = (int)cui__xml_attr_num(doc, n, "value", *index);
    cui__xml_record_self(n, ui);
    if (cui_carousel_begin(ui, n->key, count, index, &o)) {
        for (i = 0; i < n->kid_count; i++) {
            cui_xml_node *slide = n->kids[i];
            if (slide->tag_id != CUI__XML_TAG_SLIDE) continue;
            cui_carousel_slide_begin(ui);
            cui__xml_draw_kids(doc, slide, ui, disabled);
            cui_carousel_slide_end(ui);
        }
        cui_carousel_end(ui);
    }
}

/* ------------------------------------------------------------------------ */
/* One element                                                              */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
/* Lists written as one attribute: Icons="sun|moon", Labels="Jan|Feb"        */
/* ------------------------------------------------------------------------ */

typedef struct cui__xml_list_cache {
    char *text;              /* the attribute, split on its bars */
    const char **items;
    int count;
} cui__xml_list_cache;

static void cui__xml_free_list_cache(void *p) {
    cui__xml_list_cache *cache = p;
    free(cache->text);
    free(cache->items);
    free(cache);
}

/* Splits a copy of `attr` on '|'. `icons` turns each entry into its glyph, so
 * a list of icon names is read the way the Icon attribute is. */
static const char **cui__xml_attr_list(cui_xml *doc, cui_xml_node *n, const char *attr, int *count, bool icons) {
    cui__xml_list_cache *cache = n->cache2;
    if (!cache) {
        const char *value = cui__xml_attr_text(doc, n, attr, NULL);
        char *p, *text;
        int n_items = 1;
        if (!value || !*value) {
            *count = 0;
            return NULL;
        }
        cache = cui__xml_alloc(sizeof *cache);
        if (!cache) {
            *count = 0;
            return NULL;
        }
        memset(cache, 0, sizeof *cache);
        cache->text = cui__xml_strdup(value);
        if (!cache->text) {
            free(cache);
            *count = 0;
            return NULL;
        }
        for (p = cache->text; *p; p++)
            if (*p == '|') n_items++;
        cache->items = cui__xml_alloc(sizeof *cache->items * (size_t)n_items);
        if (!cache->items) {
            free(cache->text);
            free(cache);
            *count = 0;
            return NULL;
        }
        text = cache->text;
        while (text && *text) {
            char *bar = strchr(text, '|');
            char *start = text, *end;
            if (bar) *bar = 0;
            while (*start == ' ' || *start == '\t') start++;
            end = start + strlen(start);
            while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) *--end = 0;
            if (*start) cache->items[cache->count++] = icons ? cui__xml_icon_glyph(start) : start;
            text = bar ? bar + 1 : NULL;
        }
        n->cache2 = cache;
        n->cache2_free = cui__xml_free_list_cache;
    }
    *count = cache->count;
    return cache->items;
}

/* ------------------------------------------------------------------------ */
/* Buttons that copy, hold and sit connected                                 */
/* ------------------------------------------------------------------------ */

static void cui__xml_draw_button_group(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    if (disabled) cui_push_disabled(ui, true);
    cui_button_group_begin(ui);
    cui__xml_draw_kids(doc, n, ui, disabled);
    cui_button_group_end(ui);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_copy_button(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    const char *copy = cui__xml_attr_text(doc, n, "copy", NULL);
    const char *text = cui__xml_label(doc, n);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_copy_button(ui, n->key, copy && *copy ? copy : text)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_hold_button(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_hold_opts o;
    memset(&o, 0, sizeof o);
    o.duration = cui__xml_attr_num(doc, n, "duration", 0);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.variant = cui__xml_variant_of(doc, n, CUI_BUTTON_DEFAULT);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_hold_button_ex(ui, cui__xml_label(doc, n), &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
    if (disabled) cui_pop_disabled(ui);
}

/* ------------------------------------------------------------------------ */
/* Fields: tags, one time codes, passwords, dates                            */
/* ------------------------------------------------------------------------ */

static void cui__xml_draw_tag_input(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_tag_opts o;
    char *buf;
    size_t cap = 0;
    memset(&o, 0, sizeof o);
    buf = cui__xml_slot_text(doc, n, &cap, (size_t)cui__xml_attr_num(doc, n, "maxlength", 256));
    if (!buf || !cap) return;
    if (!n->seen) {
        const char *value = cui__xml_attr_text(doc, n, "value", NULL);
        if (value && !buf[0]) snprintf(buf, cap, "%s", value);
    }
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.placeholder = cui__xml_attr_text(doc, n, "placeholder", NULL);
    o.max_tags = (int)cui__xml_attr_num(doc, n, "maxtags", 0);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_tag_input_ex(ui, n->key, buf, cap, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_otp(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    int length = (int)cui__xml_attr_num(doc, n, "length", 6);
    char *buf;
    size_t cap = 0;
    if (length < 1) length = 6;
    buf = cui__xml_slot_text(doc, n, &cap, (size_t)length + 1);
    if (!buf || cap <= (size_t)length) return;
    if (!n->seen) {
        const char *value = cui__xml_attr_text(doc, n, "value", NULL);
        if (value && !buf[0]) {
            size_t k = 0;
            for (; value[k] && k < (size_t)length; k++) buf[k] = value[k];
            buf[k] = 0;
        }
    }
    if (disabled) cui_push_disabled(ui, true);
    if (cui_otp_input(ui, n->key, buf, length)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onsubmitted");
    }
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_password_strength(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    size_t cap = 0;
    char *buf = cui__xml_slot_text(doc, n, &cap, 0);
    cui_password_strength(ui, buf ? buf : "");
}

typedef struct cui__xml_date_cache {
    cui_date value;
} cui__xml_date_cache;

static void cui__xml_draw_date_picker(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_date_opts o;
    cui__xml_date_cache *cache = n->cache;
    if (!cache) {
        cache = cui__xml_alloc(sizeof *cache);
        if (!cache) return;
        memset(cache, 0, sizeof *cache);
        cache->value.year = (int)cui__xml_attr_num(doc, n, "year", 2026);
        cache->value.month = (int)cui__xml_attr_num(doc, n, "month", 1);
        cache->value.day = (int)cui__xml_attr_num(doc, n, "day", 1);
        n->cache = cache;
        n->cache_free = cui__xml_free_cache;
    }
    memset(&o, 0, sizeof o);
    o.label = cui__xml_attr_text(doc, n, "label", NULL);
    o.placeholder = cui__xml_attr_text(doc, n, "placeholder", NULL);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FIT);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_date_picker_ex(ui, n->key, &cache->value, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* ------------------------------------------------------------------------ */
/* Choice cards                                                             */
/* ------------------------------------------------------------------------ */

typedef struct cui__xml_choices_cache {
    cui_choice *items;
    int count;
} cui__xml_choices_cache;

static void cui__xml_free_choices(void *p) {
    free(((cui__xml_choices_cache *)p)->items);
    free(p);
}

static void cui__xml_draw_radio_cards(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui__xml_choices_cache *cache = n->cache;
    cui_radio_cards_opts o;
    int *index = cui__xml_slot_int(doc, n);
    int i;
    if (!cache) {
        int count = 0;
        for (i = 0; i < n->kid_count; i++)
            if (!n->kids[i]->comment && n->kids[i]->tag_id == CUI__XML_TAG_CHOICE) count++;
        cache = cui__xml_alloc(sizeof *cache);
        if (!cache) return;
        memset(cache, 0, sizeof *cache);
        cache->items = cui__xml_alloc(sizeof *cache->items * (size_t)(count ? count : 1));
        if (!cache->items) {
            free(cache);
            return;
        }
        memset(cache->items, 0, sizeof *cache->items * (size_t)(count ? count : 1));
        for (i = 0; i < n->kid_count; i++) {
            cui_xml_node *kid = n->kids[i];
            if (kid->comment || kid->tag_id != CUI__XML_TAG_CHOICE) continue;
            cache->items[cache->count].title = cui__xml_attr_raw(kid, "text");
            cache->items[cache->count].description = cui__xml_attr_raw(kid, "description");
            cache->items[cache->count].icon = cui__xml_icon_glyph(cui__xml_attr_raw(kid, "icon"));
            cache->items[cache->count].badge = cui__xml_attr_raw(kid, "badge");
            if (!cache->items[cache->count].title) cache->items[cache->count].title = kid->text;
            cache->count++;
        }
        n->cache = cache;
        n->cache_free = cui__xml_free_choices;
    }
    if (!cache->count) return;
    memset(&o, 0, sizeof o);
    o.columns = (int)cui__xml_attr_num(doc, n, "columns", 0);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_radio_cards_ex(ui, n->key, cache->items, cache->count, index, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* ------------------------------------------------------------------------ */
/* Loading, states and activity                                             */
/* ------------------------------------------------------------------------ */

static void cui__xml_draw_skeleton_group(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_skeleton_group_opts o;
    memset(&o, 0, sizeof o);
    o.dither = cui__xml_attr_bool(doc, n, "dither", false);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.gap = cui__xml_attr_num(doc, n, "gap", 0);
    if (disabled) cui_push_disabled(ui, true);
    cui_skeleton_group_begin_ex(ui, &o);
    cui__xml_draw_kids(doc, n, ui, disabled);
    cui_skeleton_group_end(ui);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_status_spinner(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    static const char *const states[] = { "idle", "busy", "success", "error" };
    cui_status_spinner_opts o;
    int status = cui__xml_choice_of(doc, n, "status", states, 4, CUI_STATUS_IDLE);
    memset(&o, 0, sizeof o);
    o.size = cui__xml_attr_num(doc, n, "size", 20);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    o.keep = cui__xml_attr_bool(doc, n, "keep", false);
    if (cui_status_spinner_ex(ui, n->key, (cui_status)status, &o)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onclicked");
    }
}

static void cui__xml_draw_timeline_item(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_timeline_opts o;
    memset(&o, 0, sizeof o);
    o.time = cui__xml_attr_text(doc, n, "time", NULL);
    o.description = cui__xml_attr_text(doc, n, "description", NULL);
    o.icon = cui__xml_icon(doc, n, NULL);
    o.tone = cui__xml_tone_of(doc, n, CUI_TONE_NEUTRAL);
    o.last = cui__xml_attr_bool(doc, n, "last", false);
    cui_timeline_item_ex(ui, cui__xml_label(doc, n), &o);
}

static void cui__xml_draw_progress_ring(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_progress_ring(ui, cui__xml_label(doc, n), cui__xml_attr_num(doc, n, "value", 0),
                      cui__xml_attr_num(doc, n, "size", 104));
}

static void cui__xml_draw_avatar_group(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    int count = 0;
    const char **names = cui__xml_attr_list(doc, n, "items", &count, false);
    if (!count) return;
    cui_avatar_group(ui, names, count, (int)cui__xml_attr_num(doc, n, "max", count),
                     cui__xml_attr_num(doc, n, "size", 32));
}

/* ------------------------------------------------------------------------ */
/* Panels over the interface                                                */
/* ------------------------------------------------------------------------ */

static bool cui__xml_draw_popover(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_popover_opts o;
    cui_id id = cui_make_id(ui, n->key);
    bool open;
    memset(&o, 0, sizeof o);
    o.placement = cui__xml_placement_of(doc, n, CUI_PLACE_BOTTOM_START);
    o.width = cui__xml_attr_num(doc, n, "width", 0);
    o.pad = cui__xml_attr_num(doc, n, "pad", 0);
    o.gap = cui__xml_attr_num(doc, n, "gap", 0);
    if (disabled) cui_push_disabled(ui, true);
    open = cui_popover_begin(ui, n->key, &o);
    if (open) {
        cui__xml_record_id(n, ui, id);
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_popover_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
    return open;
}

static void cui__xml_draw_context_menu(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    const char *target = cui__xml_attr_text(doc, n, "target", NULL);
    if (target) {
        cui_id id = cui_xml_node_id(cui_xml_find(doc, target));
        if (!id) return;
        ui->last_id = id;
    }
    if (disabled) cui_push_disabled(ui, true);
    if (cui_context_menu_begin(ui, n->key)) {
        cui__xml_record_self(n, ui);
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_context_menu_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
}

/* ------------------------------------------------------------------------ */
/* A list the user drags into another order                                 */
/* ------------------------------------------------------------------------ */

typedef struct cui__xml_reorder_cache {
    int *order;
    cui_xml_node **rows;
    int count;
} cui__xml_reorder_cache;

static void cui__xml_free_reorder(void *p) {
    cui__xml_reorder_cache *cache = p;
    free(cache->order);
    free(cache->rows);
    free(cache);
}

static void cui__xml_draw_reorder(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui__xml_reorder_cache *cache = n->cache;
    int i;
    if (!cache) {
        int count = 0;
        for (i = 0; i < n->kid_count; i++)
            if (!n->kids[i]->comment && n->kids[i]->tag_id == CUI__XML_TAG_REORDERITEM) count++;
        cache = cui__xml_alloc(sizeof *cache);
        if (!cache) return;
        memset(cache, 0, sizeof *cache);
        cache->count = count;
        cache->order = cui__xml_alloc(sizeof *cache->order * (size_t)(count ? count : 1));
        cache->rows = cui__xml_alloc(sizeof *cache->rows * (size_t)(count ? count : 1));
        if (!cache->order || !cache->rows) {
            cui__xml_free_reorder(cache);
            return;
        }
        count = 0;
        for (i = 0; i < n->kid_count; i++) {
            if (n->kids[i]->comment || n->kids[i]->tag_id != CUI__XML_TAG_REORDERITEM) continue;
            cache->rows[count] = n->kids[i];
            cache->order[count] = count;
            count++;
        }
        n->cache = cache;
        n->cache_free = cui__xml_free_reorder;
    }
    if (!cache->count) return;
    if (disabled) cui_push_disabled(ui, true);
    cui_reorder_begin(ui, n->key, cache->order, cache->count);
    for (i = 0; i < cache->count; i++) {
        int index = cache->order[i];
        if (index < 0 || index >= cache->count) index = i;
        cui_reorder_item_begin(ui, i);
        if (cache->rows[index]->shown) cui__xml_draw_kids(doc, cache->rows[index], ui, disabled);
        cui_reorder_item_end(ui);
    }
    if (cui_reorder_end(ui)) {
        n->reported = 0;
        cui__xml_fire(doc, n, "onchanged");
    }
    if (disabled) cui_pop_disabled(ui);
}

/* ------------------------------------------------------------------------ */
/* Readouts and dot-matrix displays                                         */
/* ------------------------------------------------------------------------ */

static void cui__xml_draw_readout(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_readout_opts o;
    memset(&o, 0, sizeof o);
    o.unit = cui__xml_attr_text(doc, n, "unit", NULL);
    o.detail = cui__xml_attr_text(doc, n, "detail", NULL);
    o.size = cui__xml_control_size_of(doc, n, CUI_SIZE_MD);
    o.tone = cui__xml_tone_of(doc, n, CUI_TONE_NEUTRAL);
    o.accent = cui__xml_attr_bool(doc, n, "accent", false);
    o.align = cui__xml_text_align_of(doc, n, CUI_TEXT_START);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FIT);
    cui_readout_ex(ui, cui__xml_attr_text(doc, n, "label", ""), cui__xml_attr_text(doc, n, "value", ""), &o);
}

/* ---- audio: knobs, faders and meters ------------------------------------ */

/* Numbers read afresh every frame: a meter's levels change as the app plays. */
static int cui__xml_live_numbers(const char *text, float *out, int cap) {
    int n = 0;
    while (text && *text && n < cap) {
        char *end;
        double v = strtod(text, &end);
        if (end == text) break;
        out[n++] = (float)v;
        text = end;
        while (*text == ',' || *text == ' ' || *text == ';') text++;
    }
    return n;
}

static void cui__xml_draw_knob(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_knob_opts o;
    float *value = cui__xml_slot_float(doc, n);
    const char *label = cui__xml_label(doc, n);
    float lo = cui__xml_attr_num(doc, n, "min", 0);
    float hi = cui__xml_attr_num(doc, n, "max", 1);
    memset(&o, 0, sizeof o);
    o.label = label && *label ? label : NULL;
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.unit = cui__xml_attr_text(doc, n, "unit", NULL);
    o.size = cui__xml_attr_num(doc, n, "size", 0);
    o.step = cui__xml_attr_num(doc, n, "step", 0);
    o.reset = cui__xml_attr_num(doc, n, "reset", 0);
    o.bipolar = cui__xml_attr_bool(doc, n, "bipolar", false);
    o.log_scale = cui__xml_attr_bool(doc, n, "logscale", false);
    o.hide_value = cui__xml_attr_bool(doc, n, "hidevalue", false);
    if (!n->seen) *value = cui__xml_attr_num(doc, n, "value", *value);
    if (hi <= lo) hi = lo + 1;
    if (disabled) cui_push_disabled(ui, true);
    if (cui_knob_ex(ui, n->key, value, lo, hi, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_fader(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_fader_opts o;
    float *value = cui__xml_slot_float(doc, n);
    float *meter = cui__frame_alloc(ui, sizeof *meter * 8);
    float *ticks = cui__frame_alloc(ui, sizeof *ticks * 32);
    const char *label = cui__xml_label(doc, n);
    float lo = cui__xml_attr_num(doc, n, "min", 0);
    float hi = cui__xml_attr_num(doc, n, "max", 1);
    memset(&o, 0, sizeof o);
    o.label = label && *label ? label : NULL;
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.unit = cui__xml_attr_text(doc, n, "unit", NULL);
    o.length = cui__xml_attr_num(doc, n, "length", 0);
    o.horizontal = cui__xml_attr_bool(doc, n, "horizontal", false);
    o.step = cui__xml_attr_num(doc, n, "step", 0);
    o.reset = cui__xml_attr_num(doc, n, "reset", 0);
    o.hide_value = cui__xml_attr_bool(doc, n, "hidevalue", false);
    if (ticks) {
        o.tick_count = cui__xml_live_numbers(cui__xml_attr_text(doc, n, "ticks", NULL), ticks, 32);
        o.ticks = o.tick_count ? ticks : NULL;
    }
    if (meter) {
        o.meter_channels = cui__xml_live_numbers(cui__xml_attr_text(doc, n, "meter", NULL), meter, 8);
        o.meter = o.meter_channels ? meter : NULL;
    }
    if (!n->seen) *value = cui__xml_attr_num(doc, n, "value", *value);
    if (hi <= lo) hi = lo + 1;
    if (disabled) cui_push_disabled(ui, true);
    if (cui_fader_ex(ui, n->key, value, lo, hi, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_level_meter(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_level_meter_opts o;
    float *levels = cui__frame_alloc(ui, sizeof *levels * 8);
    int channels = 0;
    if (!levels) return;
    channels = cui__xml_live_numbers(cui__xml_attr_text(doc, n, "levels", NULL), levels, 8);
    if (!channels) return;
    memset(&o, 0, sizeof o);
    o.min_db = cui__xml_attr_num(doc, n, "mindb", -60);
    o.max_db = cui__xml_attr_num(doc, n, "maxdb", 6);
    o.length = cui__xml_attr_num(doc, n, "length", 0);
    o.thickness = cui__xml_attr_num(doc, n, "thickness", 0);
    o.horizontal = cui__xml_attr_bool(doc, n, "horizontal", false);
    o.scale = cui__xml_attr_bool(doc, n, "scale", false);
    o.segmented = cui__xml_attr_bool(doc, n, "segmented", false);
    o.hold = cui__xml_attr_num(doc, n, "hold", 1.5f);
    o.decay = cui__xml_attr_num(doc, n, "decay", 0);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_level_meter_ex(ui, n->key, levels, channels, &o)) cui__xml_changed_hook(doc, n);
    if (disabled) cui_pop_disabled(ui);
}

/* A guided tour: closed until `step` reaches 0, its steps pointing at elements
 * by the Name they were given. */
static void cui__xml_draw_tour(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_tour_opts o;
    cui_tour_step *steps = cui__frame_alloc(ui, sizeof *steps * 8);
    int *step = cui__xml_slot_int(doc, n);
    int count = 0, i, result;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_num(doc, n, "width", 0);
    o.padding = cui__xml_attr_num(doc, n, "padding", 0);
    o.block_target = cui__xml_attr_bool(doc, n, "blocktarget", false);
    o.next_label = cui__xml_attr_text(doc, n, "nextlabel", NULL);
    o.back_label = cui__xml_attr_text(doc, n, "backlabel", NULL);
    o.done_label = cui__xml_attr_text(doc, n, "donelabel", NULL);
    if (!n->seen) *step = (int)cui__xml_attr_num(doc, n, "step", -1);
    for (i = 0; steps && i < n->kid_count && count < 8; i++) {
        cui_xml_node *kid = n->kids[i];
        const char *target;
        if (kid->comment || kid->tag_id != CUI__XML_TAG_TOURSTEP) continue;
        target = cui__xml_attr_text(doc, kid, "target", NULL);
        steps[count].target = target && *target ? cui_xml_node_id(cui_xml_find(doc, target)) : 0;
        steps[count].title = cui__xml_label(doc, kid);
        steps[count].body = cui__xml_attr_text(doc, kid, "description", NULL);
        steps[count].placement = cui__xml_placement_of(doc, kid, CUI_PLACE_BOTTOM_START);
        count++;
    }
    if (!steps || !count || *step < 0) return;
    if (disabled) cui_push_disabled(ui, true);
    result = cui_tour_ex(ui, n->key, steps, count, step, &o);
    if (disabled) cui_pop_disabled(ui);
    if (result) {
        n->reported = result;
        cui__xml_fire(doc, n, "onchanged");
    }
}

static void cui__xml_draw_dot_display(cui_xml *doc, cui_xml_node *n, cui_ctx *ui) {
    cui_dot_display_opts o;
    memset(&o, 0, sizeof o);
    o.pitch = cui__xml_attr_num(doc, n, "pitch", 0);
    o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
    o.accent = cui__xml_attr_bool(doc, n, "accent", false);
    o.no_dim = cui__xml_attr_bool(doc, n, "nodim", false);
    o.marquee = cui__xml_attr_bool(doc, n, "marquee", false);
    o.align = cui__xml_text_align_of(doc, n, CUI_TEXT_START);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FIT);
    cui_dot_display_ex(ui, cui__xml_label(doc, n), &o);
}

/* ---- dot chart, annotations and event track ----------------------------- */

static cui_axis_units cui__xml_axis_units_of(cui_xml *doc, cui_xml_node *n, const char *attr, int fallback) {
    static const char *const names[] = { "number", "hours" };
    return (cui_axis_units)cui__xml_choice_of(doc, n, attr, names, 2, fallback);
}

/* One annotation of a chart: a vertical line at `At`, a horizontal one at
 * `Value`, or a point where they meet. */
static bool cui__xml_read_annotation(cui_xml *doc, cui_xml_node *n, cui_chart_annotation *out) {
    static const char *const kinds[] = { "x", "y", "point" };
    const char *label = cui__xml_label(doc, n);
    out->kind = (cui_annotation_kind)cui__xml_choice_of(doc, n, "axis", kinds, 3, 0);
    out->x = cui__xml_attr_num(doc, n, "at", 0);
    out->y = cui__xml_attr_num(doc, n, "value", 0);
    out->label = label && *label ? label : cui__xml_attr_text(doc, n, "text", NULL);
    out->detail = cui__xml_attr_text(doc, n, "detail", NULL);
    out->accent = cui__xml_attr_bool(doc, n, "accent", false);
    return true;
}

static void cui__xml_draw_dot_chart(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_dot_chart_opts o;
    cui_dot_series series[8];
    cui_chart_annotation notes[8];
    int count = 0, note_count = 0, points = 0, i;
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.height = cui__xml_attr_num(doc, n, "height", 0);
    o.x_min = cui__xml_attr_num(doc, n, "xmin", 0);
    o.x_max = cui__xml_attr_num(doc, n, "xmax", 0);
    o.x_units = cui__xml_axis_units_of(doc, n, "xunits", CUI_AXIS_NUMBER);
    o.x_step = cui__xml_attr_num(doc, n, "xstep", 0);
    o.min = cui__xml_attr_num(doc, n, "min", 0);
    o.max = cui__xml_attr_num(doc, n, "max", 0);
    o.format = cui__xml_attr_text(doc, n, "format", NULL);
    o.unit = cui__xml_attr_text(doc, n, "unit", NULL);
    o.pitch = cui__xml_attr_num(doc, n, "pitch", 0);
    o.no_grid = cui__xml_attr_bool(doc, n, "nogrid", false);
    o.no_axis = cui__xml_attr_bool(doc, n, "noaxis", false);
    o.no_readout = cui__xml_attr_bool(doc, n, "noreadout", false);
    for (i = 0; i < n->kid_count; i++) {
        cui_xml_node *kid = n->kids[i];
        if (kid->comment) continue;
        if (kid->tag_id == CUI__XML_TAG_SERIES) {
            const float *values = NULL;
            int value_count;
            if (count >= 8) continue;
            value_count = cui__xml_attr_numbers(doc, kid, "values", &values);
            if (!value_count) continue;
            series[count].values = values;
            series[count].name = cui__xml_label(doc, kid);
            series[count].accent = cui__xml_attr_bool(doc, kid, "accent", false);
            if (value_count > points) points = value_count;
            count++;
        } else if (kid->tag_id == CUI__XML_TAG_ANNOTATION && note_count < 8) {
            cui__xml_read_annotation(doc, kid, &notes[note_count]);
            note_count++;
        }
    }
    if (!count || !points) return;
    o.annotations = note_count ? notes : NULL;
    o.annotation_count = note_count;
    if (disabled) cui_push_disabled(ui, true);
    {
        cui_dot_chart_result r = cui_dot_chart_ex(ui, n->key, series, count, points, &o);
        if (r.clicked) {
            n->reported = 0;
            cui__xml_fire(doc, n, "onclicked");
        }
    }
    if (disabled) cui_pop_disabled(ui);
}

/* The moments an event track holds live in the element, as the C examples keep
 * them in their own array; Values names them when the document first draws. */
typedef struct {
    double times[16];
    int count, selected;
    bool init;
} cui__xml_track_state;

static void cui__xml_draw_event_track(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_event_track_opts o;
    cui_event_track_result r;
    cui__xml_track_state *st = n->cache;
    double lo = cui__xml_attr_num(doc, n, "min", 0);
    double hi = cui__xml_attr_num(doc, n, "max", 1);
    if (!st) {
        st = cui__xml_alloc(sizeof *st);
        if (!st) return;
        memset(st, 0, sizeof *st);
        st->selected = (int)cui__xml_attr_num(doc, n, "selected", -1);
        n->cache = st;
        n->cache_free = cui__xml_free_cache;
    }
    if (!st->init) {
        const float *values = NULL;
        int value_count = cui__xml_attr_numbers(doc, n, "values", &values), k;
        for (k = 0; k < value_count && k < 16; k++) st->times[st->count++] = (double)values[k];
        st->init = true;
    }
    memset(&o, 0, sizeof o);
    o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
    o.step = cui__xml_attr_num(doc, n, "step", 0);
    o.units = cui__xml_axis_units_of(doc, n, "units", CUI_AXIS_NUMBER);
    o.label_step = cui__xml_attr_num(doc, n, "labelstep", 0);
    o.selected = &st->selected;
    o.no_add = cui__xml_attr_bool(doc, n, "noadd", false);
    o.no_remove = cui__xml_attr_bool(doc, n, "noremove", false);
    o.no_axis = cui__xml_attr_bool(doc, n, "noaxis", false);
    if (hi <= lo) hi = lo + 1;
    if (disabled) cui_push_disabled(ui, true);
    r = cui_event_track_ex(ui, n->key, st->times, &st->count, 16, lo, hi, &o);
    if (disabled) cui_pop_disabled(ui);
    if (r.changed) cui__xml_changed_hook(doc, n);
}

static bool cui__xml_draw_impl(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    switch (n->tag_id) {
    case CUI__XML_TAG_WINDOW: {
        cui_box b;
        cui__xml_box_opts(doc, n, CUI_COLUMN, &b);
        b.width = CUI_FILL;
        b.height = CUI_FILL;
        b.bg = cui__xml_attr_color(doc, n, "background", CUI_COLOR_BACKGROUND);
        cui__xml_record_id(n, ui, cui_box_begin(ui, &b));
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_box_end(ui);
        return true;
    }
    case CUI__XML_TAG_COLUMN:
    case CUI__XML_TAG_ROW:
    case CUI__XML_TAG_STACK:
    case CUI__XML_TAG_GRID:
    case CUI__XML_TAG_BOX:
    case CUI__XML_TAG_CENTER: {
        cui_dir dir = n->tag_id == CUI__XML_TAG_ROW ? CUI_ROW
                    : n->tag_id == CUI__XML_TAG_STACK ? CUI_STACK
                    : n->tag_id == CUI__XML_TAG_GRID ? CUI_GRID
                    : CUI_COLUMN;
        cui_box b;
        cui__xml_box_opts(doc, n, dir, &b);
        if (n->tag_id == CUI__XML_TAG_CENTER) {
            b.align = CUI_ALIGN_CENTER;
            if (b.justify == CUI_JUSTIFY_START) b.justify = CUI_JUSTIFY_CENTER;
        }
        cui__xml_record_id(n, ui, cui_box_begin(ui, &b));
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_box_end(ui);
        return true;
    }
    case CUI__XML_TAG_WIDGETS:
        /* a <Widgets> root, whose declarations were taken out: its other
         * children are drawn like a column's */
        cui__xml_draw_box(doc, n, ui, CUI_COLUMN, disabled);
        return true;
    case CUI__XML_TAG_CARD:
        cui__xml_draw_card(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SCROLL:
        cui__xml_draw_scroll(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SPLIT:
        cui__xml_draw_split(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SPACER:
        cui_spacer(ui);
        return true;
    case CUI__XML_TAG_GAP:
        cui_gap(ui, cui__xml_attr_num(doc, n, "size", 8));
        return true;
    case CUI__XML_TAG_SEPARATOR:
        cui_separator(ui);
        return true;
    case CUI__XML_TAG_HEADING:
    case CUI__XML_TAG_TEXT:
    case CUI__XML_TAG_LABEL:
    case CUI__XML_TAG_SECONDARY:
    case CUI__XML_TAG_MUTED:
    case CUI__XML_TAG_CODE:
    case CUI__XML_TAG_KBD:
    case CUI__XML_TAG_LINK:
    case CUI__XML_TAG_ICON:
        cui__xml_draw_text_kind(doc, n, ui, n->tag_id);
        return n->tag_id != CUI__XML_TAG_ICON;
    case CUI__XML_TAG_BUTTON:
        cui__xml_draw_button(doc, n, ui, disabled, false);
        return true;
    case CUI__XML_TAG_ICONBUTTON:
        cui__xml_draw_button(doc, n, ui, disabled, true);
        return true;
    case CUI__XML_TAG_CHECKBOX:
        cui__xml_draw_check(doc, n, ui, disabled, false);
        return true;
    case CUI__XML_TAG_SWITCH:
        cui__xml_draw_check(doc, n, ui, disabled, true);
        return true;
    case CUI__XML_TAG_RADIOGROUP:
        cui__xml_draw_radio_group(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_RADIO:
        /* only meaningful inside a RadioGroup, which draws it itself */
        return false;
    case CUI__XML_TAG_SLIDER:
        cui__xml_draw_slider(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_RANGESLIDER:
        cui__xml_draw_range_slider(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_RATING: {
        int *value = cui__xml_slot_int(doc, n);
        int max = (int)cui__xml_attr_num(doc, n, "max", 5);
        if (disabled) cui_push_disabled(ui, true);
        if (cui_rating(ui, n->key, value, max)) cui__xml_changed_hook(doc, n);
        if (disabled) cui_pop_disabled(ui);
        return true;
    }
    case CUI__XML_TAG_CHIP: {
        cui_chip_opts o;
        bool *value = cui__xml_slot_bool(doc, n);
        memset(&o, 0, sizeof o);
        o.icon = cui__xml_icon(doc, n, NULL);
        o.disabled = disabled;
        if (cui_chip_ex(ui, cui__xml_label(doc, n), value, &o)) cui__xml_changed_hook(doc, n);
        return true;
    }
    case CUI__XML_TAG_INPUT:
        cui__xml_draw_text_field(doc, n, ui, disabled, 0);
        return true;
    case CUI__XML_TAG_TEXTAREA:
        cui__xml_draw_text_field(doc, n, ui, disabled, 1);
        return true;
    case CUI__XML_TAG_SEARCH:
        cui__xml_draw_text_field(doc, n, ui, disabled, 2);
        return true;
    case CUI__XML_TAG_NUMBER:
        cui__xml_draw_number(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SELECT:
        cui__xml_draw_select(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SEGMENTED:
        cui__xml_draw_segmented(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TABS:
        cui__xml_draw_tabs(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_NAVITEM:
        cui__xml_draw_nav_item(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_NAVSECTION:
        cui_nav_section(ui, cui__xml_label(doc, n));
        return true;
    case CUI__XML_TAG_BREADCRUMB: {
        const char **items = NULL;
        int count = cui__xml_attr_items(doc, n, &items);
        int clicked;
        if (!count) return false;
        if (disabled) cui_push_disabled(ui, true);
        clicked = cui_breadcrumb(ui, items, count);
        if (disabled) cui_pop_disabled(ui);
        if (clicked >= 0) {
            n->reported = clicked;
            cui__xml_fire(doc, n, "onclicked");
        }
        return true;
    }
    case CUI__XML_TAG_PAGINATION:
        cui__xml_draw_pagination(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_STEPPER: {
        const char **items = NULL;
        int count = cui__xml_attr_items(doc, n, &items);
        if (!count) return false;
        cui_stepper(ui, items, count, (int)cui__xml_attr_num(doc, n, "current", 0));
        return true;
    }
    case CUI__XML_TAG_PROGRESS:
        cui__xml_draw_progress(doc, n, ui);
        return true;
    case CUI__XML_TAG_SPINNER: {
        cui_spinner_opts o;
        memset(&o, 0, sizeof o);
        o.style = cui__xml_spinner_style_of(doc, n, CUI_SPINNER_ARC);
        o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
        cui_spinner_ex(ui, cui__xml_attr_num(doc, n, "size", 20), &o);
        return true;
    }
    case CUI__XML_TAG_SKELETON:
        cui_skeleton(ui, cui__xml_attr_size(doc, n, "width", CUI_FILL), cui__xml_attr_num(doc, n, "height", 12));
        return true;
    case CUI__XML_TAG_BADGE:
        cui__xml_draw_badge(doc, n, ui);
        return true;
    case CUI__XML_TAG_AVATAR:
        cui__xml_draw_avatar(doc, n, ui);
        return true;
    case CUI__XML_TAG_METRIC:
        cui__xml_draw_metric(doc, n, ui);
        return true;
    case CUI__XML_TAG_METER:
        cui_meter(ui, cui__xml_attr_text(doc, n, "label", ""), cui__xml_attr_num(doc, n, "used", 0),
                  cui__xml_attr_num(doc, n, "total", 1), cui__xml_attr_text(doc, n, "unit", NULL));
        return true;
    case CUI__XML_TAG_EMPTYSTATE:
        cui__xml_draw_empty_state(doc, n, ui);
        return true;
    case CUI__XML_TAG_ALERT:
        cui__xml_draw_alert(doc, n, ui);
        return n->id != 0;
    case CUI__XML_TAG_BANNER:
        cui__xml_draw_banner(doc, n, ui);
        return n->id != 0;
    case CUI__XML_TAG_ACCORDION:
        cui__xml_draw_accordion(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TREE:
        cui__xml_draw_tree(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_LISTITEM:
        cui__xml_draw_list_item(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TABLE:
        cui__xml_draw_table(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TABLECOLUMN:
    case CUI__XML_TAG_TABLEROW:
    case CUI__XML_TAG_CELL:
        return false;
    case CUI__XML_TAG_MARKDOWN: {
        cui_markdown_opts o;
        const char *text = cui__xml_source_text(doc, n);
        memset(&o, 0, sizeof o);
        o.width = cui__xml_attr_size(doc, n, "width", CUI_FILL);
        o.fit_content = cui__xml_attr_bool(doc, n, "fitcontent", false);
        o.compact = cui__xml_attr_bool(doc, n, "compact", false);
        o.size = cui__xml_attr_num(doc, n, "size", 0);
        o.color = cui__xml_attr_color(doc, n, "color", (cui_color){ 0, 0, 0, 0 });
        cui_markdown_ex(ui, n->key, text ? text : cui__xml_label(doc, n), &o);
        return true;
    }
    case CUI__XML_TAG_CODEBLOCK: {
        cui_code_opts o;
        const char *text = cui__xml_source_text(doc, n);
        memset(&o, 0, sizeof o);
        o.lang = cui__xml_attr_text(doc, n, "lang", NULL);
        o.title = cui__xml_attr_text(doc, n, "title", NULL);
        o.line_numbers = cui__xml_attr_bool(doc, n, "linenumbers", false);
        o.highlight_from = (int)cui__xml_attr_num(doc, n, "highlightfrom", 0);
        o.highlight_to = (int)cui__xml_attr_num(doc, n, "highlightto", 0);
        o.max_height = cui__xml_attr_num(doc, n, "maxheight", 0);
        o.size = cui__xml_attr_num(doc, n, "size", 0);
        o.no_header = cui__xml_attr_bool(doc, n, "noheader", false);
        cui_code_block_ex(ui, n->key, text ? text : cui__xml_label(doc, n), &o);
        return true;
    }
    case CUI__XML_TAG_JSON: {
        cui_json_opts o;
        const char *text = cui__xml_source_text(doc, n);
        memset(&o, 0, sizeof o);
        o.expand_depth = (int)cui__xml_attr_num(doc, n, "expanddepth", 2);
        o.max_height = cui__xml_attr_num(doc, n, "maxheight", 0);
        cui_json_view_ex(ui, n->key, text ? text : cui__xml_label(doc, n), &o);
        return true;
    }
    case CUI__XML_TAG_LINECHART:
        cui__xml_draw_series_chart(doc, n, ui, false);
        return true;
    case CUI__XML_TAG_BARCHART:
        cui__xml_draw_series_chart(doc, n, ui, true);
        return true;
    case CUI__XML_TAG_DONUTCHART:
        cui__xml_draw_donut(doc, n, ui);
        return true;
    case CUI__XML_TAG_SERIES:
    case CUI__XML_TAG_SLICE:
    case CUI__XML_TAG_PART:
        return false;
    case CUI__XML_TAG_SPARKLINE: {
        const float *values = NULL;
        int count = cui__xml_attr_numbers(doc, n, "values", &values);
        if (count < 2) return false;
        cui_sparkline(ui, values, count, cui__xml_attr_size(doc, n, "width", CUI_FILL),
                      cui__xml_attr_num(doc, n, "height", 32),
                      cui__xml_attr_color(doc, n, "color", CUI_COLOR_ACCENT));
        return true;
    }
    case CUI__XML_TAG_GAUGE:
        cui_gauge(ui, cui__xml_attr_text(doc, n, "label", ""), cui__xml_attr_num(doc, n, "value", 0),
                  cui__xml_attr_num(doc, n, "size", 120));
        return true;
    case CUI__XML_TAG_PATTERNMETER:
        cui__xml_draw_pattern_meter(doc, n, ui);
        return true;
    case CUI__XML_TAG_IMAGE:
        cui__xml_draw_image(doc, n, ui);
        return true;
    case CUI__XML_TAG_TOOLBAR: {
        cui_toolbar_opts o;
        static const char *const styles[] = { "plain", "grouped", "floating" };
        memset(&o, 0, sizeof o);
        o.no_border = cui__xml_attr_bool(doc, n, "noborder", false);
        o.style = (cui_toolbar_style)cui__xml_choice_of(doc, n, "style", styles, 3, 0);
        cui__xml_record_self(n, ui);
    cui_toolbar_begin(ui, n->key, &o);
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_toolbar_end(ui);
        return true;
    }
    case CUI__XML_TAG_TOOLBARBUTTON:
        cui__xml_draw_toolbar_button(doc, n, ui, disabled, false);
        return true;
    case CUI__XML_TAG_TOOLBARPRIMARY:
        cui__xml_draw_toolbar_button(doc, n, ui, disabled, true);
        return true;
    case CUI__XML_TAG_TOOLBARSEPARATOR:
        cui_toolbar_separator(ui);
        return true;
    case CUI__XML_TAG_TOOLBARSPACER:
        cui_toolbar_spacer(ui);
        return true;
    case CUI__XML_TAG_TOOLBARGROUP:
        cui_toolbar_group_begin(ui);
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_toolbar_group_end(ui);
        return true;
    case CUI__XML_TAG_TOOLBARRADIO: {
        const char **items = NULL;
        int count = cui__xml_attr_items(doc, n, &items);
        int *index = cui__xml_slot_int(doc, n);
        if (!count && cui__xml_attr_raw(n, "icons")) {
            /* the icons alone say how many modes there are */
            int only_icons = 0;
            (void)cui__xml_attr_list(doc, n, "icons", &only_icons, true);
            count = only_icons;
        }
        if (!count) return false;
        if (!n->seen) *index = (int)cui__xml_attr_num(doc, n, "value", *index);
        {
            int icon_count = 0;
            const char **icons = cui__xml_attr_raw(n, "icons")
                               ? cui__xml_attr_list(doc, n, "icons", &icon_count, true)
                               : cui__xml_attr_list(doc, n, "items", &icon_count, true);
            if (!icons || !icon_count) return false;
            cui_toolbar_radio_opts ro;
            memset(&ro, 0, sizeof ro);
            if (cui__xml_attr_raw(n, "labels")) {
                char **names = cui__frame_alloc(ui, sizeof *names * 8);
                if (names && cui__xml_split(ui, cui__xml_attr_text(doc, n, "labels", NULL), '|', names, 8) == icon_count)
                    ro.labels = (const char *const *)names;
            }
            if (cui_toolbar_radio_ex(ui, n->key, icons, icon_count, index, &ro))
                cui__xml_changed_hook(doc, n);
        }
        return true;
    }
    case CUI__XML_TAG_STATUSBAR: {
        cui_status_bar_opts o;
        memset(&o, 0, sizeof o);
        o.no_border = cui__xml_attr_bool(doc, n, "noborder", false);
        cui__xml_record_self(n, ui);
    cui_status_bar_begin(ui, n->key, &o);
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_status_bar_end(ui);
        return true;
    }
    case CUI__XML_TAG_STATUSTEXT:
        cui__xml_draw_status_item(doc, n, ui, 0);
        return true;
    case CUI__XML_TAG_STATUSITEM:
        cui__xml_draw_status_item(doc, n, ui, 1);
        return true;
    case CUI__XML_TAG_STATUSPROGRESS: {
        cui_status_progress_opts o;
        memset(&o, 0, sizeof o);
        o.label = cui__xml_attr_text(doc, n, "label", NULL);
        o.indeterminate = cui__xml_attr_bool(doc, n, "indeterminate", false);
        o.width = cui__xml_attr_num(doc, n, "width", 0);
        cui_status_bar_progress_ex(ui, cui__xml_attr_num(doc, n, "value", 0), &o);
        return true;
    }
    case CUI__XML_TAG_STATUSSPACER:
        cui_status_bar_spacer(ui);
        return true;
    case CUI__XML_TAG_MENUBAR:
        cui__xml_draw_menu_bar(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_MENUBARMENU:
        /* the menu bar draws its own menus */
        return false;
    case CUI__XML_TAG_MENU:
        cui__xml_draw_menu(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_MENUITEM:
        cui__xml_draw_menu_item(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_MENUSEPARATOR:
        cui_menu_separator(ui);
        return true;
    case CUI__XML_TAG_MENULABEL:
        cui_menu_label(ui, cui__xml_label(doc, n));
        return true;
    case CUI__XML_TAG_DIALOG:
        cui__xml_draw_dialog(doc, n, ui, disabled);
        return n->id != 0;
    case CUI__XML_TAG_FOOTER:
        /* a dialog resized taller than its content keeps this at its bottom edge */
        cui_dialog_footer_begin(ui);
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_dialog_footer_end(ui);
        return true;
    case CUI__XML_TAG_DOCTABS:
        cui__xml_draw_doc_tabs(doc, n, ui);
        return true;
    case CUI__XML_TAG_TAB:
    case CUI__XML_TAG_ITEM:
        return false;
    case CUI__XML_TAG_STATUSTAG: {
        cui_status_tag_opts o;
        static const char *const states[] = { "pending", "inprogress", "submitted", "inreview",
                                              "success", "failed", "expired", "custom" };
        memset(&o, 0, sizeof o);
        o.size = cui__xml_control_size_of(doc, n, CUI_SIZE_MD);
        o.removable = cui__xml_attr_bool(doc, n, "removable", false);
        o.icon = cui__xml_icon(doc, n, NULL);
        o.tone = cui__xml_tone_of(doc, n, CUI_TONE_NEUTRAL);
        o.id = n->key;
        if (cui_status_tag_ex(ui, cui__xml_label(doc, n),
                              (cui_tag_status)cui__xml_choice_of(doc, n, "status", states, 8, 7), &o)) {
            n->reported = 0;
            cui__xml_fire(doc, n, "onclicked");
        }
        return true;
    }
    case CUI__XML_TAG_COLORPICKER: {
        cui_color *value;
        if (!n->cache) {
            n->cache = cui__xml_alloc(sizeof(cui_color));
            if (!n->cache) return false;
            memset(n->cache, 0, sizeof(cui_color));
            n->cache_free = cui__xml_free_cache;
            *(cui_color *)n->cache = cui__xml_attr_color(doc, n, "value", CUI_COLOR_ACCENT);
        }
        value = n->cache;
        if (cui_color_picker(ui, n->key, value)) cui__xml_changed_hook(doc, n);
        return true;
    }
    case CUI__XML_TAG_CAROUSEL:
        cui__xml_draw_carousel(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SLIDE:
        return false;
    case CUI__XML_TAG_BUTTONGROUP:
        cui__xml_draw_button_group(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_COPYBUTTON:
        cui__xml_draw_copy_button(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_HOLDBUTTON:
        cui__xml_draw_hold_button(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TAGINPUT:
        cui__xml_draw_tag_input(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_OTP:
        cui__xml_draw_otp(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_PASSWORDSTRENGTH:
        cui__xml_draw_password_strength(doc, n, ui);
        return true;
    case CUI__XML_TAG_DATEPICKER:
        cui__xml_draw_date_picker(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_RADIOCARDS:
        cui__xml_draw_radio_cards(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_CHOICE:
        /* only meaningful inside a RadioCards, which draws it itself */
        return false;
    case CUI__XML_TAG_SKELETONGROUP:
        cui__xml_draw_skeleton_group(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_STATUSSPINNER:
        cui__xml_draw_status_spinner(doc, n, ui);
        return true;
    case CUI__XML_TAG_TIMELINEITEM:
        cui__xml_draw_timeline_item(doc, n, ui);
        return true;
    case CUI__XML_TAG_PROGRESSRING:
        cui__xml_draw_progress_ring(doc, n, ui);
        return true;
    case CUI__XML_TAG_AVATARGROUP:
        cui__xml_draw_avatar_group(doc, n, ui);
        return true;
    case CUI__XML_TAG_POPOVER:
        return cui__xml_draw_popover(doc, n, ui, disabled);
    case CUI__XML_TAG_CONTEXTMENU:
        cui__xml_draw_context_menu(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_REORDER:
        cui__xml_draw_reorder(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_REORDERITEM:
        /* only meaningful inside a Reorder, which draws it itself */
        return false;
    case CUI__XML_TAG_READOUT:
        cui__xml_draw_readout(doc, n, ui);
        return true;
    case CUI__XML_TAG_DOTDISPLAY:
        cui__xml_draw_dot_display(doc, n, ui);
        return true;
    case CUI__XML_TAG_DROPZONE:
        cui__xml_draw_dropzone(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SIGNATUREPAD:
        cui__xml_draw_signature_pad(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_AVATARUPLOAD:
        cui__xml_draw_avatar_upload(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TOC:
        cui__xml_draw_toc(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TOCITEM:
        /* only meaningful inside a Toc, which draws it itself */
        return false;
    case CUI__XML_TAG_TOCANCHOR:
        cui__xml_draw_toc_anchor(doc, n, ui);
        return true;
    case CUI__XML_TAG_INLINEEDIT:
        cui__xml_draw_inline_edit(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SCRUB:
        cui__xml_draw_scrub(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_MONEYINPUT:
        cui__xml_draw_money(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SHORTCUTRECORDER:
        cui__xml_draw_shortcut_recorder(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_EMOJIPICKER:
        cui__xml_draw_emoji_picker(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_DOTCHART:
        cui__xml_draw_dot_chart(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_ANNOTATION:
        /* only meaningful inside a chart, which draws it itself */
        return false;
    case CUI__XML_TAG_EVENTTRACK:
        cui__xml_draw_event_track(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_HOVERCARD:
        cui__xml_draw_hover_card(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_CONFIRMPOPOVER:
        cui__xml_draw_confirm_popover(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_EXPANDABLETEXT:
        cui__xml_draw_expandable_text(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_EXPANDABLECARD:
        cui__xml_draw_expandable_card(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_COUNTDOWN:
        cui__xml_draw_countdown(doc, n, ui);
        return true;
    case CUI__XML_TAG_CHECKLIST:
        cui__xml_draw_checklist(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_STEP:
        /* only meaningful inside a Checklist, which draws it itself */
        return false;
    case CUI__XML_TAG_VEIL:
        cui__xml_draw_veil(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SPLITBUTTON:
        cui__xml_draw_split_button(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TOGGLEGROUP:
        cui__xml_draw_toggle_group(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_CONFIRMBUTTON:
        cui__xml_draw_confirm_button(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_STATUSBUTTON:
        cui__xml_draw_status_button(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_REACTIONS:
        cui__xml_draw_reactions(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_THEMESWITCHER:
        cui__xml_draw_theme_switcher(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_THEMETOGGLE:
        cui__xml_draw_theme_toggle(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_USERMENU:
        cui__xml_draw_user_menu(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SWIPEITEM:
        cui__xml_draw_swipe_item(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SPEEDDIAL:
        cui__xml_draw_speed_dial(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_MULTISELECT:
        cui__xml_draw_multi_select(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_DATERANGEPICKER:
        cui__xml_draw_date_range_picker(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_RANGECALENDAR:
        cui__xml_draw_range_calendar(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TIMEPICKER:
        cui__xml_draw_time_picker(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TIMEDIAL:
        cui__xml_draw_time_dial(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_WHEELPICKER:
        cui__xml_draw_wheel_picker(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_RULERSLIDER:
        cui__xml_draw_ruler_slider(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_HISTOGRAMSLIDER:
        cui__xml_draw_histogram_slider(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_CHATMESSAGE:
        cui__xml_draw_chat_message(doc, n, ui);
        return true;
    case CUI__XML_TAG_REASONING:
        cui__xml_draw_reasoning(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TOOLCALL:
        cui__xml_draw_tool_call(doc, n, ui);
        return true;
    case CUI__XML_TAG_AGENTSTATUS:
        cui__xml_draw_agent_status(doc, n, ui);
        return true;
    case CUI__XML_TAG_TYPEWRITER:
        cui__xml_draw_typewriter(doc, n, ui);
        return true;
    case CUI__XML_TAG_CITATION:
        cui__xml_draw_citation(doc, n, ui);
        return true;
    case CUI__XML_TAG_SOURCELIST:
        cui__xml_draw_source_list(doc, n, ui);
        return true;
    case CUI__XML_TAG_SOURCE:
        /* only meaningful inside a SourceList, which draws it itself */
        return false;
    case CUI__XML_TAG_RESPONSEACTIONS:
        cui__xml_draw_response_actions(doc, n, ui);
        return true;
    case CUI__XML_TAG_CONTEXTMETER:
        cui__xml_draw_context_meter(doc, n, ui);
        return true;
    case CUI__XML_TAG_CHATCOMPOSER:
        cui__xml_draw_chat_composer(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_AGENTPLAN:
        cui__xml_draw_agent_plan(doc, n, ui);
        return true;
    case CUI__XML_TAG_PLANSTEP:
        /* only meaningful inside an AgentPlan, which draws it itself */
        return false;
    case CUI__XML_TAG_PROMPTSUGGESTIONS:
        cui__xml_draw_prompt_suggestions(doc, n, ui);
        return true;
    case CUI__XML_TAG_SUGGESTION:
        /* only meaningful inside a PromptSuggestions, which draws it itself */
        return false;
    case CUI__XML_TAG_KNOB:
        cui__xml_draw_knob(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_FADER:
        cui__xml_draw_fader(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_LEVELMETER:
        cui__xml_draw_level_meter(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TOUR:
        cui__xml_draw_tour(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TOURSTEP:
        /* only meaningful inside a Tour, which draws it itself */
        return false;
    case CUI__XML_TAG_RATIOSLIDER:
        cui__xml_draw_ratio_slider(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_GRADIENTSLIDER:
        cui__xml_draw_gradient_slider(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_STEPSLIDER:
        cui__xml_draw_step_slider(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_PALETTESTRIP:
        cui__xml_draw_palette_strip(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_ASPECTPICKER:
        cui__xml_draw_aspect_picker(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_OPTIONACTION:
        cui__xml_draw_option_action(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_SEEDFIELD:
        cui__xml_draw_seed_field(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_LOADER:
        cui__xml_draw_loader(doc, n, ui);
        return true;
    case CUI__XML_TAG_SHIMMERTEXT:
        cui__xml_draw_shimmer_text(doc, n, ui);
        return true;
    case CUI__XML_TAG_TEXTREVEAL:
        cui__xml_draw_text_reveal(doc, n, ui);
        return true;
    case CUI__XML_TAG_WORDROTATE:
        cui__xml_draw_word_rotate(doc, n, ui);
        return true;
    case CUI__XML_TAG_GRADIENTTEXT:
        cui__xml_draw_gradient_text(doc, n, ui);
        return true;
    case CUI__XML_TAG_SPOTLIGHT:
        cui__xml_draw_spotlight(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_DOCK:
        cui__xml_draw_dock(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_DOCKITEM:
    case CUI__XML_TAG_DOCKSEPARATOR:
        /* only meaningful inside a Dock, which draws them itself */
        return false;
    case CUI__XML_TAG_CONTROLTILE:
        cui__xml_draw_control_tile(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_NOWPLAYING:
        cui__xml_draw_now_playing(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_WAVEFORM:
        cui__xml_draw_waveform(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TABBAR:
        cui__xml_draw_tab_bar(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TABITEM:
        /* only meaningful inside a TabBar, which draws it itself */
        return false;
    case CUI__XML_TAG_TREEMAP:
        cui__xml_draw_treemap(doc, n, ui);
        return true;
    case CUI__XML_TAG_FUNNEL:
        cui__xml_draw_funnel(doc, n, ui);
        return true;
    case CUI__XML_TAG_SPLITBAR:
        cui__xml_draw_split_bar(doc, n, ui);
        return true;
    case CUI__XML_TAG_WAFFLE:
        cui__xml_draw_waffle(doc, n, ui);
        return true;
    case CUI__XML_TAG_BARLIST:
        cui__xml_draw_bar_list(doc, n, ui);
        return true;
    case CUI__XML_TAG_ACTIVITYRINGS:
        cui__xml_draw_activity_rings(doc, n, ui);
        return true;
    case CUI__XML_TAG_HEATMAP:
        cui__xml_draw_heatmap(doc, n, ui);
        return true;
    case CUI__XML_TAG_CALENDARHEATMAP:
        cui__xml_draw_calendar_heatmap(doc, n, ui);
        return true;
    case CUI__XML_TAG_RADIALGAUGE:
        cui__xml_draw_radial_gauge(doc, n, ui);
        return true;
    case CUI__XML_TAG_RADAR:
        cui__xml_draw_radar(doc, n, ui);
        return true;
    case CUI__XML_TAG_SCATTER:
        cui__xml_draw_scatter(doc, n, ui);
        return true;
    case CUI__XML_TAG_SANKEY:
        cui__xml_draw_sankey(doc, n, ui);
        return true;
    case CUI__XML_TAG_KANBAN:
        cui__xml_draw_kanban(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_KANBANCOLUMN:
    case CUI__XML_TAG_KANBANCARD:
        /* only meaningful inside a Kanban, which draws them itself */
        return false;
    case CUI__XML_TAG_WEEKCALENDAR:
        cui__xml_draw_week_calendar(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_CALEVENT:
    case CUI__XML_TAG_TASK:
    case CUI__XML_TAG_MONTHEVENT:
        /* only meaningful inside their calendar */
        return false;
    case CUI__XML_TAG_GANTT:
        cui__xml_draw_gantt(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_MONTHCALENDAR:
        cui__xml_draw_month_calendar(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_VOICENOTE:
        cui__xml_draw_voice_note(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_VOICEWAVE:
        cui__xml_draw_voice_wave(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_VOICESESSION:
        cui__xml_draw_voice_session(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_CHATDIVIDER:
        cui__xml_draw_chat_divider(doc, n, ui);
        return true;
    case CUI__XML_TAG_TYPINGINDICATOR:
        cui__xml_draw_typing_indicator(doc, n, ui);
        return true;
    case CUI__XML_TAG_COMMENT:
        cui__xml_draw_comment(doc, n, ui);
        return true;
    case CUI__XML_TAG_PRESENCE:
        cui__xml_draw_presence(doc, n, ui);
        return true;
    case CUI__XML_TAG_COUNTER:
        cui__xml_draw_counter(doc, n, ui);
        return true;
    case CUI__XML_TAG_TICKER:
        cui__xml_draw_ticker(doc, n, ui);
        return true;
    case CUI__XML_TAG_CARDSTACK:
        cui__xml_draw_card_stack(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_CARDSTACKITEM:
        /* only meaningful inside a CardStack, which draws it itself */
        return false;
    case CUI__XML_TAG_FILTERBAR:
        cui__xml_draw_filter_bar(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_FIELD:
        /* only meaningful inside a FilterBar */
        return false;
    case CUI__XML_TAG_TREETABLE:
        cui__xml_draw_tree_table(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TREEROW:
        /* only meaningful inside a TreeTable */
        return false;
    case CUI__XML_TAG_PROBLEMS:
        cui__xml_draw_problems(doc, n, ui);
        return true;
    case CUI__XML_TAG_DIAGNOSTIC:
        return false;
    case CUI__XML_TAG_KVEDITOR:
        cui__xml_draw_kv_editor(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_PAIR:
        return false;
    case CUI__XML_TAG_PIPELINE:
        cui__xml_draw_pipeline(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_STAGE:
    case CUI__XML_TAG_JOB:
        return false;
    case CUI__XML_TAG_QUERYBUILDER:
        cui__xml_draw_query_builder(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_CONDITION:
        return false;
    case CUI__XML_TAG_SCHEDULEPICKER:
        cui__xml_draw_schedule_picker(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_KEYMAP:
        cui__xml_draw_keymap(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_BINDING:
    case CUI__XML_TAG_CATEGORY:
        return false;
    case CUI__XML_TAG_TESTEXPLORER:
        cui__xml_draw_test_explorer(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_TEST:
        return false;
    case CUI__XML_TAG_STACKTRACE:
        cui__xml_draw_stack_trace(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_FRAME:
        return false;
    case CUI__XML_TAG_REQUESTBAR:
        cui__xml_draw_request_bar(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_RESPONSESTATUS:
        cui__xml_draw_response_status(doc, n, ui);
        return true;
    case CUI__XML_TAG_URLPARAMS:
        cui__xml_draw_url_params(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_COVERAGE:
        cui__xml_draw_coverage(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_COVERAGEFILE:
        return false;
    case CUI__XML_TAG_FEATUREFLAGS:
        cui__xml_draw_feature_flags(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_FLAG:
    case CUI__XML_TAG_ENV:
        return false;
    case CUI__XML_TAG_APIEXPLORER:
        cui__xml_draw_api_explorer(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_ENDPOINT:
    case CUI__XML_TAG_PARAM:
        return false;
    case CUI__XML_TAG_REGEXTESTER:
        cui__xml_draw_regex_tester(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_NODEEDITOR:
        cui__xml_draw_node_editor(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_NODE:
    case CUI__XML_TAG_NODEINPUT:
    case CUI__XML_TAG_NODEOUTPUT:
    case CUI__XML_TAG_NODECONTENT:
    case CUI__XML_TAG_NODELINK:
        /* only meaningful inside a NodeEditor */
        return false;
    case CUI__XML_TAG_SELECTIONRING: {
        cui_selection_ring_opts o;
        memset(&o, 0, sizeof o);
        o.name = cui__xml_attr_text(doc, n, "name", NULL);
        cui_selection_ring_ex(ui, cui_color_of(ui, cui__xml_attr_color(doc, n, "color", CUI_COLOR_WARNING)), &o);
        return true;
    }
    case CUI__XML_TAG_CANVASCONTROLS:
        cui__xml_draw_canvas_controls(doc, n, ui, disabled);
        return true;
    case CUI__XML_TAG_CANVASTOGGLE:
        return false;
    case CUI__XML_TAG_LIVECURSORS:
        cui__xml_draw_live_cursors(doc, n, ui);
        return true;
    case CUI__XML_TAG_PRESENCEPILL:
        cui__xml_draw_presence_pill(doc, n, ui);
        return true;
    case CUI__XML_TAG_FORMATBAR:
        cui__xml_draw_format_bar(doc, n, ui, disabled);
        return true;

    case CUI__XML_TAG_SHORTCUT: {
        int mods = 0, key = CUI_KEY_NONE;
        const char *keys = cui__xml_attr_text(doc, n, "keys", NULL);
        if (!cui_wants_keyboard(ui) && cui__xml_parse_keys(keys, &mods, &key) &&
            cui_shortcut(ui, mods, key))
            cui__xml_fire(doc, n, "action");
        return false;
    }
    case CUI__XML_TAG_VAR: {
        const char *name = cui__xml_attr_text(doc, n, "name", NULL);
        const char *value = cui__xml_attr_text(doc, n, "value", NULL);
        if (name && *name) cui_xml_set_var(doc, name, value ? value : "");
        return false;
    }
    case CUI__XML_TAG_UNKNOWN:
    default: {
        cui_xml_tag_entry *entry = cui__xml_tag_entry(doc, n);
        if (entry && entry->fn) {
            entry->fn(doc, n, ui, entry->user);
            return true;
        }
        if (n->tag_id == CUI__XML_TAG_UNKNOWN)
            cui__xml_warn(doc, n, "<%s> is not a tag of CUI: its children are drawn in place", n->tag);
        if (n->kid_count) cui__xml_draw_kids(doc, n, ui, disabled);
        return false;
    }
    }
}

bool cui__xml_draw(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    bool dis;
    bool drew;
    bool scoped;
    const char *tooltip;
    if (!n || n->comment) return false;
    n->changed = false;
    n->reported = -1;
    /* 0 and an empty rectangle say "not drawn this frame" */
    n->id = 0;
    memset(&n->rect, 0, sizeof n->rect);
    if (!n->shown) return false;
    if (cui__xml_attr_raw(n, "visible") && !cui__xml_attr_bool(doc, n, "visible", true)) return false;
    {
        const char *cond = cui__xml_attr_raw(n, "if");
        if (cond && *cond && !cui__xml_test(doc, n, cond, true)) return false;
    }
    dis = disabled || !n->enabled || cui__xml_attr_bool(doc, n, "disabled", false);
    if (doc->depth > CUI__XML_MAX_DEPTH) return false;
    doc->depth++;
    /* A nav item shares the id scope of the items beside it: the highlight it feeds
     * belongs to the list, and a scope of its own would make every item a group of
     * one, which the C API never does (all items are drawn in their list's scope).
     * The label still tells the items apart; the list's scope tells the lists apart. */
    scoped = n->tag_id != CUI__XML_TAG_NAVITEM;
    if (scoped) cui_push_id(ui, n->key ? n->key : "cui_xml");
    if (doc->pending_popover && (n->tag_id == CUI__XML_TAG_POPOVER || n->tag_id == CUI__XML_TAG_CONFIRMPOPOVER) &&
        n == cui_xml_find(doc, doc->pending_popover)) {
        cui_popover_toggle(ui, n->key);
        free(doc->pending_popover);
        doc->pending_popover = NULL;
    }
    cui_push_disabled(ui, dis);
    drew = cui__xml_draw_impl(doc, n, ui, dis);
    n->seen = true;
    if (drew && n->id == 0) {
        n->id = cui_last_id(ui);
        n->rect = cui_last_rect(ui);
    }
    tooltip = cui__xml_attr_text(doc, n, "tooltip", NULL);
    if (tooltip && *tooltip && drew) cui_tooltip(ui, tooltip);
    cui_pop_disabled(ui);
    if (scoped) cui_pop_id(ui);
    doc->depth--;
    return drew;
}
