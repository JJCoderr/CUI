#!/usr/bin/env python3
"""Checks an XML document against what the CUI XML layer actually reads.

    python3 tools/check_xml_document.py CUI/examples/music.xml [extra.c ...]

What a tag and an attribute may be is taken from the layer's own sources: the
tag list and the catalog in src/cui_xml_widgets.c, every attribute name its
readers ask for, the tags a demo registers with cui_xml_register_tag, and the
attribute names those tags read. Anything else in the document is silently
ignored when it is drawn, so it is reported here instead.

It also checks that the file is well-formed XML (a strict parser rejects `--`
inside a comment, which is easy to write by accident) and that icon attributes
name an icon of include/cui/cui_icons.h.
"""
import os
import re
import sys
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WIDGETS = os.path.join(ROOT, "src", "cui_xml_widgets.c")
CORE = os.path.join(ROOT, "src", "cui_xml.c")
INTERNAL = os.path.join(ROOT, "src", "cui_xml_internal.h")
ICONS = os.path.join(ROOT, "src", "cui_xml_icons.h")

BOOL_ATTRIBUTES = {
    "nowrap", "tabular", "hoverable", "muted", "selected", "wrap", "password", "clearable",
    "readonly", "autofocus", "fullwidth", "loading", "searchable", "indeterminate", "dither",
    "showvalue", "striped", "compact", "selectable", "horizontal", "vertical", "leaf", "square",
    "linenumbers", "noheader", "dirty", "checked", "danger", "disabled", "icononly", "fade",
    "noborder", "shortcuts", "noclose", "noreorder", "nolegend", "nogrid", "stacked", "smooth",
    "area", "removable", "dot", "solid", "grayscale", "draggable", "resizable", "persistent",
    "lowerisbetter", "expanded", "open", "dismissible", "noclosebutton", "closable", "grow",
}

BOOLEAN_WORDS = {"true", "false", "1", "0", "yes", "no", "on", "off"}
NON_BOOLEAN_ATTRIBUTES = {
    "eventtrack": {"selected"},
    "apiexplorer": {"selected"},
    "pipeline": {"selected"},
    "filterbar": {"selected"},
    "kanbancolumn": {"dot"},
}


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def known_tags(extra_sources):
    tags = set(re.findall(r'X\(\w+,\s*"([A-Za-z]+)"', read(INTERNAL)))
    tags |= set(re.findall(r'\{\s*"([A-Za-z]+)",\s*CUI__XML_TAG_', read(WIDGETS)))
    for path in extra_sources:
        tags |= set(re.findall(r'cui_xml_register_tag\(\s*\w+\s*,\s*"([A-Za-z]+)"', read(path)))
    return {t.lower() for t in tags}


def known_attributes(extra_sources):
    names = set()
    for path in [WIDGETS, CORE] + extra_sources:
        text = read(path)
        # any reader shaped (doc, node, "name") or (node, "name")
        names |= set(re.findall(r'\(\s*(?:doc\s*,\s*)?\w+\s*,\s*"([A-Za-z.]+)"', text))
        # the catalog rows: { "Tag", "Name", ... } and { NULL, "Name", ... }
        names |= set(re.findall(r'\{\s*(?:NULL|"[A-Za-z]+")\s*,\s*"([A-Za-z.]+)"\s*,\s*CUI_XML_', text))
    return {n.lower() for n in names}


def known_icons():
    return {m.lower() for m in re.findall(r'\{\s*"([a-z0-9-]+)",', read(ICONS))}


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    doc = argv[1]
    extra = [a for a in argv[2:] if a.endswith(".c")]
    tags, icons = known_tags(extra), known_icons()
    attrs = known_attributes(extra)

    # a document declares tags and attributes of its own with <Widget>
    def declared(el):
        for kid in el:
            if kid.tag.lower() == "widget" and "Name" in kid.attrib:
                tags.add(kid.attrib["Name"].lower())
                attrs.update(k.lower() for k in kid.attrib.keys() if k.lower() != "name")
            declared(kid)

    try:
        root = ET.parse(doc).getroot()
    except ET.ParseError as error:
        print("%s: not well-formed XML: %s" % (os.path.relpath(doc, ROOT), error))
        print('  (a "----" run inside a comment is the usual cause: XML forbids "--" there)')
        return 1

    declared(root)
    problems = []

    def walk(el):
        tag = el.tag.lower()
        if tag not in tags:
            problems.append((el, "unknown tag <%s>: its children are drawn in place" % el.tag))
        for name, value in el.attrib.items():
            low = name.lower()
            if low not in attrs:
                problems.append((el, 'attribute "%s" is never read: it does nothing' % name))
                continue
            if "{" in value:
                continue                      # computed when it is drawn
            if low in ("icon", "iconright", "icons"):
                for part in value.split("|"):
                    part = part.strip()
                    if part and part.isascii() and part.lower() not in icons:
                        problems.append((el, 'icon "%s" is not a name in cui_icons.h: it is drawn as text'
                                         % part))
            if low in ("fill", "fillv") and value not in ("fill", "grow") and not value.endswith("%"):
                try:
                    float(value)
                except ValueError:
                    problems.append((el, 'Fill="%s" is neither a number nor "fill"' % value))
            if (low in BOOL_ATTRIBUTES and low not in NON_BOOLEAN_ATTRIBUTES.get(tag, ())
                    and value.lower() not in BOOLEAN_WORDS):
                problems.append((el, '%s="%s" is not a boolean' % (name, value)))
        for kid in el:
            walk(kid)

    walk(root)

    elements = sum(1 for _ in root.iter())
    print("%s: %d elements, well-formed XML" % (os.path.relpath(doc, ROOT), elements))
    if not problems:
        print("no problem found: every tag and attribute is one the layer reads")
        return 0
    print("%d problem(s):" % len(problems))
    for el, message in problems:
        print("  <%s>: %s" % (el.tag, message))
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
