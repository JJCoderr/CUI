/* Extended gallery pages: the newer components, one page per family. */
#ifndef GALLERY_MORE_H
#define GALLERY_MORE_H

#include <cui/cui.h>

int gallery_more_count(void);
const char *gallery_more_name(int page);
const char *gallery_more_icon(int page);
void gallery_more_page(cui_ctx *ui, int page);

/* The illustration the disclosure and canvas pages show, for hosts that draw it
 * from a document: a document holds text, not a procedurally painted picture. */
cui_texture gallery_more_canvas_picture(cui_ctx *ui);

/* The 640 by 400 scene the social page shows, in color or in grayscale. */
void gallery_more_scene(unsigned char *px, int w, int h, bool color);

/* The data grid of the Data grid page, its 2,000 rows and all, and the line under it. */
void gallery_more_people_grid(cui_ctx *ui);
const char *gallery_more_people_note(void);

/* The Files page: a browser over the disk, the two dialogs and a shell. */
void gallery_more_file_browser(cui_ctx *ui);
void gallery_more_file_pickers(cui_ctx *ui, bool *open_dlg, bool *save_dlg);
void gallery_more_terminal(cui_ctx *ui);
const char *gallery_more_chosen_path(void);

/* The heavy panels of the developer tools page: history, dependencies, network and trace. */
void gallery_more_commit_graph(cui_ctx *ui);
void gallery_more_graph_view(cui_ctx *ui);
void gallery_more_waterfall(cui_ctx *ui);
void gallery_more_trace(cui_ctx *ui);

/* The panels of the second developer-tools page whose data is built here. */
void gallery_more_blame(cui_ctx *ui);
void gallery_more_uptime(cui_ctx *ui);
void gallery_more_time_histogram(cui_ctx *ui);

/* The canvas page's picture card and file list. */
void gallery_more_result_card(cui_ctx *ui);
void gallery_more_canvas_files(cui_ctx *ui);

#endif
