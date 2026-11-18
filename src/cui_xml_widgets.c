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
    { "TableColumn", "Title", CUI_XML_TEXT, NULL, "Header text." },
    { "TableColumn", "Sortable", CUI_XML_BOOL, NULL, "Clicking the header sorts." },
    { "TableColumn", "Align", CUI_XML_CHOICE, "start|center|right|left|end", "Alignment of the cells." },
    { "Markdown", "Source", CUI_XML_FILE, NULL, "A file to read instead of Text." },
    { "CodeBlock", "Lang", CUI_XML_TEXT, NULL, "Language of the highlighter: c, cpp, js, ts, python..." },
    { "CodeBlock", "Title", CUI_XML_TEXT, NULL, "Shown in the title bar, e.g. a file name." },
    { "CodeBlock", "LineNumbers", CUI_XML_BOOL, NULL, "A column of line numbers." },
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
    { "DotDisplay", "Marquee", CUI_XML_BOOL, NULL, "Scrolls text wider than the box instead of shrinking it." }
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

static cui_tone cui__xml_tone_of(cui_xml *doc, cui_xml_node *n, cui_tone fallback) {
    static const char *const names[] = { "neutral", "info", "success", "warning", "danger", "violet", "teal", "rose", "accent" };
    return (cui_tone)cui__xml_choice_of(doc, n, "tone", names, 9, (int)fallback);
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
        for (i = 0; i < n->kid_count; i++) {
            cui_xml_node *row = n->kids[i];
            int c;
            if (row->tag_id != CUI__XML_TAG_TABLEROW || !row->shown) continue;
            if (cui_table_row(ui, cui__xml_attr_bool(doc, row, "selected", false))) {
                n->reported = i;
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
    if (cui__xml_attr_bool(doc, n, "toggle", false)) o.toggle = cui__xml_slot_bool(doc, n);
    if (primary) {
        if (cui_toolbar_primary_ex(ui, cui__xml_icon(doc, n, "check"), &o)) {
            n->reported = 0;
            cui__xml_fire(doc, n, "onclicked");
        }
        return;
    }
    if (cui_toolbar_button_ex(ui, cui__xml_icon(doc, n, "square"), &o)) {
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

static void cui__xml_draw_popover(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
    cui_popover_opts o;
    memset(&o, 0, sizeof o);
    o.placement = cui__xml_placement_of(doc, n, CUI_PLACE_BOTTOM_START);
    o.width = cui__xml_attr_num(doc, n, "width", 0);
    o.pad = cui__xml_attr_num(doc, n, "pad", 0);
    o.gap = cui__xml_attr_num(doc, n, "gap", 0);
    if (disabled) cui_push_disabled(ui, true);
    if (cui_popover_begin(ui, n->key, &o)) {
        cui__xml_record_self(n, ui);
        cui__xml_draw_kids(doc, n, ui, disabled);
        cui_popover_end(ui);
    }
    if (disabled) cui_pop_disabled(ui);
}

static void cui__xml_draw_context_menu(cui_xml *doc, cui_xml_node *n, cui_ctx *ui, bool disabled) {
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
        if (!count) return false;
        /* Icons="a|b" names them; without it Items are the icon names */
        {
            int icon_count = 0;
            const char **icons = cui__xml_attr_raw(n, "icons")
                               ? cui__xml_attr_list(doc, n, "icons", &icon_count, true)
                               : cui__xml_attr_list(doc, n, "items", &icon_count, true);
            if (!icons || !icon_count) return false;
            if (cui_toolbar_radio_ex(ui, n->key, icons, icon_count, index, NULL))
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
        cui__xml_draw_popover(doc, n, ui, disabled);
        return true;
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
    cui_push_id(ui, n->key ? n->key : "cui_xml");
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
    cui_pop_id(ui);
    doc->depth--;
    return drew;
}
