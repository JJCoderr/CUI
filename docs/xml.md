# Interfaces described in XML

A document describes a whole interface, or one panel of one, and CUI draws it
every frame:

```c
cui_xml *doc = cui_xml_load("settings.xml");
cui_app *app = cui_app_create(&cui_xml_app_desc(doc));   /* Title, size, theme */
while (cui_app_frame(app)) {
    cui_ctx *ui = cui_app_ui(app);
    cui_xml_poll(doc);                  /* reads the file again when it changed */
    cui_xml_render(doc, ui);
}
```

```xml
<Window Title="Settings" Width="880" Height="620" Pad="24" Gap="12">
  <Heading Level="1">Settings</Heading>
  <Card Gap="8">
    <Row Gap="8">
      <Input Name="path" Label="Folder" Placeholder="~/Projects" Fill="1"/>
      <Button Text="Save" Variant="Primary" Icon="check" OnClicked="file.save"/>
    </Row>
  </Card>
  <Switch Name="notify" Text="Notifications" Bind="notify"/>
  <Slider Name="scale" Text="Scale" Min="0.5" Max="3" Step="0.1" Bind="scale"/>
</Window>
```

The pieces this page covers:

- [How a document becomes a frame](#how-a-document-becomes-a-frame)
- [Rules](#rules)
- [Value types](#value-types)
- [Values, bindings and actions](#values-bindings-and-actions)
- [The elements](#the-elements) 鈥?containers, text, controls, display, chrome, logic
- [Tags of your own](#tags-of-your-own)
- [Repeat, ForEach, If and Include](#repeat-foreach-if-and-include)
- [Following the file](#following-the-file)
- [Tools: the catalog](#tools-the-catalog)
- [Using one panel of a document](#using-one-panel-of-a-document)

## How a document becomes a frame

CUI is immediate mode: the app describes its interface every frame. A document
is the same, written once and read every frame. Nothing is retained except what
each element holds 鈥?a switch's value, the text of a field, the open tab 鈥?and
that lives in the document, keyed by the element's `Name`, so it survives the
frames and the reloads.

Three consequences worth knowing:

- A document nests no state of its own: what a control shows is read at the
  start of its draw and written when the user changes it.
- A condition (`If="..."`) and `{"..."}` are computed every frame, so they can
  follow a bound variable without reloading anything.
- What an attribute holds is not cached: change it with
  `cui_xml_node_set_attr` and the next frame shows it.

## Rules

- An element is a component; its tag is the component type. **Tag and attribute
  names are not case sensitive.**
- Child elements are drawn inside their parent, in order.
- The text inside an element is its `Text`: `<Label>Hello</Label>` is
  `<Label Text="Hello"/>`. The element's text wins over the attribute.
- `{"..."}` is computed where the attribute is read; `{{` writes one brace, and
  a brace that names nothing stays as written (see
  [Values, bindings and actions](#values-bindings-and-actions)).
- `<` and `&` are written `&lt;` and `&amp;` inside attribute values, as XML
  requires: `If="{count &lt; 3}"`. The five predefined entities and numeric
  characters (`&#233;`, `&#xE9;`) are read; CDATA is read; comments are kept and
  drawn nowhere. Files are UTF-8. There is no DTD and no namespace.
- An unknown tag, or a value that does not parse, is reported with its line and
  column through the error callback, and the rest is still drawn. An unknown
  tag draws its children in place.
- `Name="..."` gives an element its identity: `cui_xml_find`, `cui_xml_get_*`,
  `show:NAME`, and the values that survive a reload all use it. Without one, a
  path from the root is generated.
- Paths (`Source`, `<Include>`) are looked for next to the document first, then
  in the current folder.

## Value types

| Type | Syntax |
|---|---|
| number | `12`, `0.5`, `-3` (always with a dot, whatever the locale) |
| boolean | `true` / `false`, `yes` / `no`, `on` / `off`, `1` / `0` |
| size | `120` (points), `50%` (of the parent), `fill`, `fit`, `2*` (two shares) |
| color | `#RGB`, `#RGBA`, `#RRGGBB`, `#RRGGBBAA`, a theme token (`accent`, `surface`, `text`, `border`, `danger`, `success`, `series1` to `series6`...) or a name (`white`, `black`, `red`, `green`, `blue`, `yellow`, `orange`, `gray`, `cyan`, `magenta`, `pink`, `purple`, `transparent`) |
| choice | one of the words listed for the attribute, any case |
| list | texts separated by a vertical bar: `Items="One\|Two\|Three"`, and `Icons="house\|radio"` for the icons of those entries |
| numbers | numbers separated by commas: `Values="1,2,3"` |
| action | a name for `cui_xml_on` / `cui_xml_on_any`, or one of the built-ins |

Lengths are logical points: CUI scales them for high DPI screens.

## Values, bindings and actions

**Where a value lives.** A control that holds something (a switch, a slider, a
text field, the selected tab) keeps it in its element unless the app binds it:

```c
static bool notify = true;
static float scale = 1.0f;
cui_xml_bind_bool(doc, "notify", &notify);
cui_xml_bind_float(doc, "scale", &scale);
```

The binding is found by the element's `Name`; `Bind="name"` pairs a control
with a binding whose name differs. `cui_xml_get_bool`, `get_int`, `get_float`,
`get_text` read what a control holds now, `set_*` writes what it shows, and
`cui_xml_changed(doc, "name")` is true during the frame it changed.

**Actions.** `OnClicked`, `OnChanged`, `OnSubmitted` and `OnAction` name an
action:

```c
static void on_action(cui_xml *doc, cui_xml_node *node, const char *action, void *user) {
    if (!strcmp(action, "file.save")) save();
}
cui_xml_on(doc, "file.save", on_action, NULL);   /* one name */
cui_xml_on_any(doc, on_action, NULL);            /* everything else */
```

Some names need no C at all. They apply when no function is bound to the name:

| Action | What it does |
|---|---|
| `app.quit` | `cui_xml_quit_requested(doc)` turns true |
| `theme.dark`, `theme.light`, `theme.toggle` | the document's theme, applied on the next frame |
| `accent:NAME` | `neutral`, `violet`, `blue`, `green`, `amber`, `orange`, `coral`, `rose` |
| `show:NAME`, `hide:NAME`, `toggle:NAME` | whether an element is drawn (a `Dialog`, an `Alert`, any element) |
| `enable:NAME`, `disable:NAME` | whether it takes input |
| `page:NAME=2`, `page:NAME=+1`, `page:NAME=-1` | the index of a `Tabs`, `Segmented`, `Select`, `DocTabs` or `Carousel` |
| `popover:NAME` | opens or closes the `<Popover Name="NAME">` |
| `copy:TEXT` | the text to the clipboard |

`cui_xml_set_builtins(doc, false)` turns them off (a preview shows an interface
without running it), and `cui_xml_is_builtin(name)` lists them.
`cui_xml_fire(doc, "name")` fires one from C, and `cui_xml_click(doc, "name")`
clicks a control: its action, or its value flipping.

**{"..."}** is computed with numbers, texts in quotes, `+ - * / %`,
`== != < <= > >=`, `&& || !`, `a ? b : c`, parentheses, and the functions
`min`, `max`, `clamp`, `round`, `floor`, `ceil`, `abs`, `len`, `upper`, `lower`,
`empty`, `default`. A name it reads comes from the element's parameters (in a
`<Widget>`), the document's variables (`<Var Name="count" Value="7"/>` or
`cui_xml_set_var`), or a binding:

```xml
<Text>Count: {count}</Text>
<Text>{upper(name)} ({len(name)} letters)</Text>
<Button Text="{wide ? 'Save all' : 'Save'}" OnClicked="file.save"/>
```

## The elements

Every element takes the **common attributes**: `Name`, `Id`, `Bind`, `Text`,
`Width`, `Height`, `Fill`, `FillV`, `Pad`, `PadX`, `PadY`, `Gap`, `Align`,
`Justify`, `If`, `Visible`, `Disabled`, `Tooltip`, `OnClicked`, `OnChanged`,
`OnSubmitted`, `OnAction`, `Items`, `Item`, `Into`.

`Fill="1"` takes a share of the free width in a row, `FillV="1"` of the free
height in a column; `Width="50%"` takes half of the parent.

### Containers

| Tag (aliases) | What it is | Attributes of its own |
|---|---|---|
| `Window` | the root: a column that fills the window | `Title`, `Theme` (`light`/`dark`), `Accent`, `Background` |
| `Column` (`VBox`) | children stacked top to bottom | `Pad`, `Gap`, `Align`, `Justify`, `Background`, `Border`, `BorderColor`, `Radius`, `Elevation` |
| `Row` (`HBox`) | children left to right | the same, plus `Wrap` |
| `Stack` (`Overlay`) | children on top of each other | the same |
| `Grid` | children in columns of equal width | `Columns`, `MinColumnWidth` |
| `Box` (`Panel`) | a plain container | the same as `Column`, plus `Direction` (`column`, `row`, `stack`, `grid`), `MinWidth`, `MaxWidth`, `MinHeight`, `MaxHeight` |
| `Center` | its child centered on both axes | |
| `Card` | a bordered surface with generous padding | `Hoverable`, `Muted`, `Selected`, `Dir="row"` |
| `Scroll` (`ScrollBox`) | a scrolling area | `Horizontal`, `Scroll`, `Fade` |
| `Split` | two children with a draggable divider | `Ratio`, `Vertical`, `MinFirst`, `MinSecond` |
| `Spacer` | flexible space | |
| `Gap` | a fixed space | `Size` |
| `Separator` | a hairline | |

### Text and icons

| Tag | What it is | Attributes |
|---|---|---|
| `Heading` | a heading | `Level` 1 to 4 |
| `Text` | body text | `Size`, `Color`, `Font` (`body`, `medium`, `display`, `mono`), `Align`, `Nowrap`, `Tabular` (figures of equal width), `LineHeight` |
| `Label` | medium weight text | |
| `Secondary` | secondary color text | |
| `Muted` | small, muted text | |
| `Code` | inline code | |
| `Kbd` | a keyboard keycap, `Ctrl K` | |
| `Link` | a link; `OnClicked` fires | |
| `Icon` | one Lucide icon | `Icon` (a name such as `audio-lines` or `play`, from `include/cui/cui_icons.h`; a glyph still works), `Size`, `Color` |

### Controls

| Tag | What it holds | Attributes |
|---|---|---|
| `Button` | fires `OnClicked` | `Variant` (`default`, `secondary`, `primary`, `outline`, `ghost`, `danger`, `link`), `Size`, `Icon`, `IconRight`, `Loading`, `FullWidth`, `Selected`, `Shortcut` |
| `IconButton` | fires `OnClicked` | `Icon`, `Tooltip` |
| `Checkbox` | bool | `Description` |
| `Switch` | bool | `Description` |
| `RadioGroup` | the selected index, from its `<Radio>` children | |
| `Radio` | one option of a `RadioGroup` | `Text` |
| `Slider` | a number | `Min`, `Max`, `Value`, `Step`, `ShowValue`, `Format` |
| `RangeSlider` | `Low` and `High` | `Min`, `Max`, `Low`, `High` |
| `Rating` | an int | `Max` |
| `Chip` | bool | `Icon` |
| `Input` | text | `Label`, `Placeholder`, `Hint`, `Error`, `Icon`, `Password`, `Clearable`, `ReadOnly`, `Autofocus`, `Prefix`, `Suffix`, `Value`, `MaxLength` |
| `Textarea` | text | as `Input`, plus `Height` |
| `Search` | text | as `Input` |
| `Number` | a number | `Label`, `Min`, `Max`, `Step`, `Decimals`, `Suffix` |
| `Select` | the selected index | `Label`, `Placeholder`, `Searchable`, `Items` or `<Item>` children, `Icons` |
| `Segmented` | the selected index | `Size`, `FullWidth`, entries, `Icons` |
| `Tabs` | the selected index | `Variant` (`underline`/`pill`), entries, `Icons` |
| `NavItem` | fires `OnClicked` | `Icon`, `Badge`, `Selected`, `Group` |
| `NavSection` | a small heading | |
| `Breadcrumb` | reports the clicked index | entries |
| `Pagination` | the page, 0 based | `PageCount` |
| `Stepper` | shows the current step | `Current`, entries |
| `DocTabs` | the selected index | `Shortcuts`, `NoClose`, `NoReorder`, `<Tab>` children |
| `Carousel` | the selected slide | `<Slide>` children |
| `ButtonGroup` | buttons drawn connected, on one well | `<Button>` children |
| `CopyButton` | an icon button that copies `Copy`, or its text | `Copy` |
| `HoldButton` | fires `OnClicked` once the button is held full | `Duration`, `Icon`, `Variant` |
| `TagInput` | text: tags separated by commas | `Label`, `Placeholder`, `MaxTags`, `Value` |
| `Otp` | text: a one time code | `Length`, `Value` |
| `PasswordStrength` | a meter of the bound text's strength | `Bind` names the password |
| `DatePicker` | a date with a calendar | `Label`, `Placeholder`, `Year`, `Month`, `Day` |
| `RadioCards` | the chosen card's index | `Columns`, `<Choice>` children |
| `Choice` | one card of a `RadioCards` | `Description`, `Icon`, `Badge` |

### Display

| Tag | What it is | Attributes |
|---|---|---|
| `Progress` | a progress bar | `Value` 0..1, `Indeterminate`, `Tone`, `Label`, `ShowValue`, `Dither`, `Height` |
| `ProgressRing` | circular progress with the value in the middle | `Value` 0..1, `Size` |
| `Spinner` | a loading spinner | `Size`, `Style` (`arc`, `comet`, `dotmatrix`, `radar`, `ripple`, `bounce`...) |
| `StatusSpinner` | one job's state: idle, busy, success, error | `Status`, `Size`, `Keep` |
| `Skeleton` | a placeholder | `Width`, `Height` |
| `SkeletonGroup` | skeleton bars that animate together | `Dither`, `Gap` |
| `TimelineItem` | one row of an activity timeline | `Time`, `Description`, `Icon`, `Tone`, `Last` |
| `Readout` | a label, a big value with its unit, a line under it | `Label`, `Value`, `Unit`, `Detail`, `Size`, `Tone`, `Accent` |
| `DotDisplay` | text in the 5 by 7 dot font, for clocks and counters | `Pitch`, `Color`, `Accent`, `NoDim`, `Marquee` |
| `Badge` | a small tone-colored label | `Tone`, `Dot`, `Solid`, `Icon`, `Size` |
| `Avatar` | initials or a picture | `Size`, `Status`, `Square` |
| `AvatarGroup` | a stack of avatars | `Items`, `Max`, `Size` |
| `Metric` | a big value with its trend | `Label`, `Value`, `Delta`, `DeltaLabel`, `Icon`, `Values` (a sparkline), `LowerIsBetter` |
| `Meter` | a labelled usage bar | `Label`, `Used`, `Total`, `Unit` |
| `EmptyState` | an empty state | `Description`, `Icon`, `Action`, `OnAction` |
| `Alert` | an inline alert | `Title`, `Description`, `Tone`, `Icon`, `Dismissible`, `Open` |
| `Banner` | a banner | `Tone`, `Icon`, `Action`, `Dismissible`, `Open` |
| `Accordion` | a collapsible section | `Title`, `Open` |
| `Tree` | a tree node with its children under it | `Text`, `Open`, `Icon`, `Leaf`, `Selected` |
| `ListItem` | a selectable row | `Text`, `Subtitle`, `Trailing`, `Icon`, `Selected`, `Avatar` |
| `Table` | columns and rows | `Striped`, `Compact`, `Selectable`, `Height`, `SortColumn`, `SortDescending` |
| `TableColumn` | one column | `Title`, `Width`, `Align`, `Sortable` |
| `TableRow` | one row of `<Cell>` | `Selected`, `OnClicked` |
| `Cell` | one cell: `Text`, or elements | |
| `Markdown` | markdown as native content | `Text` or `Source` |
| `CodeBlock` | code with highlight | `Text` or `Source`, `Lang`, `Title`, `LineNumbers` |
| `Json` | a collapsible JSON tree | `Text` or `Source`, `ExpandDepth` |
| `LineChart`, `BarChart` | charts | `Height`, `Area`, `Smooth`, `Stacked`, `Dither`, `NoGrid`, `NoLegend`, `Format`, `Labels` |
| `Series` | one series | `Name`, `Values`, `Color` |
| `DonutChart` | a donut | `Height`, `Format`, `<Slice Text="..." Value="..."/>` children; no `Width` fits the ring and the legend |
| `Sparkline` | a small line | `Values`, `Height`, `Color` |
| `Gauge` | a round gauge | `Label`, `Value`, `Size` |
| `PatternMeter` | a stacked usage bar | `Total`, `Unit`, `Label`, `FreeLabel`, `Grayscale`, `Height`, `<Part Name Value/>` children |
| `Image` | a picture | `Source`, `Width`, `Height`, `Radius` |

### App chrome

| Tag | What it is | Attributes |
|---|---|---|
| `Toolbar` | a strip of icon buttons | `Style` (`plain`, `grouped`, `floating`) |
| `ToolbarButton` | one icon button | `Icon`, `Tooltip`, `Label`, `Toggle`, `Shortcut`, `Tone`, `Flyout` |
| `ToolbarPrimary` | the main action of the strip | `Icon`, `Tooltip`, `Label` |
| `ToolbarRadio` | icons of which one is on | `Items`, the index |
| `ToolbarGroup`, `ToolbarSeparator`, `ToolbarSpacer` | layout of the strip | |
| `StatusBar` | a row of small items | `NoBorder` |
| `StatusText`, `StatusItem`, `StatusProgress`, `StatusSpacer` | its items | `Text`, `Icon`, `Tone`, `Value` |
| `MenuBar` | a menu bar | |
| `MenuBarMenu` | one menu of the bar | `Text`, `Width`, `Disabled` |
| `Menu` | a button that opens a dropdown | `Text`, `Icon`, `IconOnly`, `Variant`, `Placement` |
| `MenuItem` | one item | `Text`, `Icon`, `Shortcut`, `Danger`, `Checked`, `Disabled`, `OnClicked` |
| `MenuSeparator`, `MenuLabel` | lines and headings inside a menu | |
| `Dialog` | a modal dialog | `Title`, `Description`, `Width`, `Persistent`, `Open` |
| `DialogFooter` | the row of buttons at the bottom of a dialog | `<Button>` children |
| `Popover` | a panel anchored to the element before it | `Width`, `Placement`, `Gap`; `popover:NAME` opens it |
| `ContextMenu` | a menu on right click over the element before it | `<MenuItem>` children |
| `StatusTag` | a colored state tag | `Text`, `Status`, `Tone`, `Icon`, `Removable` |
| `ColorPicker` | a color | `Value` |
| `Reorder` | children the user drags into another order | `<ReorderItem>` children; `OnChanged` fires |
| `ReorderItem` | one row of a `Reorder` | its children are drawn inside the row |

### Logic and structure

| Tag | What it is |
|---|---|
| `Shortcut` | `Keys="Ctrl+S"` and `Action="..."`: a key press anywhere fires the action, except while a text field has the keyboard |
| `Var` | `Name` and `Value`: a variable `{"..."}` can read |
| `Include` | `Source="file.xml"`: brings in that file's elements and declarations |
| `Repeat` | its children `Count` times, `Var` names the index (`i`) |
| `ForEach` | its children once per `Items` entry, `Var` names the entry (`item`) |
| `Widget`, `Widgets` | a tag of your own (see below) |
| `Children` | inside a `<Widget>`: where the children given where it is used go |

## Tags of your own

A document can declare a tag and use it like any other:

```xml
<Widget Name="Field" Label="Name" Hint="Type here" Wide="false">
  <Row Gap="10" Align="center">
    <Label Width="{Wide ? 120 : 84}">{Label}</Label>
    <Input Name="field-{Label}" Placeholder="{Hint}" Fill="1"/>
    <Children/>
  </Row>
</Widget>

<Field Label="Owner" Wide="true" Name="owner">
  <Chip Text="shared" Bind="shared"/>
</Field>
```

- The attributes of `<Widget>` other than `Name` are its parameters, with their
  default values. An instance's attributes with those names override them.
- Inside, `{"..."}` reads the parameters.
- `<Children/>` is replaced by the children given where the tag is used;
  `<Children Into="x"/>` takes only those with `Into="x"`.
- What the instance writes that is not a parameter (`Name`, `Width`,
  `OnClicked`, `Tooltip`...) goes to the element the declaration becomes.
- A declaration is known from the moment it is read, for that document and the
  files it includes. `cui_xml_register_widgets(doc, text, size)` adds a library
  of them to a document, and `cui_xml_is_widget` lists them.

Tags can also be made by C: `cui_xml_register_tag(doc, "MyTag", fn, user)` draws
whatever it wants. It reads the element with `cui_xml_node_attr` (as written) and
`cui_xml_node_attr_value` (with `{"..."}` computed), and an element that behaves
like a container calls `cui_xml_draw_children` where its children go:

```c
static bool my_row(cui_xml *doc, cui_xml_node *node, cui_ctx *ui, void *user) {
    const char *label = cui_xml_node_attr_value(doc, node, "label");
    cui_box b = { .id = cui_xml_node_name(node), .dir = CUI_COLUMN, .gap = 6 };
    cui_box_begin(ui, &b);
    cui_label(ui, label);
    cui_xml_draw_children(doc, node, ui);
    cui_box_end(ui);
    return true;
}
```

`examples/music.c` (a music player in C) and `examples/music_xml.c` with
`examples/music.xml` (the same player described in a document) are the pair to
read: the document holds the layout, the C file holds the track table, the
cover art, the equalizer and the actions, and the two render pixel for pixel
alike. `musicxml --shot out.png` and `music --shot out.png` draw both offscreen
with the software renderer, which is how they are compared.

The component gallery is the larger pair: `examples/gallery_ui.c` (nine pages of
components) and `examples/gallery.xml` with `examples/gallery_xml_ui.c`, where
one document holds the sidebar, the page list and every page, `page` picks the
one drawn, and two tags made by the C side paint the accent swatches (`<Swatch>`)
and build the ten thousand row virtual list (`<Invoices>`).
`tests/xml_gallery_shots.c` draws each page both ways with one viewport, theme
and accent, ninety frames each, and compares the pictures byte for byte: they
are the same, and the test fails when one of them moves. It is the place to look
when an element does not lay out the way the C call does, and
`tools/diff_regions.py` says where two pictures part (`python
tools/diff_regions.py a.png b.png`).

Every other example has its pair too, and every pair draws the same bytes: the
C example with `--shot out.png` and the XML one with the same flag, compared by
`tests/xml_pair_shots.c` (one `ctest` entry per pair, `xml_pair_<name>`):

| C example | document | what a document cannot say, so the C side keeps it |
|---|---|---|
| `hello.c` | `hello.xml` | the click counter |
| `todo.c` | `todo.xml` | the tasks, the counter, saving |
| `caffeine.c` | `caffeine.xml` | the model (one curve per cup, peak, verdict); `<Level>` and `<Cups>` draw the chart and the track |
| `notes.c` | `notes.xml` | the notes; `<Notes>` draws the rows and their context menu |
| `settings.c` | `settings.xml` | the form's values, the section list, the theme and the accents |
| `dashboard.c` | `dashboard.xml` | the orders, the rolling totals, the drawer |
| `assistant.c` | `assistant.xml` | the conversation; nine tags draw answers, tool calls, plan, composer and so on |
| `mail.c` | `mail.xml` | the messages; `<Messages>` draws a list row, `<Drawer>` a sheet |
| `finance.c` | `finance.xml` | the transactions, the painted card, the command palette |
| `tracker.c` | `tracker.xml` | the issues; `<Issues>`, `<StatusGlyph>`, `<PriorityGlyph>` paint the board |
| `gallery_ui.c` | `gallery.xml` | the state of every control, the accent swatches, the invoice list |
| `music.c` | `music.xml` | the tracks, the cover art, the equalizer |

The window of an XML example is driven by `cui_app_run` like everywhere else:
`cui_xml_render(doc, ui)` in the frame callback, `cui_xml_poll(doc)` before it
so the file is followed, and `cui_app_quit(NULL)` when the document asks for
it. `examples/hello_xml.c` is the smallest of the thirteen and the one to copy.

## Repeat, ForEach, If and Include

```xml
<Repeat Count="3" Var="i">
  <ListItem Text="Member {i}" Trailing="{i}h" Icon="user"/>
</Repeat>

<ForEach Items="red|green|blue" Var="tone">
  <Badge Text="{tone}" Tone="{tone}"/>
</ForEach>

<Text If="flag">shown when the bound bool "flag" is true</Text>
<Include Source="parts/toolbar.xml"/>
```

The names are baked when the document is read, so `Name="row{i}"` gives
`row0`, `row1`, `row2` 鈥?one element, one binding, one value each. `If` is
computed every frame, so it follows the value.

## Following the file

A document read from a file follows it: `cui_xml_poll(doc)` reads the file again
when its timestamp or size changed, applies what changed, and keeps what the
elements hold (match by `Name`). A file with a mistake leaves the interface as
it was and reports the error:

```c
static void on_reload(cui_xml *doc, bool ok, void *user) {
    if (!ok) show_message(cui_xml_last_error(doc));
}
cui_xml_set_reload_callback(doc, on_reload, NULL);
```

`cui_xml_set_watch(doc, false)` stops looking. Texts given from memory
(`cui_xml_parse`) are never followed.

## Tools: the catalog

A designer, an editor or a help panel can ask what can be written:

```c
for (int i = 0; i < cui_xml_tag_count(); i++) {
    const char *tag = cui_xml_tag_name(i);            /* "Window", "Button"... */
    int children = cui_xml_tag_children(tag);         /* 0 none, 1 one, 2 any */
    for (int a = 0; a < cui_xml_attribute_count(tag); a++) {
        cui_xml_attribute_desc info;
        cui_xml_attribute_info(tag, a, &info);         /* name, kind, choices, group */
    }
}
```

`cui_xml_attribute_info` is the same list this page documents: the common
attributes first, then the tag's own.

`tools/check_xml_document.py` checks a document against that list without
opening a window, so a typo, an attribute no tag reads, an icon name that is not
in `cui_icons.h`, a `Fill` that is not a share and a file that a strict XML
parser rejects are all reported with the element they are on:

```sh
python3 tools/check_xml_document.py examples/music.xml examples/music_xml.c
```

The tags and attributes it accepts come from the layer's own sources, and the
`.c` arguments add the tags a demo registers (`cui_xml_register_tag`) and the
attributes those tags read.

## Using one panel of a document

A document can be one part of a frame written in C:

```c
static void frame(cui_ctx *ui, void *user) {
    cui_xml *doc = user;
    cui_row(ui, .gap = 16) {
        cui_column(ui, .fill = 1) { ...your components... }
        cui_xml_render_into(doc, ui, "inspector");   /* a named element */
    }
}
```

With a root other than `<Window>`, `cui_xml_render(doc, ui)` draws that root in
place too, so a document can be a card, a toolbar or a single control.

## Limits worth knowing

- Item lists (`Items`) and chart values (`Values`) are read from the attributes
  and the `<Item>`, `<Series>`, `<Part>` children once: they are the data a
  document brings, not a view of the app's data. For a list that changes at run
  time, a tag made by C is the way in.
- An element's rectangle (`cui_xml_node_rect`) comes from the frame that laid it
  out: after the first frame an element with a stable place has one.
- The document owns everything it creates with `malloc` and frees it in
  `cui_xml_free`; the `cui_ctx` must outlive it (a picture's texture is
  destroyed with the context).
- The gallery pair covers the nine pages and the sidebar. What the gallery
  opens over them — the two dialogs, the notification sheet, the command palette
  and the toasts — is still drawn by `examples/gallery_ui.c` alone: the page
  compares byte for byte, the modals have no document yet (`<Dialog>` can say
  the two dialogs; a sheet and the palette need a tag first, and the palette's
  commands are the app's data).
