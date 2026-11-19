/* The component gallery of examples/gallery_ui.c, with its interface in
 * examples/gallery.xml. Shared by the galleryxml app and the comparison test
 * (tests/xml_gallery_shots.c), so both draw the same frames from one filter. */
#ifndef GALLERY_XML_UI_H
#define GALLERY_XML_UI_H

#include <cui/cui_xml.h>

#include "gallery_ui.h"

/* Reads `path` and wires it to `g`'s values; NULL when the file cannot be
 * read. The document keeps a pointer to `g`, so it must outlive it. */
cui_xml *gallery_xml_open(const char *path, gallery *g);

void gallery_xml_render(cui_xml *doc, cui_ctx *ui, gallery *g);

#endif
