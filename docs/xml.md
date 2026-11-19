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
- [The elements](#the-elements) —containers, text, controls, display, chrome, logic
- [Tags of your own](#tags-of-your-own)
- [Repeat, ForEach, If and Include](#repeat-foreach-if-and-include)
- [Following the file](#following-the-file)
- [Tools: the catalog](#tools-the-catalog)
- [Using one panel of a document](#using-one-panel-of-a-document)

## How a document becomes a frame

CUI is immediate mode: the app describes its interface every frame. A document
is the same, written once and read every frame. Nothing is retained except what
each element holds —a switch's value, the text of a field, the open tab —and
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
| `Card` | a bordered surface with generous padding | `Hoverable`, `Muted`, `Selected`, `Dir="row"`, `MinWidth`, `MaxWidth` |
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
| `NavItem` | fires `OnClicked`; the items of one `Group` share one highlight, which glides from item to item | `Icon`, `Badge`, `Selected`, `Group` |
| `NavSection` | a small heading | |
| `Toc` | the section in view; clicking scrolls there | `Scroll` names the `Scroll` it follows, `<TocItem>` children |
| `TocItem` | one entry of a `Toc` | `Anchor`, `Level` |
| `TocAnchor` | where a section starts inside that scroll area | `Anchor` |
| `InlineEdit` | text, edited where it stands | `Font`, `Size`, `Placeholder` |
| `Scrub` | a number, dragged sideways | `Min`, `Max`, `Value`, `Step`, `Format`, `Suffix` |
| `MoneyInput` | an amount | `Label`, `Value`, `Symbol`, `Code`, `Decimals`, `Hint` |
| `ShortcutRecorder` | a key combination | `Shortcut`, `Placeholder` |
| `EmojiPicker` | reports the emoji picked | `Height`, `Size`, `Tone`, `NoSearch` |
| `DotChart` | reports clicks with their x | `Height`, `XMin`, `XMax`, `XUnits`, `XStep`, `Min`, `Max`, `Unit`, `Pitch`, `NoAxis`, `<Series>` and `<Annotation>` children |
| `Annotation` | a line or a point over a chart | `Axis` (`x`/`y`/`point`), `At`, `Value`, `Detail`, `Accent` |
| `EventTrack` | moments the user drags, adds and removes | `Values`, `Min`, `Max`, `Step`, `Units`, `LabelStep`, `Selected`, `NoAdd`, `NoRemove` |
| `HoverCard` | a panel that appears while the pointer rests on it | `Width`, `Placement`, `Delay`, children |
| `ConfirmPopover` | a question anchored to the element before it; `OnClicked` when confirmed | `Title`, `Description`, `ConfirmLabel`, `CancelLabel`, `Danger` |
| `ExpandableText` | text clamped to a few lines, with Show more | `Lines`, `Size` |
| `ExpandableCard` | a card whose header opens its children | `Open`, `Description`, `Icon`, `Badge`, `BadgeTone` |
| `Countdown` | digits counting down | `Seconds`, `HideDays`, `Compact` |
| `Checklist` | an onboarding card, with `<Step>` children | `Title`, `Description` |
| `Step` | one step of a `Checklist` | `Description`, `Action`, `Done` |
| `Veil` | covers its children with dots until `Revealed` | `Revealed`, `Label`, `Hint`, `Toggle`, `Radius` |
| `SplitButton` | reports the menu entry chosen | `Items`, `Icons`, `Variant`, `Icon` |
| `ToggleGroup` | the toggles that are on | `Items`, `Icons`, `On`, `IconOnly`, `Exclusive` |
| `ConfirmButton` | `OnClicked` once confirmed | `ConfirmLabel`, `Timeout`, `Icon`, `Variant` |
| `StatusButton` | `OnClicked` while idle | `Status`, `SuccessLabel`, `BusyLabel`, `ErrorLabel`, `Icon` |
| `Reactions` | the pills the user reacted with | `Items` (`icon:count[:on]`) |
| `ThemeSwitcher` | the theme mode | `Mode`, `System` |
| `ThemeToggle` | switches light and dark | |
| `UserMenu` | a menu of account actions around its children | `Detail`, `Status`, `Compact`, `Placement` |
| `SwipeItem` | the action swiped to, in `OnClicked` | `Left`, `Right` (`icon:label:tone`) |
| `SpeedDial` | reports the action chosen | `Items` (`icon:label`), `Tooltip`, `Icon` |
| `MultiSelect` | the items chosen | `Items`, `On`, `Label`, `Placeholder`, `Searchable`, `MaxChips` |
| `DateRangePicker` | a start and an end date | `From`, `To`, `NoPresets`, `Width` |
| `RangeCalendar` | the range picked in one or two months | `From`, `To`, `Months` |
| `TimePicker` | a time | `Hour`, `Minute`, `TwelveHour`, `MinuteStep` |
| `TimeDial` | a time set on a clock face | `Hour`, `Minute`, `Size` |
| `WheelPicker` | the index on the drum | `Items`, `Value`, `Rows`, `Loop`, `JoinLeft`, `JoinRight` |
| `RulerSlider` | a number on a tape measure | `Min`, `Max`, `Value`, `Step`, `MajorEvery`, `Spacing`, `Unit` |
| `HistogramSlider` | the two ends of a range | `Values`, `Min`, `Max`, `Low`, `High` |
| `ChatMessage` | one message of a conversation | `Mine`, `Author`, `Time`, `Status`, `Grouped`, `LastInGroup`, `Markdown` |
| `Reasoning` | what the model thought, folded away | `Thinking`, `Seconds`, `Title`, children |
| `ToolCall` | one tool call, with its input and output | `Status`, `Icon`, `Detail`, `Input`, `Output`, `Seconds` |
| `AgentStatus` | what the agent is doing now | `Status`, `Seconds`, `Icon` |
| `Typewriter` | text that types itself in | `Size`, `Speed`, `Delay` |
| `Citation` | a number that shows one source on hover | `Number`, `From` (a SourceList's Name), `Index` |
| `SourceList` | the documents an answer used | `<Source>` children |
| `Source` | one document | `Title`, `URL`, `Snippet` |
| `ResponseActions` | copy, regenerate, vote and share | `CopyText`, `NoRegenerate`, `NoShare` |
| `ContextMeter` | how full the context is | `Used`, `Limit`, `Unit`, `<Part>` children |
| `ChatComposer` | a growing field with attachments and models | `Placeholder`, `Busy`, `Attachments`, `Models`, `Model` |
| `AgentPlan` | steps with their status | `Title`, `<PlanStep>` children |
| `PlanStep` | one step | `Detail`, `Status` |
| `PromptSuggestions` | cards to start a conversation with | `Columns`, `<Suggestion>` children |
| `Suggestion` | one card | `Icon`, `Detail` |
| `Knob` | a number turned by dragging | `Min`, `Max`, `Value`, `Unit`, `Format`, `Size`, `Reset`, `LogScale`, `Bipolar` |
| `Fader` | a number on a track, with a level meter beside it | `Min`, `Max`, `Value`, `Length`, `Ticks`, `Meter`, `Horizontal` |
| `LevelMeter` | bars that follow the levels given each frame | `Levels`, `Length`, `Horizontal`, `Scale`, `Segmented` |
| `Tour` | a guided tour of elements by name | `Step`, `Width`, `<TourStep>` children |
| `TourStep` | one step of a `Tour` | `Target`, `Description`, `Placement` |
| `RatioSlider` | the share between two named parts | `First`, `Second`, `Value`, `Tone`, `Step`, `Height` |
| `GradientSlider` | a number on a track that shows it | `Min`, `Max`, `Value`, `Track` (`hue`/`temperature`/`opacity`), `Step`, `Swatch`, `Icon`, `Tooltip`, `Color` |
| `StepSlider` | an index that snaps to a few stops | `Min`, `Max`, `Value`, `Icon`, `Tooltip`, `ValueText` |
| `PaletteStrip` | the colors and what is locked | `Colors`, `Locked`, `Seed`, `ValueText`, `NoShuffle` |
| `AspectPicker` | the ratio chosen | `Value`, `Custom`, `CustomLabel`, `Base`, `ValueText` |
| `OptionAction` | the option chosen; `OnClicked` for its action | `Items`, `Value`, `Icon`, `Tooltip`, `ValueText` |
| `SeedField` | a seed and its lock | `Value`, `Locked`, `ValueText`, `RandomText`, `FixedText` |
| `FormatBar` | color, weight, style, alignment and lists | `Tone`, `Bold`, `Underline`, `Link`, `Style`, `Size`, `Align`, `List` |
| `NodeEditor` | a canvas of nodes joined by links | `Height`, `MinZoom`, `MaxZoom`, `NoGrid`, `Minimap`, `<Node>`, `<NodeLink>` children |
| `Node` | one node | `Width`, `Icon`, with `<NodeInput>`, `<NodeOutput>` and `<NodeContent>` children |
| `NodeInput` / `NodeOutput` | a pin | `Pin`, `Type` |
| `NodeContent` | the row of a node that holds components | its children |
| `NodeLink` | a link between two pins | `Link`, `From`, `To` |
| `SelectionRing` | someone else's selection around what it follows | `Color`, `Name` |
| `CanvasControls` | a zoom rail over the previous component | `Content` (`x,y,w,h`), `NoFit`, `StartSide`, `<CanvasToggle>` children |
| `CanvasToggle` | one switch of the rail | `Icon`, `Text`, `On` |
| `LiveCursors` | other people's pointers | `Users` (`Name:tone[:speaking]`), `View` from the canvas |
| `PresencePill` | who is here | `Users`, `Size`, `Visible` |
| `Loader` | a looping wait | `Style` (`dots`/`bars`/`pulse`/`orbit`/`grid`), `Size` |
| `ShimmerText` | text with a highlight sweeping across it | `Size`, `Speed` |
| `TextReveal` | words that rise into place | `Size`, `Font`, `Delay` |
| `WordRotate` | one word at a time | `Words`, `Interval`, `Size`, `Font` |
| `GradientText` | text painted with a gradient | `From`, `To`, `Animate`, `Size`, `Font` |
| `Spotlight` | a card with a light that follows the pointer | `Fill`, `Beam`, `Pad`, `Color` |
| `Dock` | a row of icons that magnify | `Size`, `Magnify`, `<DockItem>` children |
| `DockItem` | one icon | `Icon`, `Color`, `Running`, `Badge` |
| `DockSeparator` | a line between groups | |
| `ControlTile` | a large toggle | `On`, `Icon`, `Detail`, `Wide`, `Width` |
| `NowPlaying` | artwork, a scrubber and transport | `Title`, `Artist`, `ArtColor`, `Position`, `Duration`, `Playing` |
| `Waveform` | bars from peaks, seekable | `Values`, `Progress`, `Height` |
| `TabBar` | a bottom tab bar | `Selected`, `Width`, `<TabItem>` children |
| `TabItem` | one tab | `Icon`, `Badge` |
| `Treemap` | rectangles by value | `Height`, `Format`, `<Part>` children |
| `Funnel` | narrowing stages | `Format`, `<Part>` children |
| `SplitBar` | one bar split into parts | `Format`, `NoLegend`, `<Part>` children |
| `Waffle` | a grid of squares by share | `Cell`, `Columns`, `Rows`, `<Part>` children |
| `BarList` | ranked bars | `Format`, `MaxRows`, `KeepOrder`, `<Part>` children |
| `ActivityRings` | concentric rings | `Size`, `Center`, `Caption`, `<Part>` children |
| `Heatmap` | cells from `Values`, `Rows` and `Cols` | `RowLabels`, `ColLabels`, `ColLabelStep`, `Format` |
| `CalendarHeatmap` | a year of activity | `Values`, `Days`, `Start`, `Unit` |
| `RadialGauge` | a value on an arc | `Value`, `Min`, `Max`, `Label`, `Unit`, `Format`, `Detail`, `Style`, `Bands`, `Target`, `ShowTarget` |
| `Radar` | series over shared axes | `Axes`, `Size`, `Max`, `<Series>` children |
| `Scatter` | points per series | `Height`, `XLabel`, `YLabel`, `<Series Points="x,y;..." Sizes="...">` children |
| `Sankey` | flows between named nodes | `Nodes`, `Links` (`from-to:value`), `Height` |
| `Kanban` | a board; `OnChanged` when a card is dropped | `<KanbanColumn>` children |
| `KanbanColumn` | one column | `Dot`, `Limit`, `Width`, `<KanbanCard>` children |
| `KanbanCard` | one card | `Card` (its id), children are its content |
| `WeekCalendar` | seven days of events | `StartHour`, `EndHour`, `Height`, `<Event>` children |
| `Event` | one event | `Day`, `Start`, `End`, `Detail`, `Tone` |
| `Gantt` | a plan of bars | `Days`, `Today`, `Height`, `<Task>` children |
| `Task` | one bar | `Owner`, `Start`, `End`, `Progress`, `DependsOn`, `Milestone` |
| `MonthCalendar` | a month grid | `<MonthEvent>` children |
| `MonthEvent` | one event | `Day`, `Span`, `Time`, `AllDay`, `Tone` |
| `VoiceNote` | a recorded message with its waveform | `Levels`, `Mine`, `Duration`, `Position`, `Speed`, `Playing` |
| `VoiceWave` | a strip of levels, one column each | `Levels`, `Owners`, `Clip`, `Seek`, `Progress`, `Pitch`, `Reply` |
| `VoiceSession` | a spoken conversation with its transcript | `State`, `Levels`, `Owners`, `Transcript`, `Upcoming`, `Reply`, `Muted`, `NoEnd` |
| `ChatDivider` | a line with a label between groups of messages | its text |
| `TypingIndicator` | bouncing dots under the last message | the name, e.g. `Jonas` |
| `Comment` | one comment, its likes and its badge | `Author`, `Time`, `Depth`, `Likes`, `Liked`, `Edited`, `Badge` |
| `Presence` | overlapping avatars of who is here | `Users` (`Name:#rrggbb[:away]`), `Visible`, `Size` |
| `Counter` | a figure whose changed digits roll | `Value`, `Format`, `Size`, `GroupThousands` |
| `Ticker` | a strip of figures scrolling by | `Items` (`Label:Value:Delta[:lower]`), `Speed`, `Height` |
| `CardStack` | cards a flick throws away | `Count`, `Width`, `Height`, `Visible`, `<CardStackItem>` children |
| `CardStackItem` | one card of a `CardStack` | its children are the card
| `Breadcrumb` | reports the clicked index | entries |
| `Pagination` | the page, 0 based | `PageCount` |
| `Stepper` | shows the current step | `Current`, entries |
| `DocTabs` | the selected index | `Shortcuts`, `NoClose`, `NoReorder`, `<Tab>` children |
| `Carousel` | the selected slide | `<Slide>` children |
| `Slide` | one slide of a `Carousel` | its children are the slide |
| `FilterBar` | a search field and chips; holds what is chosen and typed | `Selected` (`5,0,0` masks), `Search`, `<Field>` children |
| `Field` | one filter of a `FilterBar` | `Icon`, `Options` (separated by `\|`) |
| `TreeTable` | a table with disclosure and indentation | `Columns` (`Name:fill:left\|Files:80:right`), `<TreeRow>` children |
| `TreeRow` | one row of a `TreeTable` | `Depth`, `Open`, `Icon`, `Cells` (the other columns, `\|`) |
| `Problems` | diagnostics grouped by file | `Height`, `<Diagnostic>` children |
| `Diagnostic` | one diagnostic | `Severity`, `File`, `Line`, `Column`, `Source`, `Code` |
| `KvEditor` | key and value rows; holds the pairs | `<Pair>` children |
| `Pair` | one pair | `Key`, `Value`, `Enabled`, `Secret` |
| `Pipeline` | stages with their jobs | `Selected`, `<Stage>` and `<Job>` children |
| `Stage` | one stage name | its text |
| `Job` | one job | `Stage`, `Status`, `Duration`, `AllowFailure` |
| `QueryBuilder` | nested groups over fields | `Sentence`, `<Field>`, `<Condition>` children |
| `Condition` | a group or a rule | `Depth`, `Group` (`all`/`any`), `Field`, `Op`, `Value` |
| `SchedulePicker` | a cron expression, edited friendly | `Cron`, `Now`, `Utc`, `NextCount` |
| `Keymap` | a keyboard of shortcuts | `Mods`, `<Category>`, `<Binding>` children |
| `Binding` | one shortcut | `Keys` (`primary+s`, `f5`), `Category` |
| `Category` | one category name | its text |
| `TestExplorer` | suites and tests | `Height`, `<Test>` children |
| `Test` | one suite or test | `Depth`, `Status`, `Duration`, `Message` |
| `StackTrace` | an exception and its frames | `Title`, `Message`, `Lang`, `<Frame>` children |
| `Frame` | one frame | `File`, `Line`, `Column`, `Library`, `Source`, `First` |
| `RequestBar` | the method, the URL and Send | `Method`, `Url` |
| `ResponseStatus` | a response in one line | `Code`, `Time`, `Size` |
| `UrlParams` | a URL's query as pairs; holds them | `Url` |
| `Coverage` | coverage per file | `Height`, `<CoverageFile>` children |
| `CoverageFile` | one file | `Lines`, `LinesHit`, `Branches`, `BranchesHit`, `Functions`, `FunctionsHit` |
| `FeatureFlags` | a switch per environment | `<Env>` and `<Flag>` children |
| `Flag` | one flag | `Description`, `Envs` (`on,on,off`), `Rollouts` |
| `Env` | one environment name | its text |
| `ApiExplorer` | endpoints by tag | `Selected`, `<Endpoint>` children |
| `Endpoint` | one endpoint | `Method`, `Path`, `Category`, `Deprecated`, `<Param>` children |
| `Param` | one parameter | `In`, `Type`, `Required`, `Description` |
| `RegexTester` | a pattern and a sample text | `Pattern`, `Text`, `Height` |
| `ButtonGroup` | buttons drawn connected, on one well | `<Button>` children |
| `CopyButton` | an icon button that copies `Copy`, or its text | `Copy` |
| `HoldButton` | fires `OnClicked` once the button is held full | `Duration`, `Icon`, `Variant` |
| `TagInput` | text: tags separated by commas | `Label`, `Placeholder`, `MaxTags`, `Value` |
| `Otp` | text: a one time code | `Length`, `Value` |
| `PasswordStrength` | a meter of the bound text's strength | `Bind` names the password |
| `DatePicker` | a date with a calendar | `Label`, `Placeholder`, `Year`, `Month`, `Day` |
| `RadioCards` | the chosen card's index | `Columns`, `<Choice>` children |
| `Choice` | one card of a `RadioCards` | `Description`, `Icon`, `Badge` |
| `Dropzone` | files dropped on it: `OnChanged` reports how many, `OnClicked` when the area is clicked | `Title`, `Description`, `Icon`, `Accept`, `Height`, `Compact` |
| `SignaturePad` | the strokes drawn on it; `OnChanged` fires | `Height`, `Placeholder` |
| `AvatarUpload` | a picture dropped on the avatar; `OnChanged` fires | `Size` |

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
`row0`, `row1`, `row2` —one element, one binding, one value each. `If` is
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
  opens over them —the two dialogs, the notification sheet, the command palette
  and the toasts —is still drawn by `examples/gallery_ui.c` alone: the page
  compares byte for byte, the modals have no document yet (`<Dialog>` can say
  the two dialogs; a sheet and the palette need a tag first, and the palette's
  commands are the app's data).

## Moving a page of the gallery into the document

All thirty pages are drawn by `examples/gallery.xml`, and
`tests/xml_gallery_shots.c` compares every one of them, byte for byte, in both
themes. The tags some pages need are not part of the library: a browser over the
disk, a terminal, a generated data grid, a trace of four hundred spans and a
painted picture are registered by the gallery host (`<PeopleGrid>`,
`<FileBrowser>`, `<Shell>`, `<TraceView>`, `<ResultCard>` and others), because
what they hold is data or a texture, not an attribute. A page moves over in five steps:

1. Read the page in `examples/gallery_more.c` and list its components and where
   their data comes from.
2. Add the missing tags: a name in `src/cui_xml_internal.h`, a draw function and
   a row in the attribute table in `src/cui_xml_widgets.c`.
3. Write the page into `examples/gallery.xml` under `If="page == N"`. Data a
   document cannot work out — a curve, a level each frame, a generated picture —
   is computed in `examples/gallery_xml_ui.c` and handed over as a value
   (`{traffic}`), or drawn by a tag the host registers when it needs a texture
   (`<CanvasPicture>`, `<PriceChart>`, `<SceneViewer>`).
4. Compare until it is identical, then in both themes, and add the page number
   to `converted[]`:

   ```sh
   cmake --build build --parallel
   ./build/tests/cui_xml_gallery_shots /tmp/shots 17 light
   ./build/tests/cui_xml_gallery_shots /tmp/shots 17 dark
   ```

5. Run the whole suite and update this page's rows, `CHANGELOG.md` and the
   coverage note at the top of `examples/gallery.xml`.

What has bitten so far, all of it found by the byte comparison rather than by
reading the code:

- **A name is a key**: `cui_xml_bind_int(doc, "mode", &v)` makes every element
  called `mode` read and write `v`, and a stateful element whose `Name` matches a
  bound name takes the binding as its own value. Two elements that share a name
  also share the state saved between frames. Give a control that keeps a value
  its own name (`tool-mode` next to the theme's `mode`), or no name at all.

- **Arrays handed to a component must live in frame memory**
  (`cui__frame_alloc`): components keep the pointer in paint data and read it
  after the call.
- **`cui__xml_attr_list` keeps one list per element**, so two lists on one
  element (a heatmap's row and column labels) need splitting by hand.
- **Line breaks are normalized**, so a document written on Windows draws the
  same text as the C example.
- **Numbers cross as text**: `%.9g` round-trips a `float` exactly, `%.6f` does
  not, and a last-bit difference shows up as a handful of pixels.
- **Animated components are read from the frame or a shared clock**, never from
  a `static` that carries over from the previous run; the C example is adjusted
  the same way when it depends on one.
- **A tag and an app's own tag share one namespace**: registering a name the
  library already uses fails, so `examples/assistant.xml` calls its own message
  tag `<Message>`.
- **Elements with children take their title from `Text`**, not from the text
  inside them.

