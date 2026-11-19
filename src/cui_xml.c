/* CUI XML: the document. Reading the text, expanding what makes it a tree
 * (<Include>, <Repeat>, <ForEach>, <Widget>), computing {"..."}, and holding
 * what the interface needs between frames: the values, the bindings, the
 * actions and the file it follows.
 *
 * The tags themselves are in cui_xml_widgets.c: this file never draws.
 */
#include "cui_xml_internal.h"

#include <ctype.h>
#include <stdarg.h>
#include <sys/stat.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#  define cui__xml_stat _stat64
typedef struct _stat64 cui__xml_stat_t;
#else
#  define cui__xml_stat stat
typedef struct stat cui__xml_stat_t;
#endif

/* ------------------------------------------------------------------------ */
/* Small helpers                                                            */
/* ------------------------------------------------------------------------ */

void *cui__xml_alloc(size_t size) {
    void *p = malloc(size ? size : 1);
    return p;
}

static void *cui__xml_realloc(void *p, size_t size) {
    void *q = realloc(p, size ? size : 1);
    return q;
}

char *cui__xml_strdup(const char *s) {
    size_t n;
    char *d;
    if (!s) return NULL;
    n = strlen(s) + 1;
    d = cui__xml_alloc(n);
    if (d) memcpy(d, s, n);
    return d;
}

char *cui__xml_strndup(const char *s, size_t n) {
    char *d = cui__xml_alloc(n + 1);
    if (!d) return NULL;
    if (n) memcpy(d, s, n);
    d[n] = 0;
    return d;
}

static bool cui__xml_same(const char *a, const char *b) {
    if (!a || !b) return a == b;
    return strcmp(a, b) == 0;
}

/* ASCII case insensitive compare of a whole string. */
static bool cui__xml_same_ci(const char *a, const char *b) {
    if (!a || !b) return a == b;
    while (*a && *b) {
        if (cui__ascii_lower((unsigned char)*a) != cui__ascii_lower((unsigned char)*b)) return false;
        a++;
        b++;
    }
    return *a == *b;
}

static bool cui__xml_starts_ci(const char *s, const char *prefix) {
    while (*prefix) {
        if (!*s || cui__ascii_lower((unsigned char)*s) != cui__ascii_lower((unsigned char)*prefix))
            return false;
        s++;
        prefix++;
    }
    return true;
}

static char *cui__xml_trim(char *s) {
    char *end;
    while (*s && (unsigned char)*s <= ' ') s++;
    end = s + strlen(s);
    while (end > s && (unsigned char)end[-1] <= ' ') end--;
    *end = 0;
    return s;
}

/* Appends text to a NUL terminated buffer that lives in a realloc'd block. */
static bool cui__xml_append(char **buf, size_t *len, size_t *cap, const char *text, size_t n) {
    if (*len + n + 1 > *cap) {
        size_t ncap = *cap ? *cap : 32;
        char *nb;
        while (ncap < *len + n + 1) ncap *= 2;
        nb = cui__xml_realloc(*buf, ncap);
        if (!nb) return false;
        *buf = nb;
        *cap = ncap;
    }
    if (n) memcpy(*buf + *len, text, n);
    *len += n;
    (*buf)[*len] = 0;
    return true;
}

/* ------------------------------------------------------------------------ */
/* Mistakes                                                                 */
/* ------------------------------------------------------------------------ */

void cui__xml_error(cui_xml *doc, int line, int column, const char *fmt, ...) {
    char message[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(message, sizeof message, fmt, ap);
    va_end(ap);
    if (!doc) return;
    if (doc->ok) {
        doc->err_line = line;
        doc->err_col = column;
        snprintf(doc->last_message, sizeof doc->last_message, "%s", message);
    }
    doc->ok = false;
    snprintf(doc->last_error, sizeof doc->last_error, "%d:%d: %s", line, column, message);
    if (doc->error_fn) {
        cui_xml_error e;
        e.line = line;
        e.column = column;
        snprintf(e.message, sizeof e.message, "%s", message);
        doc->error_fn(doc->path ? doc->path : "", &e, doc->error_user);
    }
}

void cui__xml_warn(cui_xml *doc, const cui_xml_node *node, const char *fmt, ...) {
    char message[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(message, sizeof message, fmt, ap);
    va_end(ap);
    cui__xml_error(doc, node ? node->line : 0, node ? node->column : 0, "%s", message);
}

/* ------------------------------------------------------------------------ */
/* Nodes                                                                    */
/* ------------------------------------------------------------------------ */

static void cui__xml_free_vars(cui_xml_var *vars, int count) {
    int i;
    for (i = 0; i < count; i++) {
        free(vars[i].name);
        free(vars[i].value);
    }
    free(vars);
}

static void cui__xml_node_free(cui_xml_node *n) {
    int i;
    if (!n) return;
    free(n->tag);
    free(n->text);
    for (i = 0; i < n->attr_count; i++) {
        free(n->attrs[i].name);
        free(n->attrs[i].value);
    }
    free(n->attrs);
    for (i = 0; i < n->kid_count; i++) cui__xml_node_free(n->kids[i]);
    free(n->kids);
    for (i = 0; i < n->item_count; i++) free(n->items[i]);
    free(n->items);
    free(n->nums);
    if (n->cache && n->cache_free) n->cache_free(n->cache);
    if (n->cache2 && n->cache2_free) n->cache2_free(n->cache2);
    cui__xml_free_vars(n->vars, n->var_count);
    if (n->key_owned) free(n->key);
    free(n->name);
    free(n->buf);
    free(n);
}

static cui_xml_node *cui__xml_node_new(void) {
    cui_xml_node *n = cui__xml_alloc(sizeof *n);
    if (!n) return NULL;
    memset(n, 0, sizeof *n);
    n->shown = true;
    n->enabled = true;
    n->reported = -1;
    n->tag_id = CUI__XML_TAG_UNKNOWN;
    return n;
}

static void cui__xml_node_add_kid(cui_xml_node *parent, cui_xml_node *kid) {
    if (!parent || !kid) return;
    if (parent->kid_count == parent->kid_cap) {
        int ncap = parent->kid_cap ? parent->kid_cap * 2 : 4;
        cui_xml_node **nk = cui__xml_realloc(parent->kids, sizeof *nk * (size_t)ncap);
        if (!nk) return;
        parent->kids = nk;
        parent->kid_cap = ncap;
    }
    kid->parent = parent;
    parent->kids[parent->kid_count++] = kid;
}

static void cui__xml_node_insert_kid(cui_xml_node *parent, cui_xml_node *kid, int at) {
    int i;
    if (!parent || !kid) return;
    if (parent->kid_count == parent->kid_cap) {
        int ncap = parent->kid_cap ? parent->kid_cap * 2 : 4;
        cui_xml_node **nk = cui__xml_realloc(parent->kids, sizeof *nk * (size_t)ncap);
        if (!nk) return;
        parent->kids = nk;
        parent->kid_cap = ncap;
    }
    if (at < 0 || at > parent->kid_count) at = parent->kid_count;
    for (i = parent->kid_count; i > at; i--) parent->kids[i] = parent->kids[i - 1];
    parent->kids[at] = kid;
    kid->parent = parent;
    parent->kid_count++;
}

static void cui__xml_node_remove_kid(cui_xml_node *parent, int index) {
    int i;
    if (!parent || index < 0 || index >= parent->kid_count) return;
    for (i = index; i + 1 < parent->kid_count; i++) parent->kids[i] = parent->kids[i + 1];
    parent->kids[i] = NULL;
    parent->kid_count--;
}

const char *cui__xml_attr_raw(const cui_xml_node *node, const char *name) {
    int i;
    if (!node || !name) return NULL;
    for (i = 0; i < node->attr_count; i++)
        if (cui__xml_same_ci(node->attrs[i].name, name)) return node->attrs[i].value;
    return NULL;
}

static int cui__xml_attr_index(const cui_xml_node *node, const char *name) {
    int i;
    if (!node || !name) return -1;
    for (i = 0; i < node->attr_count; i++)
        if (cui__xml_same_ci(node->attrs[i].name, name)) return i;
    return -1;
}

/* Sets an attribute, keeping "as written" so the tools read what the file says. */
static bool cui__xml_attr_put(cui_xml_node *node, const char *name, const char *value) {
    int i;
    char *copy;
    if (!node || !name || !*name) return false;
    i = cui__xml_attr_index(node, name);
    if (!value) {
        if (i < 0) return true;
        free(node->attrs[i].name);
        free(node->attrs[i].value);
        for (; i + 1 < node->attr_count; i++) node->attrs[i] = node->attrs[i + 1];
        node->attr_count--;
        return true;
    }
    copy = cui__xml_strdup(value);
    if (!copy) return false;
    if (i >= 0) {
        free(node->attrs[i].value);
        node->attrs[i].value = copy;
        return true;
    }
    if (node->attr_count == node->attr_cap) {
        int ncap = node->attr_cap ? node->attr_cap * 2 : 4;
        cui_xml_attr *na = cui__xml_realloc(node->attrs, sizeof *na * (size_t)ncap);
        if (!na) {
            free(copy);
            return false;
        }
        node->attrs = na;
        node->attr_cap = ncap;
    }
    node->attrs[node->attr_count].name = cui__xml_strdup(name);
    node->attrs[node->attr_count].value = copy;
    node->attr_count++;
    return true;
}

/* Drops what was parsed from the attributes; set_attr calls it. */
static void cui__xml_node_drop_caches(cui_xml_node *n) {
    int i;
    for (i = 0; i < n->item_count; i++) free(n->items[i]);
    free(n->items);
    n->items = NULL;
    n->item_count = 0;
    n->item_cap = 0;
    free(n->nums);
    n->nums = NULL;
    n->num_count = 0;
    n->num_cap = 0;
    n->nums_parsed = false;
    if (n->cache && n->cache_free) n->cache_free(n->cache);
    n->cache = NULL;
    n->cache_free = NULL;
    if (n->cache2 && n->cache2_free) n->cache2_free(n->cache2);
    n->cache2 = NULL;
    n->cache2_free = NULL;
}

static cui_xml_node *cui__xml_node_clone(const cui_xml_node *src) {
    cui_xml_node *n;
    int i;
    if (!src) return NULL;
    n = cui__xml_node_new();
    if (!n) return NULL;
    n->tag = cui__xml_strdup(src->tag);
    n->tag_id = src->tag_id;
    n->text = cui__xml_strdup(src->text);
    n->comment = src->comment;
    n->line = src->line;
    n->column = src->column;
    n->shown = src->shown;
    n->enabled = src->enabled;
    n->b = src->b;
    n->i = src->i;
    n->f = src->f;
    n->d = src->d;
    for (i = 0; i < src->attr_count; i++)
        cui__xml_attr_put(n, src->attrs[i].name, src->attrs[i].value);
    for (i = 0; i < src->var_count; i++) {
        /* scope variables are copied by the expansion, not here */
    }
    for (i = 0; i < src->kid_count; i++) cui__xml_node_add_kid(n, cui__xml_node_clone(src->kids[i]));
    return n;
}

/* ------------------------------------------------------------------------ */
/* Reading the text                                                         */
/* ------------------------------------------------------------------------ */

typedef struct cui__xml_reader {
    cui_xml *doc;
    const char *s;
    size_t size, pos;
    int line, col;
} cui__xml_reader;

static int cui__xml_peek(cui__xml_reader *r) {
    return r->pos < r->size ? (unsigned char)r->s[r->pos] : -1;
}

static int cui__xml_next(cui__xml_reader *r) {
    int c;
    if (r->pos >= r->size) return -1;
    c = (unsigned char)r->s[r->pos++];
    if (c == '\n') {
        r->line++;
        r->col = 1;
    } else {
        r->col++;
    }
    return c;
}

static bool cui__xml_match(cui__xml_reader *r, const char *text) {
    size_t n = strlen(text);
    size_t i;
    if (r->pos + n > r->size) return false;
    for (i = 0; i < n; i++)
        if (r->s[r->pos + i] != text[i]) return false;
    for (i = 0; i < n; i++) cui__xml_next(r);
    return true;
}

static void cui__xml_skip_space(cui__xml_reader *r) {
    while (cui__xml_peek(r) >= 0 && (unsigned char)cui__xml_peek(r) <= ' ') cui__xml_next(r);
}

/* Reads up to (not including) any of the terminators; the terminator stays. */
static char *cui__xml_read_until(cui__xml_reader *r, const char *terminators) {
    size_t start = r->pos, len = 0, cap = 0;
    char *out = NULL;
    while (r->pos < r->size) {
        int c = cui__xml_peek(r);
        if (c >= 0 && strchr(terminators, c)) break;
        if (!cui__xml_append(&out, &len, &cap, r->s + r->pos, 1)) break;
        cui__xml_next(r);
    }
    if (!out) out = cui__xml_strndup(r->s + start, 0);
    return out;
}

/* Reads up to a terminator and steps over it; reads to the end when the
 * terminator is missing. */
static char *cui__xml_read_until_str(cui__xml_reader *r, const char *terminator) {
    size_t len = 0, cap = 0;
    size_t n = strlen(terminator);
    char *out = NULL;
    while (r->pos < r->size) {
        if (r->pos + n <= r->size && !strncmp(r->s + r->pos, terminator, n)) {
            size_t i;
            for (i = 0; i < n; i++) cui__xml_next(r);
            break;
        }
        if (!cui__xml_append(&out, &len, &cap, r->s + r->pos, 1)) break;
        cui__xml_next(r);
    }
    if (!out) out = cui__xml_strndup("", 0);
    return out;
}

/* XML turns CRLF and a lone CR into LF as it reads, so a document written or
 * checked out on Windows reads exactly as it would anywhere else. It matters for
 * text a document carries rather than lays out: code, JSON, markdown. */
static void cui__xml_normalize_breaks(char *s) {
    char *w = s;
    for (; *s; s++) {
        if (*s == '\r') {
            *w++ = '\n';
            if (s[1] == '\n') s++;
        } else {
            *w++ = *s;
        }
    }
    *w = 0;
}

static void cui__xml_decode_entities(char *s) {
    char *src = s, *dst = s;
    while (*src) {
        if (*src != '&') {
            *dst++ = *src++;
            continue;
        }
        if (!strncmp(src, "&amp;", 5)) { *dst++ = '&'; src += 5; continue; }
        if (!strncmp(src, "&lt;", 4)) { *dst++ = '<'; src += 4; continue; }
        if (!strncmp(src, "&gt;", 4)) { *dst++ = '>'; src += 4; continue; }
        if (!strncmp(src, "&quot;", 6)) { *dst++ = '"'; src += 6; continue; }
        if (!strncmp(src, "&apos;", 6)) { *dst++ = '\''; src += 6; continue; }
        if (!strncmp(src, "&nbsp;", 6)) { *dst++ = ' '; src += 6; continue; }
        if (src[1] == '#') {
            unsigned long cp = 0;
            const char *p = src + 2;
            int base = 10;
            if (*p == 'x' || *p == 'X') { base = 16; p++; }
            while ((base == 16 && isxdigit((unsigned char)*p)) ||
                   (base == 10 && isdigit((unsigned char)*p))) {
                cp = cp * (unsigned long)base + (unsigned long)(base == 16 ? (isdigit((unsigned char)*p) ? *p - '0' : (cui__ascii_lower((unsigned char)*p) - 'a' + 10)) : *p - '0');
                p++;
            }
            if (*p == ';' && cp) {
                char enc[4];
                int n = cui__utf8_encode((uint32_t)cp, enc);
                int i;
                for (i = 0; i < n; i++) *dst++ = enc[i];
                src = (char *)p + 1;
                continue;
            }
        }
        *dst++ = *src++;
    }
    *dst = 0;
}

static bool cui__xml_valid_name_char(int c) {
    return isalnum(c) || c == '_' || c == '-' || c == '.' || c == ':' || c == '{' || c == '}';
}

static char *cui__xml_read_name(cui__xml_reader *r) {
    size_t len = 0, cap = 0;
    char *out = NULL;
    while (cui__xml_peek(r) >= 0 && cui__xml_valid_name_char(cui__xml_peek(r))) {
        if (!cui__xml_append(&out, &len, &cap, r->s + r->pos, 1)) break;
        cui__xml_next(r);
    }
    if (!out) out = cui__xml_strndup("", 0);
    return out;
}

/* Parses one element (the reader is on its '<'). Returns NULL on a mistake. */
static cui_xml_node *cui__xml_parse_element(cui__xml_reader *r) {
    cui_xml_node *node;
    int line = r->line, col = r->col;
    bool self_closing = false;
    cui__xml_next(r);   /* '<' */
    node = cui__xml_node_new();
    if (!node) return NULL;
    node->line = line;
    node->column = col;
    node->tag = cui__xml_read_name(r);
    if (!node->tag || !*node->tag) {
        cui__xml_error(r->doc, line, col, "an element has no name");
        cui__xml_node_free(node);
        return NULL;
    }
    for (;;) {
        int c;
        cui__xml_skip_space(r);
        c = cui__xml_peek(r);
        if (c < 0) {
            cui__xml_error(r->doc, line, col, "<%s> is not closed", node->tag);
            cui__xml_node_free(node);
            return NULL;
        }
        if (c == '/') {
            cui__xml_next(r);
            cui__xml_skip_space(r);
            if (cui__xml_peek(r) == '>') {
                cui__xml_next(r);
                self_closing = true;
                break;
            }
            cui__xml_error(r->doc, r->line, r->col, "expected '>' after '/' in <%s>", node->tag);
            cui__xml_node_free(node);
            return NULL;
        }
        if (c == '>') {
            cui__xml_next(r);
            break;
        }
        {
            char *name = cui__xml_read_name(r);
            char *value = NULL;
            if (!name || !*name) {
                free(name);
                cui__xml_error(r->doc, r->line, r->col, "expected an attribute in <%s>", node->tag);
                cui__xml_node_free(node);
                return NULL;
            }
            cui__xml_skip_space(r);
            if (cui__xml_peek(r) != '=') {
                /* an attribute with no value: written as "" */
                cui__xml_attr_put(node, name, "");
                free(name);
                continue;
            }
            cui__xml_next(r);
            cui__xml_skip_space(r);
            {
                int quote = cui__xml_peek(r);
                if (quote != '"' && quote != '\'') {
                    cui__xml_error(r->doc, r->line, r->col, "the value of %s in <%s> is not quoted",
                                   name, node->tag);
                    free(name);
                    cui__xml_node_free(node);
                    return NULL;
                }
                cui__xml_next(r);
                value = cui__xml_read_until(r, quote == '"' ? "\"" : "\'");
                cui__xml_next(r);
            }
            if (value) {
                cui__xml_decode_entities(value);
                cui__xml_normalize_breaks(value);
            }
            cui__xml_attr_put(node, name, value ? value : "");
            free(name);
            free(value);
        }
    }
    node->tag_id = cui__xml_tag_lookup(node->tag);

    if (!self_closing) {
        size_t text_len = 0, text_cap = 0;
        for (;;) {
            int c = cui__xml_peek(r);
            if (c < 0) {
                cui__xml_error(r->doc, line, col, "<%s> is not closed", node->tag);
                break;
            }
            if (c == '<') {
                if (r->pos + 1 < r->size && r->s[r->pos + 1] == '!') {
                    int cline = r->line, ccol = r->col;
                    if (r->pos + 3 < r->size && !strncmp(r->s + r->pos, "<!--", 4)) {
                        cui_xml_node *comment;
                        char *text;
                        cui__xml_match(r, "<!--");
                        text = cui__xml_read_until_str(r, "-->");
                        comment = cui__xml_node_new();
                        if (comment) {
                            comment->comment = true;
                            comment->line = cline;
                            comment->column = ccol;
                            comment->tag = cui__xml_strdup("#comment");
                            comment->text = text ? text : cui__xml_strdup("");
                            cui__xml_node_add_kid(node, comment);
                        } else {
                            free(text);
                        }
                        continue;
                    }
                    if (r->pos + 8 < r->size && !strncmp(r->s + r->pos, "<![CDATA[", 9)) {
                        char *text;
                        cui__xml_match(r, "<![CDATA[");
                        text = cui__xml_read_until_str(r, "]]>");
                        if (text) {
                            cui__xml_normalize_breaks(text);
                            cui__xml_append(&node->text, &text_len, &text_cap, text, strlen(text));
                            free(text);
                        }
                        continue;
                    }
                    /* <!DOCTYPE ...> */
                    while (cui__xml_peek(r) >= 0 && cui__xml_peek(r) != '>') cui__xml_next(r);
                    if (cui__xml_peek(r) == '>') cui__xml_next(r);
                    continue;
                }
                if (r->pos + 1 < r->size && r->s[r->pos + 1] == '?') {
                    if (!cui__xml_match(r, "<?")) break;
                    while (cui__xml_peek(r) >= 0) {
                        if (cui__xml_match(r, "?>")) break;
                        cui__xml_next(r);
                    }
                    continue;
                }
                if (r->pos + 1 < r->size && r->s[r->pos + 1] == '/') {
                    char *name;
                    int cline = r->line, ccol = r->col;
                    cui__xml_match(r, "</");
                    name = cui__xml_read_name(r);
                    cui__xml_skip_space(r);
                    if (cui__xml_peek(r) == '>') cui__xml_next(r);
                    if (!cui__xml_same_ci(name, node->tag))
                        cui__xml_error(r->doc, cline, ccol, "</%s> closes <%s>", name ? name : "", node->tag);
                    free(name);
                    break;
                }
                {
                    cui_xml_node *kid = cui__xml_parse_element(r);
                    if (!kid) break;
                    cui__xml_node_add_kid(node, kid);
                }
                continue;
            }
            {
                char *chunk = cui__xml_read_until(r, "<");
                if (chunk) {
                    char *trimmed;
                    cui__xml_decode_entities(chunk);
                    cui__xml_normalize_breaks(chunk);
                    trimmed = cui__xml_trim(chunk);
                    if (*trimmed) {
                        if (text_len) cui__xml_append(&node->text, &text_len, &text_cap, " ", 1);
                        cui__xml_append(&node->text, &text_len, &text_cap, trimmed, strlen(trimmed));
                    }
                    free(chunk);
                }
            }
        }
    }
    if (!node->text) node->text = NULL;
    return node;
}

/* ------------------------------------------------------------------------ */
/* {"..."}: values and expressions                                          */
/* ------------------------------------------------------------------------ */

typedef struct cui__xml_value {
    int kind;      /* 0 number, 1 string */
    double num;
    const char *str;
} cui__xml_value;

typedef struct cui__xml_expr {
    cui_xml *doc;
    cui_xml_node *node;
    const cui_xml_var *scope;
    int scope_count;
    const char *s;
    int pos;
    bool failed;
    const char *message;
} cui__xml_expr;

static const char *cui__xml_var_get(const cui_xml_var *vars, int count, const char *name, size_t len) {
    int i;
    for (i = 0; i < count; i++) {
        if (strlen(vars[i].name) == len && !strncmp(vars[i].name, name, len)) return vars[i].value;
    }
    return NULL;
}

static bool cui__xml_parse_number_text(const char *s, double *out) {
    char *end;
    double v;
    if (!s || !*s) return false;
    v = strtod(s, &end);
    if (end == s) return false;
    while (*end == ' ') end++;
    if (*end) return false;
    *out = v;
    return true;
}

/* Resolves one identifier: the element's scope, then the document's
 * variables, then its bindings. */
static bool cui__xml_lookup(cui__xml_expr *e, const char *name, size_t len, cui__xml_value *out) {
    const char *v;
    char key[128];
    int i;
    if (len >= sizeof key) return false;
    memcpy(key, name, len);
    key[len] = 0;
    if (e->scope_count) {
        v = cui__xml_var_get(e->scope, e->scope_count, key, len);
        if (v) {
            double num;
            if (cui__xml_parse_number_text(v, &num)) {
                out->kind = 0;
                out->num = num;
            } else {
                out->kind = 1;
                out->str = v;
            }
            return true;
        }
    }
    if (e->node) {
        v = cui__xml_var_get(e->node->vars, e->node->var_count, key, len);
        if (v) {
            double num;
            if (cui__xml_parse_number_text(v, &num)) {
                out->kind = 0;
                out->num = num;
            } else {
                out->kind = 1;
                out->str = v;
            }
            return true;
        }
    }
    if (e->doc) {
        v = cui__xml_var_get(e->doc->vars, e->doc->var_count, key, len);
        if (v) {
            double num;
            if (cui__xml_parse_number_text(v, &num)) {
                out->kind = 0;
                out->num = num;
            } else {
                out->kind = 1;
                out->str = v;
            }
            return true;
        }
        for (i = 0; i < e->doc->bind_count; i++) {
            cui_xml_bind *b = &e->doc->binds[i];
            if (!cui__xml_same(b->name, key)) continue;
            out->kind = b->kind == CUI__XML_BIND_TEXT ? 1 : 0;
            switch (b->kind) {
            case CUI__XML_BIND_BOOL: out->num = *(bool *)b->ptr ? 1 : 0; break;
            case CUI__XML_BIND_INT: out->num = (double)*(int *)b->ptr; break;
            case CUI__XML_BIND_FLOAT: out->num = (double)*(float *)b->ptr; break;
            case CUI__XML_BIND_DOUBLE: out->num = *(double *)b->ptr; break;
            case CUI__XML_BIND_TEXT: out->str = (const char *)b->ptr; break;
            default: return false;
            }
            return true;
        }
    }
    if (cui__xml_same_ci(key, "true")) {
        out->kind = 0;
        out->num = 1;
        return true;
    }
    if (cui__xml_same_ci(key, "false") || cui__xml_same_ci(key, "none")) {
        out->kind = 0;
        out->num = 0;
        return true;
    }
    return false;
}

static void cui__xml_expr_space(cui__xml_expr *e) {
    while (e->s[e->pos] == ' ' || e->s[e->pos] == '\t' || e->s[e->pos] == '\n') e->pos++;
}

static bool cui__xml_parse_clause(cui__xml_expr *e, cui__xml_value *out);

static bool cui__xml_truth(cui__xml_value v) {
    return v.kind == 0 ? v.num != 0 : (v.str && *v.str);
}

static bool cui__xml_parse_call(cui__xml_expr *e, const char *name, cui__xml_value *out) {
    cui__xml_value a[3];
    int argc = 0;
    e->pos++;   /* '(' */
    for (;;) {
        cui__xml_expr_space(e);
        if (e->s[e->pos] == ')') {
            e->pos++;
            break;
        }
        if (argc >= 3) {
            e->failed = true;
            e->message = "too many arguments";
            return false;
        }
        if (!cui__xml_parse_clause(e, &a[argc])) return false;
        argc++;
        cui__xml_expr_space(e);
        if (e->s[e->pos] == ',') {
            e->pos++;
            continue;
        }
        if (e->s[e->pos] == ')') {
            e->pos++;
            break;
        }
        e->failed = true;
        e->message = "expected , or )";
        return false;
    }
    out->kind = 0;
    out->num = 0;
    if (!strcmp(name, "min") && argc == 2) {
        out->num = a[0].num < a[1].num ? a[0].num : a[1].num;
        return true;
    }
    if (!strcmp(name, "max") && argc == 2) {
        out->num = a[0].num > a[1].num ? a[0].num : a[1].num;
        return true;
    }
    if (!strcmp(name, "clamp") && argc == 3) {
        double v = a[0].num;
        if (v < a[1].num) v = a[1].num;
        if (v > a[2].num) v = a[2].num;
        out->num = v;
        return true;
    }
    if (!strcmp(name, "round") && argc == 1) { out->num = (double)(long long)(a[0].num + (a[0].num < 0 ? -0.5 : 0.5)); return true; }
    if (!strcmp(name, "floor") && argc == 1) { out->num = floor(a[0].num); return true; }
    if (!strcmp(name, "ceil") && argc == 1) { out->num = ceil(a[0].num); return true; }
    if (!strcmp(name, "abs") && argc == 1) { out->num = a[0].num < 0 ? -a[0].num : a[0].num; return true; }
    if (!strcmp(name, "len") && argc == 1) {
        out->num = a[0].kind == 1 ? (double)strlen(a[0].str) : 0;
        return true;
    }
    if (!strcmp(name, "empty") && argc == 1) {
        out->num = a[0].kind == 1 ? (*a[0].str == 0) : (a[0].num == 0);
        return true;
    }
    if (!strcmp(name, "default") && argc == 2) {
        if (a[0].kind == 1 ? (*a[0].str == 0) : (a[0].num == 0)) *out = a[1];
        else *out = a[0];
        return true;
    }
    if ((!strcmp(name, "upper") || !strcmp(name, "lower")) && argc == 1) {
        char *buf;
        size_t n, i;
        const char *src = a[0].kind == 1 ? a[0].str : "";
        n = strlen(src);
        buf = cui__xml_scratch_alloc(e->doc, n + 1);
        if (!buf) {
            e->failed = true;
            e->message = "out of memory";
            return false;
        }
        for (i = 0; i < n; i++) {
            char c = src[i];
            buf[i] = name[0] == 'u' ? (char)toupper((unsigned char)c) : (char)tolower((unsigned char)c);
        }
        buf[n] = 0;
        out->kind = 1;
        out->str = buf;
        return true;
    }
    e->failed = true;
    e->message = "unknown function";
    return false;
}

static bool cui__xml_parse_primary(cui__xml_expr *e, cui__xml_value *out) {
    char c;
    cui__xml_expr_space(e);
    c = e->s[e->pos];
    if (c == '(') {
        e->pos++;
        if (!cui__xml_parse_clause(e, out)) return false;
        cui__xml_expr_space(e);
        if (e->s[e->pos] == ')') e->pos++;
        else {
            e->failed = true;
            e->message = "expected )";
            return false;
        }
        return true;
    }
    if (c == '\'' || c == '"') {
        char quote = c;
        size_t start;
        e->pos++;
        start = (size_t)e->pos;
        while (e->s[e->pos] && e->s[e->pos] != quote) e->pos++;
        if (e->s[e->pos] != quote) {
            e->failed = true;
            e->message = "unterminated text";
            return false;
        }
        out->kind = 1;
        out->str = cui__xml_strndup(e->s + start, (size_t)e->pos - start);
        e->pos++;
        return true;
    }
    if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)e->s[e->pos + 1])) ||
        (c == '-' && (isdigit((unsigned char)e->s[e->pos + 1]) || e->s[e->pos + 1] == '.'))) {
        char *end;
        out->kind = 0;
        out->num = strtod(e->s + e->pos, &end);
        if (end == e->s + e->pos) {
            e->failed = true;
            e->message = "bad number";
            return false;
        }
        e->pos = (int)(end - e->s);
        return true;
    }
    if (isalpha((unsigned char)c) || c == '_') {
        size_t start = (size_t)e->pos;
        char name[64];
        size_t len;
        while (isalnum((unsigned char)e->s[e->pos]) || e->s[e->pos] == '_' || e->s[e->pos] == '.') e->pos++;
        len = (size_t)e->pos - start;
        if (len >= sizeof name) {
            e->failed = true;
            e->message = "name too long";
            return false;
        }
        memcpy(name, e->s + start, len);
        name[len] = 0;
        if (e->s[e->pos] == '(') return cui__xml_parse_call(e, name, out);
        if (!cui__xml_lookup(e, name, len, out)) {
            e->failed = true;
            e->message = "unknown name";
            return false;
        }
        return true;
    }
    e->failed = true;
    e->message = "unexpected character";
    return false;
}

static bool cui__xml_parse_unary(cui__xml_expr *e, cui__xml_value *out) {
    cui__xml_expr_space(e);
    if (e->s[e->pos] == '!') {
        cui__xml_value v;
        e->pos++;
        if (!cui__xml_parse_unary(e, &v)) return false;
        out->kind = 0;
        out->num = cui__xml_truth(v) ? 0 : 1;
        return true;
    }
    if (e->s[e->pos] == '-' && !isdigit((unsigned char)e->s[e->pos + 1])) {
        cui__xml_value v;
        e->pos++;
        if (!cui__xml_parse_unary(e, &v)) return false;
        out->kind = 0;
        out->num = -v.num;
        return true;
    }
    return cui__xml_parse_primary(e, out);
}

static bool cui__xml_parse_mul(cui__xml_expr *e, cui__xml_value *out) {
    cui__xml_value rhs;
    if (!cui__xml_parse_unary(e, out)) return false;
    for (;;) {
        char c;
        cui__xml_expr_space(e);
        c = e->s[e->pos];
        if (c != '*' && c != '/' && c != '%') return true;
        e->pos++;
        if (!cui__xml_parse_unary(e, &rhs)) return false;
        if (c == '*') out->num = out->num * rhs.num;
        else if (c == '/') out->num = rhs.num != 0 ? out->num / rhs.num : 0;
        else out->num = rhs.num != 0 ? fmod(out->num, rhs.num) : 0;
        out->kind = 0;
    }
}

static bool cui__xml_parse_add(cui__xml_expr *e, cui__xml_value *out) {
    cui__xml_value rhs;
    if (!cui__xml_parse_mul(e, out)) return false;
    for (;;) {
        char c;
        cui__xml_expr_space(e);
        c = e->s[e->pos];
        if (c != '+' && c != '-') return true;
        e->pos++;
        if (!cui__xml_parse_mul(e, &rhs)) return false;
        if (out->kind == 1 || rhs.kind == 1) {
            /* text + text joins them */
            const char *a = out->kind == 1 ? out->str : "";
            const char *b = rhs.kind == 1 ? rhs.str : "";
            size_t na = strlen(a), nb = strlen(b);
            char *buf = cui__xml_scratch_alloc(e->doc, na + nb + 1);
            if (!buf) {
                e->failed = true;
                e->message = "out of memory";
                return false;
            }
            memcpy(buf, a, na);
            memcpy(buf + na, b, nb);
            buf[na + nb] = 0;
            out->kind = 1;
            out->str = buf;
        } else {
            out->num = c == '+' ? out->num + rhs.num : out->num - rhs.num;
        }
    }
}

static bool cui__xml_parse_rel(cui__xml_expr *e, cui__xml_value *out) {
    if (!cui__xml_parse_add(e, out)) return false;
    for (;;) {
        cui__xml_value rhs;
        int op = 0;
        double a, b;
        cui__xml_expr_space(e);
        if (e->s[e->pos] == '<' && e->s[e->pos + 1] == '=') { op = 1; e->pos += 2; }
        else if (e->s[e->pos] == '>' && e->s[e->pos + 1] == '=') { op = 2; e->pos += 2; }
        else if (e->s[e->pos] == '<') { op = 3; e->pos++; }
        else if (e->s[e->pos] == '>') { op = 4; e->pos++; }
        else return true;
        if (!cui__xml_parse_add(e, &rhs)) return false;
        if (out->kind == 1 || rhs.kind == 1) {
            a = (double)(out->kind == 1 && out->str ? strlen(out->str) : out->num);
            b = (double)(rhs.kind == 1 && rhs.str ? strlen(rhs.str) : rhs.num);
        } else {
            a = out->num;
            b = rhs.num;
        }
        out->kind = 0;
        switch (op) {
        case 1: out->num = a <= b; break;
        case 2: out->num = a >= b; break;
        case 3: out->num = a < b; break;
        default: out->num = a > b; break;
        }
    }
}

static bool cui__xml_parse_eq(cui__xml_expr *e, cui__xml_value *out) {
    if (!cui__xml_parse_rel(e, out)) return false;
    for (;;) {
        cui__xml_value rhs;
        bool equal;
        int negate = 0;
        cui__xml_expr_space(e);
        if (e->s[e->pos] == '=' && e->s[e->pos + 1] == '=') e->pos += 2;
        else if (e->s[e->pos] == '!' && e->s[e->pos + 1] == '=') { negate = 1; e->pos += 2; }
        else return true;
        if (!cui__xml_parse_rel(e, &rhs)) return false;
        if (out->kind == 1 || rhs.kind == 1)
            equal = cui__xml_same(out->kind == 1 ? out->str : "", rhs.kind == 1 ? rhs.str : "");
        else
            equal = out->num == rhs.num;
        out->kind = 0;
        out->num = (equal != (negate != 0)) ? 1 : 0;
    }
}

static bool cui__xml_parse_and(cui__xml_expr *e, cui__xml_value *out) {
    if (!cui__xml_parse_eq(e, out)) return false;
    for (;;) {
        cui__xml_value rhs;
        cui__xml_expr_space(e);
        if (!(e->s[e->pos] == '&' && e->s[e->pos + 1] == '&')) return true;
        e->pos += 2;
        if (!cui__xml_parse_eq(e, &rhs)) return false;
        out->kind = 0;
        out->num = (cui__xml_truth(*out) && cui__xml_truth(rhs)) ? 1 : 0;
    }
}

static bool cui__xml_parse_or(cui__xml_expr *e, cui__xml_value *out) {
    if (!cui__xml_parse_and(e, out)) return false;
    for (;;) {
        cui__xml_value rhs;
        cui__xml_expr_space(e);
        if (!(e->s[e->pos] == '|' && e->s[e->pos + 1] == '|')) return true;
        e->pos += 2;
        if (!cui__xml_parse_and(e, &rhs)) return false;
        out->kind = 0;
        out->num = (cui__xml_truth(*out) || cui__xml_truth(rhs)) ? 1 : 0;
    }
}

static bool cui__xml_parse_ternary(cui__xml_expr *e, cui__xml_value *out) {
    if (!cui__xml_parse_or(e, out)) return false;
    cui__xml_expr_space(e);
    if (e->s[e->pos] == '?') {
        cui__xml_value yes, no;
        bool truth = cui__xml_truth(*out);
        e->pos++;
        if (!cui__xml_parse_ternary(e, &yes)) return false;
        cui__xml_expr_space(e);
        if (e->s[e->pos] != ':') {
            e->failed = true;
            e->message = "expected :";
            return false;
        }
        e->pos++;
        if (!cui__xml_parse_ternary(e, &no)) return false;
        *out = truth ? yes : no;
    }
    return true;
}

static bool cui__xml_parse_clause(cui__xml_expr *e, cui__xml_value *out) {
    return cui__xml_parse_ternary(e, out);
}

static const char *cui__xml_value_text(cui_xml *doc, cui__xml_value v) {
    char *buf;
    if (v.kind == 1) return v.str ? v.str : "";
    buf = cui__xml_scratch_alloc(doc, 48);
    if (!buf) return "";
    if (v.num == (double)(long long)v.num && v.num < 1e15 && v.num > -1e15)
        snprintf(buf, 48, "%lld", (long long)v.num);
    else
        snprintf(buf, 48, "%g", v.num);
    return buf;
}

/* Evaluates a text: returns true and writes the value when it is a complete,
 * valid expression. */
static bool cui__xml_eval(cui_xml *doc, cui_xml_node *node, const cui_xml_var *scope, int scope_count,
                          const char *text, cui__xml_value *out) {
    cui__xml_expr e;
    memset(&e, 0, sizeof e);
    e.doc = doc;
    e.node = node;
    e.scope = scope;
    e.scope_count = scope_count;
    e.s = text;
    out->kind = 0;
    out->num = 0;
    out->str = "";
    if (!cui__xml_parse_clause(&e, out)) return false;
    cui__xml_expr_space(&e);
    if (e.s[e.pos]) return false;
    return true;
}

/* Computes {"..."} in a text. Without a brace the text is returned as it is;
 * otherwise the result lives in the document's frame scratch. A brace that
 * cannot be computed stays as written, so a document that needs a name the
 * app has not set still loads. */
static const char *cui__xml_substitute(cui_xml *doc, cui_xml_node *node,
                                      const cui_xml_var *scope, int scope_count, const char *src) {
    size_t len = 0, cap = 0;
    char *out = NULL;
    int i = 0;
    if (!src || !strchr(src, '{')) return src;
    while (src[i]) {
        if (src[i] == '{' && src[i + 1] == '{') {
            cui__xml_append(&out, &len, &cap, "{", 1);
            i += 2;
            continue;
        }
        if (src[i] == '{') {
            int depth = 1;
            int start = i + 1;
            int j = i + 1;
            char *inner;
            cui__xml_value v;
            while (src[j] && depth > 0) {
                if (src[j] == '{') depth++;
                else if (src[j] == '}') depth--;
                if (depth > 0) j++;
            }
            if (depth != 0) {
                /* unterminated: keep it */
                cui__xml_append(&out, &len, &cap, src + i, 1);
                i++;
                continue;
            }
            inner = cui__xml_strndup(src + start, (size_t)j - start);
            if (inner && cui__xml_eval(doc, node, scope, scope_count, inner, &v)) {
                const char *text = cui__xml_value_text(doc, v);
                cui__xml_append(&out, &len, &cap, text, strlen(text));
            } else {
                cui__xml_append(&out, &len, &cap, src + i, (size_t)j - i + 1);
            }
            free(inner);
            i = j + 1;
            continue;
        }
        cui__xml_append(&out, &len, &cap, src + i, 1);
        i++;
    }
    if (!out) return "";
    return out;
}

/* Evaluates an attribute as a condition: If="Wide &amp;&amp; count &gt; 2". */
bool cui__xml_test(cui_xml *doc, cui_xml_node *node, const char *expression, bool fallback) {
    cui__xml_value v;
    const char *text;
    if (!expression || !*expression) return fallback;
    text = cui__xml_substitute(doc, node, NULL, 0, expression);
    if (!cui__xml_eval(doc, node, NULL, 0, text ? text : expression, &v)) {
        if (cui__xml_same_ci(expression, "false") || cui__xml_same_ci(expression, "no") ||
            cui__xml_same_ci(expression, "0"))
            return false;
        if (cui__xml_same_ci(expression, "true") || cui__xml_same_ci(expression, "yes") ||
            cui__xml_same_ci(expression, "1"))
            return true;
        return fallback;
    }
    return cui__xml_truth(v);
}

/* ------------------------------------------------------------------------ */
/* Frame scratch                                                            */
/* ------------------------------------------------------------------------ */

char *cui__xml_scratch_alloc(cui_xml *doc, size_t size) {
    int slot;
    if (!doc) return NULL;
    if (size < 64) size = 64;
    slot = doc->scratch_pos % CUI__XML_SCRATCH;
    doc->scratch_pos++;
    if (doc->scratch_cap[slot] < size) {
        char *p = cui__xml_realloc(doc->scratch[slot], size);
        if (!p) return NULL;
        doc->scratch[slot] = p;
        doc->scratch_cap[slot] = size;
    }
    doc->scratch[slot][0] = 0;
    return doc->scratch[slot];
}

/* ------------------------------------------------------------------------ */
/* Attribute readers                                                        */
/* ------------------------------------------------------------------------ */

const char *cui__xml_attr_text(cui_xml *doc, cui_xml_node *node, const char *name, const char *fallback) {
    const char *raw = cui__xml_attr_raw(node, name);
    if (!raw) return fallback;
    return cui__xml_substitute(doc, node, NULL, 0, raw);
}

const char *cui__xml_node_text_computed(cui_xml *doc, cui_xml_node *node) {
    if (!node || !node->text) return "";
    return cui__xml_substitute(doc, node, NULL, 0, node->text);
}

static float cui__xml_parse_float(const char *s, float fallback) {
    char *end;
    double v;
    if (!s || !*s) return fallback;
    v = strtod(s, &end);
    if (end == s) return fallback;
    return (float)v;
}

float cui__xml_attr_num(cui_xml *doc, cui_xml_node *node, const char *name, float fallback) {
    return cui__xml_parse_float(cui__xml_attr_text(doc, node, name, NULL), fallback);
}

bool cui__xml_attr_bool(cui_xml *doc, cui_xml_node *node, const char *name, bool fallback) {
    const char *s = cui__xml_attr_text(doc, node, name, NULL);
    if (!s || !*s) return fallback;
    if (cui__xml_same_ci(s, "true") || cui__xml_same_ci(s, "yes") || cui__xml_same_ci(s, "on") ||
        cui__xml_same_ci(s, "1"))
        return true;
    if (cui__xml_same_ci(s, "false") || cui__xml_same_ci(s, "no") || cui__xml_same_ci(s, "off") ||
        cui__xml_same_ci(s, "0") || cui__xml_same_ci(s, "none"))
        return false;
    {
        double v;
        if (cui__xml_parse_number_text(s, &v)) return v != 0;
    }
    return fallback;
}

static cui_size cui__xml_parse_size(const char *s, cui_size fallback) {
    float v;
    char *end;
    if (!s || !*s) return fallback;
    while (*s == ' ') s++;
    if (cui__xml_same_ci(s, "fill") || cui__xml_same_ci(s, "grow")) return CUI_FILL;
    if (cui__xml_same_ci(s, "fit") || cui__xml_same_ci(s, "auto")) return CUI_FIT;
    v = (float)strtod(s, &end);
    if (end == s) return fallback;
    while (*end == ' ') end++;
    if (*end == '%') return CUI_PCT(v);
    if (*end == '*') return CUI_GROW(v);
    return CUI_PX(v);
}

cui_size cui__xml_attr_size(cui_xml *doc, cui_xml_node *node, const char *name, cui_size fallback) {
    return cui__xml_parse_size(cui__xml_attr_text(doc, node, name, NULL), fallback);
}

/* The color names a document can write, with the token each one asks for. */
static const struct {
    const char *name;
    int token;          /* 0: a plain color, see below */
    uint32_t rgba;
} cui__xml_colors[] = {
    { "transparent", 0, 0x00000000u },
    { "white", 0, 0xFFFFFFFFu }, { "black", 0, 0xFF000000u },
    { "red", 0, 0xFFE5484Du }, { "green", 0, 0xFF30A46Cu }, { "blue", 0, 0xFF3E63DDu },
    { "yellow", 0xFFFFFFFFu, 0 },   /* replaced below */
    { "orange", 0, 0xFFFF8A3Du }, { "gray", 0, 0xFF8B8F98u }, { "grey", 0, 0xFF8B8F98u },
    { "cyan", 0, 0xFF00A2C7u }, { "magenta", 0, 0xFFD6409Fu }, { "pink", 0, 0xFFE93D82u },
    { "purple", 0, 0xFF8E4EC6u },
    { "background", CUI_TOKEN_BACKGROUND, 0 }, { "surface", CUI_TOKEN_SURFACE, 0 },
    { "surfaceraised", CUI_TOKEN_SURFACE_RAISED, 0 }, { "surfacemuted", CUI_TOKEN_SURFACE_MUTED, 0 },
    { "text", CUI_TOKEN_FOREGROUND, 0 }, { "foreground", CUI_TOKEN_FOREGROUND, 0 },
    { "textsecondary", CUI_TOKEN_TEXT_SECONDARY, 0 }, { "textmuted", CUI_TOKEN_TEXT_MUTED, 0 },
    { "border", CUI_TOKEN_BORDER, 0 }, { "bordersubtle", CUI_TOKEN_BORDER_SUBTLE, 0 },
    { "borderstrong", CUI_TOKEN_BORDER_STRONG, 0 },
    { "accent", CUI_TOKEN_ACCENT, 0 }, { "accentstrong", CUI_TOKEN_ACCENT_STRONG, 0 },
    { "accentsubtle", CUI_TOKEN_ACCENT_SUBTLE, 0 },
    { "accentforeground", CUI_TOKEN_ACCENT_FOREGROUND, 0 },
    { "success", CUI_TOKEN_SUCCESS, 0 }, { "warning", CUI_TOKEN_WARNING, 0 },
    { "danger", CUI_TOKEN_DANGER, 0 }, { "info", CUI_TOKEN_INFO, 0 },
    { "hover", CUI_TOKEN_HOVER, 0 }, { "scrim", CUI_TOKEN_SCRIM, 0 },
    { "focusring", CUI_TOKEN_FOCUS_RING, 0 },
    { "series1", CUI_TOKEN_SERIES_1, 0 }, { "series2", CUI_TOKEN_SERIES_2, 0 },
    { "series3", CUI_TOKEN_SERIES_3, 0 }, { "series4", CUI_TOKEN_SERIES_4, 0 },
    { "series5", CUI_TOKEN_SERIES_5, 0 }, { "series6", CUI_TOKEN_SERIES_6, 0 }
};

static bool cui__xml_parse_color(const char *s, cui_color *out) {
    int i;
    if (!s || !*s) return false;
    if (s[0] == '#') {
        size_t n = strlen(s + 1);
        unsigned int v = 0;
        size_t k;
        for (k = 0; k < n; k++) {
            int c = (unsigned char)s[1 + k];
            int d;
            if (isdigit(c)) d = c - '0';
            else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
            else return false;
            v = (v << 4) | (unsigned int)d;
        }
        if (n == 3) {
            unsigned int r = (v >> 8) & 0xF, g = (v >> 4) & 0xF, b = v & 0xF;
            out->r = (uint8_t)(r * 17);
            out->g = (uint8_t)(g * 17);
            out->b = (uint8_t)(b * 17);
            out->a = 255;
            return true;
        }
        if (n == 4) {
            unsigned int r = (v >> 12) & 0xF, g = (v >> 8) & 0xF, b = (v >> 4) & 0xF, a = v & 0xF;
            out->r = (uint8_t)(r * 17);
            out->g = (uint8_t)(g * 17);
            out->b = (uint8_t)(b * 17);
            out->a = (uint8_t)(a * 17);
            return true;
        }
        if (n == 6) {
            out->r = (uint8_t)((v >> 16) & 0xFF);
            out->g = (uint8_t)((v >> 8) & 0xFF);
            out->b = (uint8_t)(v & 0xFF);
            out->a = 255;
            return true;
        }
        if (n == 8) {
            out->r = (uint8_t)((v >> 24) & 0xFF);
            out->g = (uint8_t)((v >> 16) & 0xFF);
            out->b = (uint8_t)((v >> 8) & 0xFF);
            out->a = (uint8_t)(v & 0xFF);
            return true;
        }
        return false;
    }
    for (i = 0; i < CUI_COUNTOF(cui__xml_colors); i++) {
        if (!cui__xml_same_ci(s, cui__xml_colors[i].name)) continue;
        if (cui__xml_same_ci(s, "yellow")) {
            out->r = 0xF5;
            out->g = 0xD9;
            out->b = 0x0A;
            out->a = 255;
            return true;
        }
        if (cui__xml_colors[i].token) {
            /* A theme token is written as the token color is: the component
             * resolves it against the theme when it draws, so a document
             * follows light and dark. */
            out->r = (uint8_t)cui__xml_colors[i].token;
            out->g = 0xC5;
            out->b = 0x1D;
            out->a = 0;
            return true;
        }
        out->r = (uint8_t)((cui__xml_colors[i].rgba >> 24) & 0xFF);
        out->g = (uint8_t)((cui__xml_colors[i].rgba >> 16) & 0xFF);
        out->b = (uint8_t)((cui__xml_colors[i].rgba >> 8) & 0xFF);
        out->a = (uint8_t)(cui__xml_colors[i].rgba & 0xFF);
        return true;
    }
    return false;
}

cui_color cui__xml_attr_color(cui_xml *doc, cui_xml_node *node, const char *name, cui_color fallback) {
    const char *s = cui__xml_attr_text(doc, node, name, NULL);
    cui_color c;
    if (!s || !*s) return fallback;
    if (cui__xml_parse_color(s, &c)) return c;
    cui__xml_warn(doc, node, "%s=\"%s\" is not a color", name, s);
    return fallback;
}

int cui__xml_attr_choice(cui_xml *doc, cui_xml_node *node, const char *name,
                         const char *const *names, int count, int fallback) {
    const char *s = cui__xml_attr_text(doc, node, name, NULL);
    int i;
    if (!s || !*s) return fallback;
    for (i = 0; i < count; i++)
        if (names[i] && cui__xml_same_ci(s, names[i])) return i;
    return fallback;
}

int cui__xml_attr_items(cui_xml *doc, cui_xml_node *node, const char ***out) {
    const char *items;
    if (!node) return 0;
    if (!node->item_count) {
        items = cui__xml_attr_raw(node, "items");
        if (items && *items) {
            const char *p = items;
            while (*p) {
                const char *bar = strchr(p, '|');
                size_t n = bar ? (size_t)(bar - p) : strlen(p);
                char *entry = cui__xml_strndup(p, n);
                if (entry) {
                    char *t = cui__xml_trim(entry);
                    if (node->item_count == node->item_cap) {
                        int ncap = node->item_cap ? node->item_cap * 2 : 4;
                        char **ni = cui__xml_realloc(node->items, sizeof *ni * (size_t)ncap);
                        if (!ni) {
                            free(entry);
                            break;
                        }
                        node->items = ni;
                        node->item_cap = ncap;
                    }
                    node->items[node->item_count++] = cui__xml_strdup(t);
                    free(entry);
                }
                if (!bar) break;
                p = bar + 1;
            }
        } else {
            int i;
            for (i = 0; i < node->kid_count; i++) {
                cui_xml_node *kid = node->kids[i];
                const char *text;
                if (kid->comment) continue;
                if (kid->tag_id != CUI__XML_TAG_ITEM && kid->tag_id != CUI__XML_TAG_TAB &&
                    kid->tag_id != CUI__XML_TAG_SLICE && kid->tag_id != CUI__XML_TAG_PART &&
                    kid->tag_id != CUI__XML_TAG_SERIES)
                    continue;
                text = cui__xml_attr_raw(kid, "text");
                if (!text) text = kid->text;
                if (!text) continue;
                if (node->item_count == node->item_cap) {
                    int ncap = node->item_cap ? node->item_cap * 2 : 4;
                    char **ni = cui__xml_realloc(node->items, sizeof *ni * (size_t)ncap);
                    if (!ni) break;
                    node->items = ni;
                    node->item_cap = ncap;
                }
                node->items[node->item_count++] = cui__xml_strdup(text);
            }
        }
    }
    if (out) *out = (const char **)node->items;
    (void)doc;
    return node->item_count;
}

int cui__xml_attr_numbers(cui_xml *doc, cui_xml_node *node, const char *name, const float **out) {
    const char *s;
    if (!node) return 0;
    if (!node->nums_parsed) {
        node->nums_parsed = true;
        s = cui__xml_attr_text(doc, node, name, NULL);
        while (s && *s) {
            char *end;
            double v = strtod(s, &end);
            if (end == s) break;
            if (node->num_count == node->num_cap) {
                int ncap = node->num_cap ? node->num_cap * 2 : 8;
                float *nn = cui__xml_realloc(node->nums, sizeof *nn * (size_t)ncap);
                if (!nn) break;
                node->nums = nn;
                node->num_cap = ncap;
            }
            node->nums[node->num_count++] = (float)v;
            s = end;
            while (*s == ',' || *s == ' ' || *s == ';') s++;
        }
    }
    if (out) *out = node->nums;
    (void)doc;
    return node->num_count;
}

/* ------------------------------------------------------------------------ */
/* What the controls hold                                                   */
/* ------------------------------------------------------------------------ */

static cui_xml_bind *cui__xml_bind_find(cui_xml *doc, const char *name) {
    int i;
    if (!doc || !name || !*name) return NULL;
    for (i = 0; i < doc->bind_count; i++)
        if (cui__xml_same(doc->binds[i].name, name)) return &doc->binds[i];
    return NULL;
}

static cui_xml_bind *cui__xml_bind_for(cui_xml *doc, cui_xml_node *node) {
    const char *bind;
    if (!doc || !node) return NULL;
    bind = cui__xml_attr_raw(node, "bind");
    if (bind && *bind) {
        cui_xml_bind *b = cui__xml_bind_find(doc, bind);
        if (b) return b;
    }
    return cui__xml_bind_find(doc, node->key);
}

bool *cui__xml_slot_bool(cui_xml *doc, cui_xml_node *node) {
    cui_xml_bind *b = cui__xml_bind_for(doc, node);
    if (b && b->kind == CUI__XML_BIND_BOOL && b->ptr) return (bool *)b->ptr;
    return &node->b;
}

int *cui__xml_slot_int(cui_xml *doc, cui_xml_node *node) {
    cui_xml_bind *b = cui__xml_bind_for(doc, node);
    if (b && b->kind == CUI__XML_BIND_INT && b->ptr) return (int *)b->ptr;
    return &node->i;
}

float *cui__xml_slot_float(cui_xml *doc, cui_xml_node *node) {
    cui_xml_bind *b = cui__xml_bind_for(doc, node);
    if (b && b->kind == CUI__XML_BIND_FLOAT && b->ptr) return (float *)b->ptr;
    return &node->f;
}

double *cui__xml_slot_double(cui_xml *doc, cui_xml_node *node) {
    cui_xml_bind *b = cui__xml_bind_for(doc, node);
    if (b && b->kind == CUI__XML_BIND_DOUBLE && b->ptr) return (double *)b->ptr;
    return &node->d;
}

char *cui__xml_slot_text(cui_xml *doc, cui_xml_node *node, size_t *cap, size_t want) {
    cui_xml_bind *b = cui__xml_bind_for(doc, node);
    if (b && b->kind == CUI__XML_BIND_TEXT && b->ptr && b->cap) {
        *cap = b->cap;
        return (char *)b->ptr;
    }
    if (want < 32) want = 32;
    if (!node->buf || node->buf_cap < want) {
        size_t ncap = node->buf_cap ? node->buf_cap : 256;
        char *nb;
        while (ncap < want) ncap *= 2;
        nb = cui__xml_realloc(node->buf, ncap);
        if (!nb) {
            *cap = node->buf_cap;
            return node->buf;
        }
        if (!node->buf) nb[0] = 0;
        else nb[node->buf_cap - 1] = 0;
        node->buf = nb;
        node->buf_cap = ncap;
    }
    *cap = node->buf_cap;
    return node->buf;
}

/* ------------------------------------------------------------------------ */
/* Actions                                                                  */
/* ------------------------------------------------------------------------ */

static bool cui__xml_builtin(cui_xml *doc, cui_xml_node *node, const char *action) {
    const char *arg;
    size_t prefix;
    if (!doc->builtins || !action) return false;
    if (!strcmp(action, "app.quit")) {
        doc->quit = true;
        return true;
    }
    if (cui__xml_same_ci(action, "theme.dark") || cui__xml_same_ci(action, "theme.light") ||
        cui__xml_same_ci(action, "theme.toggle")) {
        bool dark;
        if (cui__xml_same_ci(action, "theme.dark")) dark = true;
        else if (cui__xml_same_ci(action, "theme.light")) dark = false;
        else dark = !(doc->has_theme && doc->theme == CUI_THEME_DARK);
        doc->theme = dark ? CUI_THEME_DARK : CUI_THEME_LIGHT;
        doc->has_theme = true;
        doc->prepared = false;
        return true;
    }
    if (cui__xml_starts_ci(action, "accent:")) {
        static const char *const names[] = { "neutral", "violet", "blue", "green", "amber", "orange", "coral", "rose" };
        int i;
        arg = action + 7;
        for (i = 0; i < 8; i++) {
            if (cui__xml_same_ci(arg, names[i])) {
                doc->accent = (cui_accent)i;
                doc->has_accent = true;
                doc->prepared = false;
                return true;
            }
        }
        return false;
    }
    prefix = 0;
    if (cui__xml_starts_ci(action, "show:")) prefix = 5;
    else if (cui__xml_starts_ci(action, "hide:")) prefix = 5;
    else if (cui__xml_starts_ci(action, "toggle:")) prefix = 7;
    if (prefix) {
        const char *name = action + prefix;
        cui_xml_node *target = cui_xml_find(doc, name);
        bool shown;
        if (!target) return false;
        shown = cui__xml_starts_ci(action, "show:") ? true
              : cui__xml_starts_ci(action, "hide:") ? false : !target->shown;
        target->shown = shown;
        /* an overlay's value is "closed": the switch moves it too */
        if (target->tag_id == CUI__XML_TAG_DIALOG || target->tag_id == CUI__XML_TAG_ALERT ||
            target->tag_id == CUI__XML_TAG_BANNER)
            *cui__xml_slot_bool(doc, target) = shown;
        return true;
    }
    if (cui__xml_starts_ci(action, "enable:") || cui__xml_starts_ci(action, "disable:")) {
        cui_xml_node *target = cui_xml_find(doc, action + 7);
        if (!target) return false;
        target->enabled = cui__xml_starts_ci(action, "enable:");
        return true;
    }
    if (cui__xml_starts_ci(action, "page:")) {
        /* page:NAME=2, page:NAME=+1, page:NAME=-1 */
        const char *eq = strchr(action + 5, '=');
        char name[96];
        size_t len;
        int value;
        cui_xml_node *target;
        if (!eq) return false;
        len = (size_t)(eq - (action + 5));
        if (len >= sizeof name) return false;
        memcpy(name, action + 5, len);
        name[len] = 0;
        target = cui_xml_find(doc, name);
        if (!target) return false;
        value = atoi(eq + 1);
        if (eq[1] == '+' || eq[1] == '-') target->i += value;
        else target->i = value;
        target->changed = true;
        (void)node;
        return true;
    }
    if (cui__xml_starts_ci(action, "copy:")) {
        doc->pending_copy = cui__xml_strdup(action + 5);
        return true;
    }
    if (cui__xml_starts_ci(action, "popover:")) {
        free(doc->pending_popover);
        doc->pending_popover = cui__xml_strdup(action + 8);
        return true;
    }
    return false;
}

void cui__xml_fire_name(cui_xml *doc, cui_xml_node *node, const char *action) {
    int i;
    if (!doc || !action || !*action) return;
    for (i = 0; i < doc->action_count; i++) {
        if (cui__xml_same(doc->actions[i].name, action)) {
            if (doc->actions[i].fn) doc->actions[i].fn(doc, node, action, doc->actions[i].user);
            return;
        }
    }
    if (cui__xml_builtin(doc, node, action)) return;
    if (doc->any_fn) doc->any_fn(doc, node, action, doc->any_user);
}

void cui__xml_fire(cui_xml *doc, cui_xml_node *node, const char *attr) {
    const char *action = cui__xml_attr_raw(node, attr);
    if (action && *action) cui__xml_fire_name(doc, node, action);
}

void cui__xml_changed_hook(cui_xml *doc, cui_xml_node *node) {
    node->changed = true;
    cui__xml_fire(doc, node, "onchanged");
}

/* ------------------------------------------------------------------------ */
/* Expanding the text into the tree that is drawn                           */
/* ------------------------------------------------------------------------ */

static cui_xml_widget *cui__xml_widget_find(cui_xml *doc, const char *name) {
    int i;
    if (!doc || !name) return NULL;
    for (i = 0; i < doc->widget_count; i++)
        if (cui__xml_same_ci(doc->widgets[i].name, name)) return &doc->widgets[i];
    return NULL;
}

static void cui__xml_widget_free(cui_xml_widget *w) {
    int i;
    free(w->name);
    for (i = 0; i < w->param_count; i++) {
        free(w->params[i].name);
        free(w->params[i].value);
    }
    free(w->params);
    cui__xml_node_free(w->body);
    memset(w, 0, sizeof *w);
}

static void cui__xml_add_widget(cui_xml *doc, cui_xml_node *decl) {
    cui_xml_widget w;
    const char *name;
    int i;
    if (!decl || decl->tag_id != CUI__XML_TAG_WIDGET) return;
    name = cui__xml_attr_raw(decl, "name");
    if (!name || !*name) {
        cui__xml_warn(doc, decl, "<Widget> has no Name");
        return;
    }
    if (cui__xml_widget_find(doc, name)) return;   /* the first one wins */
    memset(&w, 0, sizeof w);
    w.name = cui__xml_strdup(name);
    for (i = 0; i < decl->attr_count; i++) {
        if (cui__xml_same_ci(decl->attrs[i].name, "name")) continue;
        if (w.param_count == w.param_cap) {
            int ncap = w.param_cap ? w.param_cap * 2 : 4;
            cui_xml_attr *np = cui__xml_realloc(w.params, sizeof *np * (size_t)ncap);
            if (!np) break;
            w.params = np;
            w.param_cap = ncap;
        }
        w.params[w.param_count].name = cui__xml_strdup(decl->attrs[i].name);
        w.params[w.param_count].value = cui__xml_strdup(decl->attrs[i].value);
        w.param_count++;
    }
    /* The body is one element; a comment or a second one is ignored. */
    for (i = 0; i < decl->kid_count; i++) {
        if (decl->kids[i]->comment) continue;
        w.body = cui__xml_node_clone(decl->kids[i]);
        break;
    }
    if (!w.body) {
        cui__xml_warn(doc, decl, "<Widget Name=\"%s\"> has no content", name);
        cui__xml_widget_free(&w);
        return;
    }
    if (doc->widget_count == doc->widget_cap) {
        int ncap = doc->widget_cap ? doc->widget_cap * 2 : 4;
        cui_xml_widget *nw = cui__xml_realloc(doc->widgets, sizeof *nw * (size_t)ncap);
        if (!nw) {
            cui__xml_widget_free(&w);
            return;
        }
        doc->widgets = nw;
        doc->widget_cap = ncap;
    }
    doc->widgets[doc->widget_count++] = w;
}

/* Moves the declarations of another document into this one (an <Include>, or
 * a file handed to cui_xml_register_widgets). */
static void cui__xml_take_widgets(cui_xml *doc, cui_xml *from) {
    int i;
    if (!doc || !from) return;
    for (i = 0; i < from->widget_count; i++) {
        if (cui__xml_widget_find(doc, from->widgets[i].name)) continue;
        if (doc->widget_count == doc->widget_cap) {
            int ncap = doc->widget_cap ? doc->widget_cap * 2 : 4;
            cui_xml_widget *nw = cui__xml_realloc(doc->widgets, sizeof *nw * (size_t)ncap);
            if (!nw) return;
            doc->widgets = nw;
            doc->widget_cap = ncap;
        }
        doc->widgets[doc->widget_count++] = from->widgets[i];
        memset(&from->widgets[i], 0, sizeof from->widgets[i]);
    }
}

/* Collects the <Widget> declarations of a tree and takes them out of it. */
static void cui__xml_collect_widgets(cui_xml *doc, cui_xml_node *node) {
    int i;
    if (!node) return;
    for (i = 0; i < node->kid_count; i++) {
        cui_xml_node *kid = node->kids[i];
        if (kid->tag_id == CUI__XML_TAG_WIDGET) {
            cui__xml_add_widget(doc, kid);
            cui__xml_node_remove_kid(node, i);
            cui__xml_node_free(kid);
            i--;
            continue;
        }
        cui__xml_collect_widgets(doc, kid);
    }
}

static void cui__xml_var_push(cui_xml_var **vars, int *count, int *cap, const char *name, const char *value) {
    int i;
    for (i = 0; i < *count; i++) {
        if (cui__xml_same((*vars)[i].name, name)) {
            free((*vars)[i].value);
            (*vars)[i].value = cui__xml_strdup(value ? value : "");
            return;
        }
    }
    if (*count == *cap) {
        int ncap = *cap ? *cap * 2 : 4;
        cui_xml_var *nv = cui__xml_realloc(*vars, sizeof *nv * (size_t)ncap);
        if (!nv) return;
        *vars = nv;
        *cap = ncap;
    }
    (*vars)[*count].name = cui__xml_strdup(name);
    (*vars)[*count].value = cui__xml_strdup(value ? value : "");
    (*count)++;
}

static void cui__xml_vars_free(cui_xml_var *vars, int count) {
    cui__xml_free_vars(vars, count);
}

/* A clone whose {"..."} were computed with `scope`. */
static cui_xml_node *cui__xml_clone_subst(cui_xml *doc, const cui_xml_node *src,
                                          const cui_xml_var *scope, int scope_count) {
    cui_xml_node *n;
    int i;
    if (!src) return NULL;
    n = cui__xml_node_new();
    if (!n) return NULL;
    n->tag = cui__xml_strdup(src->tag);
    n->tag_id = src->tag_id;
    n->comment = src->comment;
    n->line = src->line;
    n->column = src->column;
    n->shown = src->shown;
    n->enabled = src->enabled;
    n->b = src->b;
    n->i = src->i;
    n->f = src->f;
    n->d = src->d;
    if (src->text) {
        const char *computed = cui__xml_substitute(doc, NULL, scope, scope_count, src->text);
        n->text = cui__xml_strdup(computed ? computed : "");
    }
    for (i = 0; i < src->attr_count; i++) {
        const char *value = cui__xml_substitute(doc, NULL, scope, scope_count, src->attrs[i].value);
        cui__xml_attr_put(n, src->attrs[i].name, value ? value : "");
    }
    /* The scope travels with the node, so a {"..."} left in it (one that
     * needs a binding) still sees the parameters at draw time. */
    for (i = 0; i < scope_count; i++) {
        int cap = n->var_cap;
        cui__xml_var_push(&n->vars, &n->var_count, &cap, scope[i].name, scope[i].value);
        n->var_cap = cap;
    }
    for (i = 0; i < src->kid_count; i++)
        cui__xml_node_add_kid(n, cui__xml_clone_subst(doc, src->kids[i], scope, scope_count));
    return n;
}

/* Reads a file next to the document, or as written. Returns a malloc'd path. */
static char *cui__xml_resolve_in(cui_xml *doc, const char *path) {
    char *joined;
    size_t n;
    if (!path || !*path) return NULL;
    if (path[0] == '/' || path[0] == '\\' || (isalpha((unsigned char)path[0]) && path[1] == ':'))
        return cui__xml_strdup(path);
    n = strlen(doc->dir ? doc->dir : "") + strlen(path) + 1;
    joined = cui__xml_alloc(n);
    if (!joined) return NULL;
    snprintf(joined, n, "%s%s", doc->dir ? doc->dir : "", path);
    return joined;
}

static bool cui__xml_file_exists(const char *path) {
    cui__xml_stat_t st;
    if (!path || !*path) return false;
    return cui__xml_stat(path, &st) == 0;
}

/* Instantiates a <Widget> declaration at the place one of its instances is. */
static cui_xml_node *cui__xml_instantiate(cui_xml *doc, cui_xml_node *use, cui_xml_widget *w) {
    cui_xml_var *scope = NULL;
    int scope_count = 0, scope_cap = 0;
    cui_xml_node *root;
    int i;
    /* The parameters: their defaults, then what the instance gives. */
    for (i = 0; i < w->param_count; i++)
        cui__xml_var_push(&scope, &scope_count, &scope_cap, w->params[i].name, w->params[i].value);
    for (i = 0; i < use->attr_count; i++) {
        int p;
        for (p = 0; p < w->param_count; p++) {
            if (cui__xml_same_ci(use->attrs[i].name, w->params[p].name)) {
                cui__xml_var_push(&scope, &scope_count, &scope_cap, w->params[p].name, use->attrs[i].value);
                break;
            }
        }
        if (p == w->param_count) {
        }
    }
    root = cui__xml_clone_subst(doc, w->body, scope, scope_count);
    if (!root) {
        cui__xml_vars_free(scope, scope_count);
        return NULL;
    }
    /* <Children/> takes the children given where the tag is used. */
    {
        int placed = 0;
        int k;
        for (k = 0; k < root->kid_count; k++) {
            cui_xml_node *kid = root->kids[k];
            const char *into;
            if (kid->tag_id != CUI__XML_TAG_CHILDREN) continue;
            into = cui__xml_attr_raw(kid, "into");
            {
                int c;
                for (c = 0; c < use->kid_count; c++) {
                    cui_xml_node *given = use->kids[c];
                    const char *mark;
                    if (given->comment) continue;
                    mark = cui__xml_attr_raw(given, "into");
                    if (into && *into) {
                        if (!mark || !cui__xml_same_ci(mark, into)) continue;
                    }
                    cui__xml_node_insert_kid(root, cui__xml_node_clone(given), k + 1 + placed);
                    placed++;
                }
            }
        }
        if (placed) {
            /* the placeholders go */
            for (k = root->kid_count - 1; k >= 0; k--) {
                if (root->kids[k]->tag_id == CUI__XML_TAG_CHILDREN) {
                    cui_xml_node *gone = root->kids[k];
                    cui__xml_node_remove_kid(root, k);
                    cui__xml_node_free(gone);
                }
            }
        } else {
            int k2;
            for (k2 = 0; k2 < use->kid_count; k2++)
                if (!use->kids[k2]->comment) cui__xml_node_add_kid(root, cui__xml_node_clone(use->kids[k2]));
        }
    }
    /* What the instance writes that is not a parameter goes on the root. */
    for (i = 0; i < use->attr_count; i++) {
        int p, is_param = 0;
        for (p = 0; p < w->param_count; p++)
            if (cui__xml_same_ci(use->attrs[i].name, w->params[p].name)) is_param = 1;
        if (!is_param) cui__xml_attr_put(root, use->attrs[i].name,
                                         cui__xml_substitute(doc, use, scope, scope_count, use->attrs[i].value));
    }
    cui__xml_vars_free(scope, scope_count);
    return root;
}

/* Expands what stands for other elements, in place under one parent:
 * <Include>, <Repeat>, <ForEach>, a tag a <Widget> declared, and a <Widget>
 * written where it is not wanted. Each replacement is visited again by the
 * same loop, so a template that includes a file that repeats a widget works.
 */
static void cui__xml_expand_in(cui_xml *doc, cui_xml_node *parent, int depth) {
    int i = 0;
    if (!parent) return;
    if (depth > CUI__XML_MAX_DEPTH) {
        cui__xml_warn(doc, parent, "the document nests too deeply (a <Widget> that uses itself?)");
        return;
    }
    while (i < parent->kid_count) {
        cui_xml_node *kid = parent->kids[i];
        if (kid->comment) {
            i++;
            continue;
        }
        if (kid->tag_id == CUI__XML_TAG_INCLUDE) {
            const char *source = cui__xml_attr_raw(kid, "source");
            char *path = cui__xml_resolve_in(doc, source);
            size_t size = 0;
            char *text = path ? cui__xml_read_file(path, &size) : NULL;
            int inserted = 0;
            if (text) {
                cui_xml *sub = cui_xml_parse(text, size, NULL);
                free(text);
                if (sub) {
                    cui__xml_take_widgets(doc, sub);
                    if (sub->root && sub->root->tag_id != CUI__XML_TAG_WIDGET) {
                        int k;
                        cui__xml_collect_widgets(doc, sub->root);
                        for (k = 0; k < sub->root->kid_count; k++) {
                            cui_xml_node *moved = sub->root->kids[k];
                            sub->root->kids[k] = NULL;
                            moved->parent = NULL;
                            cui__xml_node_insert_kid(parent, moved, i + inserted);
                            inserted++;
                        }
                    }
                    cui_xml_free(sub);
                }
            } else if (path) {
                cui__xml_warn(doc, kid, "Include could not read %s", path);
            }
            free(path);
            /* the content took its place: the <Include> is now after it */
            cui__xml_node_remove_kid(parent, i + inserted);
            cui__xml_node_free(kid);
            continue;
        }
        if (kid->tag_id == CUI__XML_TAG_WIDGET) {
            cui__xml_add_widget(doc, kid);
            cui__xml_node_remove_kid(parent, i);
            cui__xml_node_free(kid);
            continue;
        }
        if (kid->tag_id == CUI__XML_TAG_WIDGETS) {
            cui__xml_collect_widgets(doc, kid);
            cui__xml_node_remove_kid(parent, i);
            cui__xml_node_free(kid);
            continue;
        }
        if (kid->tag_id == CUI__XML_TAG_REPEAT || kid->tag_id == CUI__XML_TAG_FOREACH) {
            const char *var = cui__xml_attr_raw(kid, "var");
            cui_xml_var *scope = NULL;
            int scope_count = 0, scope_cap = 0;
            int rounds = 0;
            int kid_count = kid->kid_count;
            int r, k, placed = 0;
            if (kid->tag_id == CUI__XML_TAG_REPEAT) {
                rounds = (int)cui__xml_attr_num(doc, kid, "count", 0);
                if (!var || !*var) var = "i";
                if (rounds > 512) rounds = 512;
            } else {
                const char **entries = NULL;
                rounds = cui__xml_attr_items(doc, kid, &entries);
                if (!var || !*var) var = "item";
            }
            for (r = 0; r < rounds; r++) {
                if (kid->tag_id == CUI__XML_TAG_REPEAT) {
                    char value[24];
                    snprintf(value, sizeof value, "%d", r);
                    cui__xml_var_push(&scope, &scope_count, &scope_cap, var, value);
                } else {
                    const char **entries = NULL;
                    cui__xml_attr_items(doc, kid, &entries);
                    cui__xml_var_push(&scope, &scope_count, &scope_cap, var, entries[r]);
                }
                for (k = 0; k < kid_count; k++) {
                    cui_xml_node *copy;
                    if (kid->kids[k]->comment) continue;
                    copy = cui__xml_clone_subst(doc, kid->kids[k], scope, scope_count);
                    if (!copy) continue;
                    cui__xml_node_insert_kid(parent, copy, i + placed);
                    placed++;
                }
            }
            cui__xml_vars_free(scope, scope_count);
            cui__xml_node_remove_kid(parent, i + placed);
            cui__xml_node_free(kid);
            continue;
        }
        {
            cui_xml_widget *w = NULL;
            if (kid->tag_id == CUI__XML_TAG_UNKNOWN || kid->tag_id == CUI__XML_TAG_ITEM)
                w = cui__xml_widget_find(doc, kid->tag);
            if (w) {
                cui_xml_node *built = cui__xml_instantiate(doc, kid, w);
                if (built) {
                    cui__xml_node_insert_kid(parent, built, i);
                    cui__xml_node_remove_kid(parent, i + 1);
                    cui__xml_node_free(kid);
                    cui__xml_expand_in(doc, built, depth + 1);
                    i++;
                    continue;
                }
            }
        }
        cui__xml_expand_in(doc, kid, depth + 1);
        i++;
    }
}

static void cui__xml_expand(cui_xml *doc, cui_xml_node *root) {
    cui__xml_expand_in(doc, root, 0);
}
/* ------------------------------------------------------------------------ */
/* Keys: the identity of an element between frames and reloads              */
/* ------------------------------------------------------------------------ */

static void cui__xml_assign_keys(cui_xml *doc, cui_xml_node *node, const char *parent_key, int index) {
    int i;
    if (!node) return;
    free(node->name);
    node->name = NULL;
    if (node->key_owned) {
        free(node->key);
        node->key = NULL;
        node->key_owned = false;
    }
    {
        const char *name = cui__xml_attr_raw(node, "name");
        if (name && *name) {
            node->name = cui__xml_strdup(name);
            node->key = node->name;
            node->key_owned = false;
        } else {
            size_t n = strlen(parent_key) + strlen(node->tag ? node->tag : "") + 24;
            char *key = cui__xml_alloc(n);
            if (key) {
                snprintf(key, n, "%s/%d:%s", parent_key, index, node->tag ? node->tag : "");
                node->key = key;
                node->key_owned = true;
            }
        }
    }
    (void)doc;
    for (i = 0; i < node->kid_count; i++)
        cui__xml_assign_keys(doc, node->kids[i], node->key ? node->key : "", i);
}

/* ------------------------------------------------------------------------ */
/* Documents                                                                */
/* ------------------------------------------------------------------------ */

char *cui__xml_read_file(const char *path, size_t *size) {
    FILE *f;
    long len;
    char *buf;
    size_t got;
    if (!path) return NULL;
    f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    len = ftell(f);
    if (len < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    buf = cui__xml_alloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    got = fread(buf, 1, (size_t)len, f);
    fclose(f);
    buf[got] = 0;
    if (size) *size = got;
    return buf;
}

static void cui__xml_doc_reset(cui_xml *doc) {
    cui__xml_node_free(doc->root);
    doc->root = NULL;
    free(doc->title);
    doc->title = NULL;
    doc->width = 900;
    doc->height = 640;
    doc->has_theme = false;
    doc->has_accent = false;
}

/* A new document, empty and ready to be read. */
static cui_xml *cui__xml_doc_new(void) {
    cui_xml *doc = cui__xml_alloc(sizeof *doc);
    if (!doc) return NULL;
    memset(doc, 0, sizeof *doc);
    doc->path = cui__xml_strdup("");
    doc->dir = cui__xml_strdup("");
    doc->builtins = true;
    doc->ok = true;
    doc->watch = false;
    doc->width = 900;
    doc->height = 640;
    return doc;
}
static void cui__xml_free_widgets(cui_xml *doc) {
    int i;
    for (i = 0; i < doc->widget_count; i++) cui__xml_widget_free(&doc->widgets[i]);
    free(doc->widgets);
    doc->widgets = NULL;
    doc->widget_count = doc->widget_cap = 0;
}

static void cui__xml_read_window(cui_xml *doc) {
    cui_xml_node *root = doc->root;
    if (!root) return;
    doc->title = cui__xml_strdup(cui__xml_attr_raw(root, "title") ? cui__xml_attr_raw(root, "title") : "");
    doc->width = (int)cui__xml_attr_num(doc, root, "width", 900);
    doc->height = (int)cui__xml_attr_num(doc, root, "height", 640);
    if (doc->width < 120) doc->width = 120;
    if (doc->height < 80) doc->height = 80;
    {
        const char *theme = cui__xml_attr_raw(root, "theme");
        if (theme && *theme) {
            if (cui__xml_same_ci(theme, "dark")) {
                doc->theme = CUI_THEME_DARK;
                doc->has_theme = true;
            } else if (cui__xml_same_ci(theme, "light")) {
                doc->theme = CUI_THEME_LIGHT;
                doc->has_theme = true;
            }
        }
        {
            const char *accent = cui__xml_attr_raw(root, "accent");
            static const char *const names[] = { "neutral", "violet", "blue", "green", "amber", "orange", "coral", "rose" };
            int i;
            if (accent && *accent) {
                for (i = 0; i < 8; i++) {
                    if (cui__xml_same_ci(accent, names[i])) {
                        doc->accent = (cui_accent)i;
                        doc->has_accent = true;
                        break;
                    }
                }
            }
        }
    }
}

/* Builds the tree from the text: reading, declarations, expansion, keys. */
static bool cui__xml_build(cui_xml *doc, const char *text, size_t size, cui_xml_error *error) {
    cui__xml_reader r;
    char *copy;
    size_t n = size;
    memset(&r, 0, sizeof r);
    r.doc = doc;
    r.line = 1;
    r.col = 1;
    /* the reader needs a terminated buffer: only attributes are re-read from
     * it, and the parse stops at `size` anyway */
    copy = cui__xml_strndup(text ? text : "", size);
    if (!copy) return false;
    r.s = copy;
    r.size = n;
    r.line = 1;
    r.col = 1;
    if (n >= 3 && (unsigned char)copy[0] == 0xEF && (unsigned char)copy[1] == 0xBB &&
        (unsigned char)copy[2] == 0xBF) {
        r.pos = 3;
        r.col = 2;
    }
    /* What comes before the root: prolog, doctype, comments, whitespace. */
    for (;;) {
        cui__xml_skip_space(&r);
        if (cui__xml_peek(&r) < 0) break;
        if (cui__xml_peek(&r) != '<') {
            cui__xml_error(doc, r.line, r.col, "text before the root element");
            break;
        }
        if (r.pos + 1 < r.size && r.s[r.pos + 1] == '?') {
            if (!cui__xml_match(&r, "<?")) break;
            while (cui__xml_peek(&r) >= 0) {
                if (cui__xml_match(&r, "?>")) break;
                cui__xml_next(&r);
            }
            continue;
        }
        if (r.pos + 3 < r.size && !strncmp(r.s + r.pos, "<!--", 4)) {
            cui__xml_match(&r, "<!--");
            while (cui__xml_peek(&r) >= 0) {
                if (cui__xml_match(&r, "-->")) break;
                cui__xml_next(&r);
            }
            continue;
        }
        if (r.pos + 1 < r.size && r.s[r.pos + 1] == '!') {
            while (cui__xml_peek(&r) >= 0 && cui__xml_peek(&r) != '>') cui__xml_next(&r);
            if (cui__xml_peek(&r) == '>') cui__xml_next(&r);
            continue;
        }
        break;
    }
    doc->root = cui__xml_parse_element(&r);
    free(copy);
    if (!doc->root) {
        if (error) {
            memset(error, 0, sizeof *error);
            error->line = 1;
            error->column = 1;
            snprintf(error->message, sizeof error->message, "no element was read");
        }
        return false;
    }
    if (error && !doc->ok) {
        memset(error, 0, sizeof *error);
        error->line = doc->err_line;
        error->column = doc->err_col;
        snprintf(error->message, sizeof error->message, "%s",
                 doc->last_message[0] ? doc->last_message : "the text has a mistake");
    }
    if (doc->root->tag_id == CUI__XML_TAG_WIDGET) {
        /* the document is one declaration: what is left draws nothing */
        cui_xml_node *empty;
        cui__xml_add_widget(doc, doc->root);
        empty = cui__xml_node_new();
        if (empty) {
            empty->tag = cui__xml_strdup("Column");
            empty->tag_id = CUI__XML_TAG_COLUMN;
            cui__xml_node_free(doc->root);
            doc->root = empty;
        }
    } else {
        cui__xml_collect_widgets(doc, doc->root);
    }
    cui__xml_expand(doc, doc->root);
    cui__xml_assign_keys(doc, doc->root, "", 0);
    cui__xml_read_window(doc);
    return true;
}

cui_xml *cui_xml_parse(const char *text, size_t size, cui_xml_error *error) {
    cui_xml *doc = cui__xml_doc_new();
    if (!doc) return NULL;
    if (error) memset(error, 0, sizeof *error);
    if (!cui__xml_build(doc, text ? text : "", size ? size : (text ? strlen(text) : 0), error)) {
        if (!doc->root) {
            cui_xml_free(doc);
            return NULL;
        }
    }
    return doc;
}

/* Fills what <Window> says about the file, and starts following it. */
static void cui__xml_doc_set_path(cui_xml *doc, const char *path) {
    const char *slash;
    free(doc->path);
    free(doc->dir);
    doc->path = cui__xml_strdup(path ? path : "");
    slash = NULL;
    if (path) {
        const char *p;
        for (p = path; *p; p++)
            if (*p == '/' || *p == '\\') slash = p;
    }
    if (slash) {
        size_t n = (size_t)(slash - path) + 1;
        doc->dir = cui__xml_alloc(n + 1);
        if (doc->dir) {
            memcpy(doc->dir, path, n);
            doc->dir[n] = 0;
        }
    } else {
        doc->dir = cui__xml_strdup("");
    }
}

static bool cui__xml_file_stamp_of(const char *path, long long *size, long long *time) {
    cui__xml_stat_t st;
    if (!path || !*path) return false;
    if (cui__xml_stat(path, &st) != 0) return false;
    *size = (long long)st.st_size;
    *time = (long long)st.st_mtime;
    return true;
}

cui_xml *cui_xml_load(const char *path) {
    size_t size = 0;
    char *text = cui__xml_read_file(path, &size);
    cui_xml *doc = cui__xml_doc_new();
    if (!doc) {
        free(text);
        return NULL;
    }
    /* The folder is known before the text is read, so an <Include> or a Source
     * written in it is looked for next to the file. */
    cui__xml_doc_set_path(doc, path);
    doc->watch = true;
    doc->has_stamp = cui__xml_file_stamp_of(path, &doc->stamp[0], &doc->stamp[1]);
    if (!text) {
        doc->ok = false;
        doc->err_line = 1;
        doc->err_col = 1;
        snprintf(doc->last_message, sizeof doc->last_message, "cannot read the file");
        snprintf(doc->last_error, sizeof doc->last_error, "%s: 1:1: cannot read the file",
                 path ? path : "");
        if (doc->error_fn) {
            cui_xml_error e;
            e.line = 1;
            e.column = 1;
            snprintf(e.message, sizeof e.message, "cannot read the file");
            doc->error_fn(path ? path : "", &e, doc->error_user);
        }
        return doc;
    }
    cui__xml_build(doc, text, size, NULL);
    free(text);
    return doc;
}

void cui_xml_free(cui_xml *doc) {
    int i;
    if (!doc) return;
    cui__xml_doc_reset(doc);
    cui__xml_free_widgets(doc);
    for (i = 0; i < doc->bind_count; i++) free(doc->binds[i].name);
    free(doc->binds);
    for (i = 0; i < doc->var_count; i++) {
        free(doc->vars[i].name);
        free(doc->vars[i].value);
    }
    free(doc->vars);
    for (i = 0; i < doc->action_count; i++) free(doc->actions[i].name);
    free(doc->actions);
    for (i = 0; i < doc->tag_count; i++) free(doc->tags[i].name);
    free(doc->tags);
    for (i = 0; i < CUI__XML_SCRATCH; i++) free(doc->scratch[i]);
    free(doc->pending_copy);
    free(doc->pending_popover);
    free(doc->path);
    free(doc->dir);
    free(doc);
}

bool cui_xml_ok(const cui_xml *doc) { return doc && doc->ok; }

const char *cui_xml_last_error(const cui_xml *doc) { return doc ? doc->last_error : ""; }

void cui_xml_set_error_callback(cui_xml *doc, cui_xml_error_fn fn, void *user) {
    if (!doc) return;
    doc->error_fn = fn;
    doc->error_user = user;
}

const char *cui_xml_path(const cui_xml *doc) { return doc && doc->path ? doc->path : ""; }

const char *cui_xml_title(const cui_xml *doc) { return doc && doc->title ? doc->title : ""; }

void cui_xml_window_size(const cui_xml *doc, int *width, int *height) {
    if (width) *width = doc ? doc->width : 0;
    if (height) *height = doc ? doc->height : 0;
}

cui_theme_mode cui_xml_theme(const cui_xml *doc, cui_theme_mode fallback) {
    return doc && doc->has_theme ? doc->theme : fallback;
}

cui_app_desc cui_xml_app_desc(const cui_xml *doc) {
    cui_app_desc d;
    memset(&d, 0, sizeof d);
    d.title = cui_xml_title(doc);
    if (d.title[0] == 0) d.title = "CUI";
    d.width = doc ? doc->width : 900;
    d.height = doc ? doc->height : 640;
    d.theme = cui_xml_theme(doc, CUI_THEME_LIGHT);
    if (doc && doc->has_accent) d.accent = doc->accent;
    return d;
}

/* ------------------------------------------------------------------------ */
/* Drawing                                                                  */
/* ------------------------------------------------------------------------ */

static void cui__xml_apply_theme(cui_xml *doc, cui_ctx *ui) {
    if (doc->prepared) return;
    if (doc->has_theme) cui_set_theme(ui, doc->theme);
    if (doc->has_accent) cui_set_accent(ui, doc->accent);
    doc->prepared = true;
}

static void cui__xml_frame_begin(cui_xml *doc, cui_ctx *ui) {
    uint64_t frame = cui_frame_count(ui);
    if (doc->frame != frame) {
        doc->frame = frame;
        doc->scratch_pos = 0;
    }
    cui__xml_apply_theme(doc, ui);
    if (doc->pending_copy) {
        cui_set_clipboard_text(ui, doc->pending_copy);
        free(doc->pending_copy);
        doc->pending_copy = NULL;
    }
}

void cui_xml_render(cui_xml *doc, cui_ctx *ui) {
    if (!doc || !doc->root || !ui) return;
    cui__xml_frame_begin(doc, ui);
    doc->depth = 0;
    cui__xml_draw(doc, doc->root, ui, false);
}

bool cui_xml_render_into(cui_xml *doc, cui_ctx *ui, const char *name) {
    cui_xml_node *node;
    if (!doc || !ui || !name) return false;
    node = cui_xml_find(doc, name);
    if (!node) return false;
    cui__xml_frame_begin(doc, ui);
    doc->depth = 0;
    return cui__xml_draw(doc, node, ui, false);
}

/* ------------------------------------------------------------------------ */
/* Bindings                                                                 */
/* ------------------------------------------------------------------------ */

static bool cui__xml_bind_put(cui_xml *doc, const char *name, int kind, void *ptr, size_t cap) {
    cui_xml_bind *b;
    if (!doc || !name || !*name || !ptr) return false;
    b = cui__xml_bind_find(doc, name);
    if (!b) {
        if (doc->bind_count == doc->bind_cap) {
            int ncap = doc->bind_cap ? doc->bind_cap * 2 : 8;
            cui_xml_bind *nb = cui__xml_realloc(doc->binds, sizeof *nb * (size_t)ncap);
            if (!nb) return false;
            doc->binds = nb;
            doc->bind_cap = ncap;
        }
        b = &doc->binds[doc->bind_count++];
        memset(b, 0, sizeof *b);
        b->name = cui__xml_strdup(name);
    }
    b->kind = kind;
    b->ptr = ptr;
    b->cap = cap;
    return true;
}

bool cui_xml_bind_bool(cui_xml *doc, const char *name, bool *value) {
    return cui__xml_bind_put(doc, name, CUI__XML_BIND_BOOL, value, 0);
}
bool cui_xml_bind_int(cui_xml *doc, const char *name, int *value) {
    return cui__xml_bind_put(doc, name, CUI__XML_BIND_INT, value, 0);
}
bool cui_xml_bind_float(cui_xml *doc, const char *name, float *value) {
    return cui__xml_bind_put(doc, name, CUI__XML_BIND_FLOAT, value, 0);
}
bool cui_xml_bind_double(cui_xml *doc, const char *name, double *value) {
    return cui__xml_bind_put(doc, name, CUI__XML_BIND_DOUBLE, value, 0);
}
bool cui_xml_bind_text(cui_xml *doc, const char *name, char *buf, size_t cap) {
    return cui__xml_bind_put(doc, name, CUI__XML_BIND_TEXT, buf, cap);
}

bool cui_xml_unbind(cui_xml *doc, const char *name) {
    int i;
    if (!doc || !name) return false;
    for (i = 0; i < doc->bind_count; i++) {
        if (!cui__xml_same(doc->binds[i].name, name)) continue;
        free(doc->binds[i].name);
        for (; i + 1 < doc->bind_count; i++) doc->binds[i] = doc->binds[i + 1];
        doc->bind_count--;
        return true;
    }
    return false;
}

/* ------------------------------------------------------------------------ */
/* Reading and writing the values                                           */
/* ------------------------------------------------------------------------ */

static cui_xml_node *cui__xml_find_node(cui_xml_node *node, const char *name) {
    int i;
    if (!node) return NULL;
    if (node->key && cui__xml_same(node->key, name)) return node;
    for (i = 0; i < node->kid_count; i++) {
        cui_xml_node *found = cui__xml_find_node(node->kids[i], name);
        if (found) return found;
    }
    return NULL;
}

cui_xml_node *cui_xml_find(cui_xml *doc, const char *name) {
    if (!doc || !doc->root || !name || !*name) return NULL;
    if (cui__xml_same(doc->root->key, name)) return doc->root;
    return cui__xml_find_node(doc->root, name);
}

cui_xml_node *cui_xml_root(cui_xml *doc) { return doc ? doc->root : NULL; }

bool cui_xml_get_bool(cui_xml *doc, const char *name) {
    cui_xml_node *n = cui_xml_find(doc, name);
    return n ? *cui__xml_slot_bool(doc, n) : false;
}
int cui_xml_get_int(cui_xml *doc, const char *name) {
    cui_xml_node *n = cui_xml_find(doc, name);
    return n ? *cui__xml_slot_int(doc, n) : 0;
}
float cui_xml_get_float(cui_xml *doc, const char *name) {
    cui_xml_node *n = cui_xml_find(doc, name);
    return n ? *cui__xml_slot_float(doc, n) : 0;
}
double cui_xml_get_double(cui_xml *doc, const char *name) {
    cui_xml_node *n = cui_xml_find(doc, name);
    return n ? *cui__xml_slot_double(doc, n) : 0;
}
const char *cui_xml_get_text(cui_xml *doc, const char *name) {
    cui_xml_node *n = cui_xml_find(doc, name);
    size_t cap = 0;
    if (!n) return "";
    return cui__xml_slot_text(doc, n, &cap, 0);
}

void cui_xml_set_bool(cui_xml *doc, const char *name, bool value) {
    cui_xml_node *n = cui_xml_find(doc, name);
    if (n) *cui__xml_slot_bool(doc, n) = value;
}
void cui_xml_set_int(cui_xml *doc, const char *name, int value) {
    cui_xml_node *n = cui_xml_find(doc, name);
    if (n) *cui__xml_slot_int(doc, n) = value;
}
void cui_xml_set_float(cui_xml *doc, const char *name, float value) {
    cui_xml_node *n = cui_xml_find(doc, name);
    if (n) *cui__xml_slot_float(doc, n) = value;
}
void cui_xml_set_double(cui_xml *doc, const char *name, double value) {
    cui_xml_node *n = cui_xml_find(doc, name);
    if (n) *cui__xml_slot_double(doc, n) = value;
}
void cui_xml_set_index(cui_xml *doc, const char *name, int index) {
    cui_xml_node *n = cui_xml_find(doc, name);
    if (n) *cui__xml_slot_int(doc, n) = index;
}
void cui_xml_set_text(cui_xml *doc, const char *name, const char *text) {
    cui_xml_node *n = cui_xml_find(doc, name);
    size_t cap = 0;
    char *buf;
    if (!n) return;
    buf = cui__xml_slot_text(doc, n, &cap, text ? strlen(text) + 1 : 1);
    if (!buf || !cap) return;
    snprintf(buf, cap, "%s", text ? text : "");
}

bool cui_xml_changed(cui_xml *doc, const char *name) {
    cui_xml_node *n = cui_xml_find(doc, name);
    return n ? n->changed : false;
}

void cui_xml_set_var(cui_xml *doc, const char *name, const char *value) {
    int i;
    if (!doc || !name || !*name) return;
    for (i = 0; i < doc->var_count; i++) {
        if (!cui__xml_same(doc->vars[i].name, name)) continue;
        if (!value) {
            free(doc->vars[i].name);
            free(doc->vars[i].value);
            for (; i + 1 < doc->var_count; i++) doc->vars[i] = doc->vars[i + 1];
            doc->var_count--;
            return;
        }
        free(doc->vars[i].value);
        doc->vars[i].value = cui__xml_strdup(value);
        return;
    }
    if (!value) return;
    {
        int cap = doc->var_cap;
        cui__xml_var_push(&doc->vars, &doc->var_count, &cap, name, value);
        doc->var_cap = cap;
    }
}

const char *cui_xml_get_var(cui_xml *doc, const char *name) {
    int i;
    if (!doc || !name) return "";
    for (i = 0; i < doc->var_count; i++)
        if (cui__xml_same(doc->vars[i].name, name)) return doc->vars[i].value;
    return "";
}

/* ------------------------------------------------------------------------ */
/* Actions from C                                                           */
/* ------------------------------------------------------------------------ */

bool cui_xml_on(cui_xml *doc, const char *action, cui_xml_action_fn fn, void *user) {
    int i;
    if (!doc || !action || !*action) return false;
    for (i = 0; i < doc->action_count; i++) {
        if (!cui__xml_same(doc->actions[i].name, action)) continue;
        doc->actions[i].fn = fn;
        doc->actions[i].user = user;
        return true;
    }
    if (doc->action_count == doc->action_cap) {
        int ncap = doc->action_cap ? doc->action_cap * 2 : 8;
        cui_xml_action_entry *na = cui__xml_realloc(doc->actions, sizeof *na * (size_t)ncap);
        if (!na) return false;
        doc->actions = na;
        doc->action_cap = ncap;
    }
    doc->actions[doc->action_count].name = cui__xml_strdup(action);
    doc->actions[doc->action_count].fn = fn;
    doc->actions[doc->action_count].user = user;
    doc->action_count++;
    return true;
}

void cui_xml_off(cui_xml *doc, const char *action) {
    int i;
    if (!doc || !action) return;
    for (i = 0; i < doc->action_count; i++) {
        if (!cui__xml_same(doc->actions[i].name, action)) continue;
        free(doc->actions[i].name);
        for (; i + 1 < doc->action_count; i++) doc->actions[i] = doc->actions[i + 1];
        doc->action_count--;
        return;
    }
}

void cui_xml_on_any(cui_xml *doc, cui_xml_action_fn fn, void *user) {
    if (!doc) return;
    doc->any_fn = fn;
    doc->any_user = user;
}

void cui_xml_fire(cui_xml *doc, const char *action) {
    if (!doc || !action) return;
    cui__xml_fire_name(doc, NULL, action);
}

bool cui_xml_click(cui_xml *doc, const char *name) {
    cui_xml_node *n = cui_xml_find(doc, name);
    const char *action;
    if (!n) return false;
    action = cui__xml_attr_raw(n, "onclicked");
    if (!action) action = cui__xml_attr_raw(n, "onaction");
    if (action && *action) {
        cui__xml_fire_name(doc, n, action);
        return true;
    }
    switch (n->tag_id) {
    case CUI__XML_TAG_CHECKBOX:
    case CUI__XML_TAG_SWITCH:
    case CUI__XML_TAG_CHIP: {
        bool *v = cui__xml_slot_bool(doc, n);
        *v = !*v;
        cui__xml_changed_hook(doc, n);
        return true;
    }
    case CUI__XML_TAG_ACCORDION:
    case CUI__XML_TAG_TREE:
    case CUI__XML_TAG_ALERT:
    case CUI__XML_TAG_BANNER:
    case CUI__XML_TAG_DIALOG: {
        bool *v = cui__xml_slot_bool(doc, n);
        *v = !*v;
        return true;
    }
    case CUI__XML_TAG_TABS:
    case CUI__XML_TAG_SEGMENTED:
    case CUI__XML_TAG_SELECT:
    case CUI__XML_TAG_DOCTABS:
    case CUI__XML_TAG_CAROUSEL:
    case CUI__XML_TAG_RATING:
    case CUI__XML_TAG_PAGINATION:
    case CUI__XML_TAG_NUMBER:
    case CUI__XML_TAG_RADIOGROUP: {
        int *v = cui__xml_slot_int(doc, n);
        *v = *v + 1;
        cui__xml_changed_hook(doc, n);
        return true;
    }
    default:
        break;
    }
    return false;
}

bool cui_xml_is_builtin(const char *action) {
    if (!action || !*action) return false;
    if (cui__xml_same_ci(action, "app.quit")) return true;
    if (cui__xml_starts_ci(action, "theme.")) return true;
    if (cui__xml_starts_ci(action, "accent:")) return true;
    if (cui__xml_starts_ci(action, "show:") || cui__xml_starts_ci(action, "hide:") ||
        cui__xml_starts_ci(action, "toggle:") || cui__xml_starts_ci(action, "enable:") ||
        cui__xml_starts_ci(action, "disable:") || cui__xml_starts_ci(action, "page:") ||
        cui__xml_starts_ci(action, "copy:") || cui__xml_starts_ci(action, "popover:"))
        return true;
    return false;
}

void cui_xml_set_builtins(cui_xml *doc, bool enabled) {
    if (doc) doc->builtins = enabled;
}

bool cui_xml_quit_requested(const cui_xml *doc) { return doc ? doc->quit : false; }

void cui_xml_set_quit_requested(cui_xml *doc, bool requested) {
    if (doc) doc->quit = requested;
}

/* ------------------------------------------------------------------------ */
/* Elements: reading and changing                                           */
/* ------------------------------------------------------------------------ */

const char *cui_xml_node_tag(const cui_xml_node *node) { return node && node->tag ? node->tag : ""; }

const char *cui_xml_node_name(const cui_xml_node *node) { return node && node->key ? node->key : ""; }

const char *cui_xml_node_text(cui_xml *doc, cui_xml_node *node) {
    return cui__xml_node_text_computed(doc, node);
}

const char *cui_xml_node_attr(const cui_xml_node *node, const char *name) {
    const char *v = cui__xml_attr_raw(node, name);
    return v ? v : "";
}

const char *cui_xml_node_attr_value(cui_xml *doc, cui_xml_node *node, const char *name) {
    const char *v = cui__xml_attr_text(doc, node, name, NULL);
    return v ? v : "";
}

bool cui_xml_node_set_attr(cui_xml_node *node, const char *name, const char *value) {
    if (!node || !name || !*name) return false;
    cui__xml_node_drop_caches(node);
    if (!cui__xml_attr_put(node, name, value)) return false;
    if (cui__xml_same_ci(name, "name")) {
        free(node->name);
        node->name = value ? cui__xml_strdup(value) : NULL;
    }
    return true;
}

bool cui_xml_node_visible(const cui_xml_node *node) { return node ? node->shown : false; }

void cui_xml_node_show(cui_xml_node *node, bool shown) {
    if (node) node->shown = shown;
}

bool cui_xml_node_enabled(const cui_xml_node *node) { return node ? node->enabled : false; }

void cui_xml_node_enable(cui_xml_node *node, bool enabled) {
    if (node) node->enabled = enabled;
}

bool cui_xml_node_bool(cui_xml *doc, cui_xml_node *node) {
    return node ? *cui__xml_slot_bool(doc, node) : false;
}
int cui_xml_node_int(cui_xml *doc, cui_xml_node *node) {
    return node ? *cui__xml_slot_int(doc, node) : 0;
}
float cui_xml_node_float(cui_xml *doc, cui_xml_node *node) {
    return node ? *cui__xml_slot_float(doc, node) : 0;
}
double cui_xml_node_double(cui_xml *doc, cui_xml_node *node) {
    return node ? *cui__xml_slot_double(doc, node) : 0;
}
const char *cui_xml_node_value(cui_xml *doc, cui_xml_node *node) {
    size_t cap = 0;
    if (!node) return "";
    return cui__xml_slot_text(doc, node, &cap, 0);
}

int cui_xml_node_index(const cui_xml_node *node) { return node ? node->reported : -1; }

cui_id cui_xml_node_id(const cui_xml_node *node) { return node ? node->id : 0; }

cui_rect cui_xml_node_rect(const cui_xml_node *node) {
    cui_rect r = { 0, 0, 0, 0 };
    return node ? node->rect : r;
}

cui_xml_node *cui_xml_node_by_id(cui_xml *doc, cui_id id) {
    cui_xml_node *stack[64];
    int top = 0;
    if (!doc || !doc->root || !id) return NULL;
    stack[top++] = doc->root;
    while (top > 0) {
        cui_xml_node *n = stack[--top];
        int i;
        if (n->id == id) return n;
        for (i = n->kid_count - 1; i >= 0 && top < 64; i--) stack[top++] = n->kids[i];
    }
    return NULL;
}

int cui_xml_node_child_count(const cui_xml_node *node) { return node ? node->kid_count : 0; }

cui_xml_node *cui_xml_node_child(const cui_xml_node *node, int index) {
    if (!node || index < 0 || index >= node->kid_count) return NULL;
    return node->kids[index];
}

cui_xml_node *cui_xml_node_parent(const cui_xml_node *node) { return node ? node->parent : NULL; }

bool cui_xml_node_is_comment(const cui_xml_node *node) { return node ? node->comment : false; }

/* ------------------------------------------------------------------------ */
/* Templates and tags from C                                                */
/* ------------------------------------------------------------------------ */

int cui_xml_register_widgets(cui_xml *doc, const char *text, size_t size) {
    cui_xml *sub;
    int before;
    if (!doc || !text) return 0;
    before = doc->widget_count;
    sub = cui_xml_parse(text, size, NULL);
    if (!sub) return 0;
    /* cui_xml_parse already collected them into `sub`; move them over */
    cui__xml_take_widgets(doc, sub);
    cui_xml_free(sub);
    return doc->widget_count - before;
}

bool cui_xml_is_widget(const cui_xml *doc, const char *tag) {
    int i;
    if (!doc || !tag) return false;
    for (i = 0; i < doc->widget_count; i++)
        if (cui__xml_same_ci(doc->widgets[i].name, tag)) return true;
    return false;
}

bool cui_xml_register_tag(cui_xml *doc, const char *tag, cui_xml_tag_fn fn, void *user) {
    int i;
    if (!doc || !tag || !*tag || !fn) return false;
    if (cui__xml_tag_lookup(tag) != CUI__XML_TAG_UNKNOWN) return false;
    for (i = 0; i < doc->tag_count; i++) {
        if (!cui__xml_same_ci(doc->tags[i].name, tag)) continue;
        doc->tags[i].fn = fn;
        doc->tags[i].user = user;
        return true;
    }
    if (doc->tag_count == doc->tag_cap) {
        int ncap = doc->tag_cap ? doc->tag_cap * 2 : 8;
        cui_xml_tag_entry *nt = cui__xml_realloc(doc->tags, sizeof *nt * (size_t)ncap);
        if (!nt) return false;
        doc->tags = nt;
        doc->tag_cap = ncap;
    }
    doc->tags[doc->tag_count].name = cui__xml_strdup(tag);
    doc->tags[doc->tag_count].fn = fn;
    doc->tags[doc->tag_count].user = user;
    doc->tag_count++;
    return true;
}

bool cui_xml_is_tag_registered(const cui_xml *doc, const char *tag) {
    int i;
    if (!doc || !tag) return false;
    for (i = 0; i < doc->tag_count; i++)
        if (cui__xml_same_ci(doc->tags[i].name, tag)) return true;
    return false;
}

/* The app's tag for an element, NULL when the element is one of ours. */
cui_xml_tag_entry *cui__xml_tag_entry(cui_xml *doc, const cui_xml_node *node) {
    int i;
    if (!doc || !node) return NULL;
    for (i = 0; i < doc->tag_count; i++)
        if (cui__xml_same_ci(doc->tags[i].name, node->tag)) return &doc->tags[i];
    return NULL;
}

/* ------------------------------------------------------------------------ */
/* Live reload                                                              */
/* ------------------------------------------------------------------------ */

typedef struct cui__xml_state {
    char *key;
    bool shown, enabled;
    bool b;
    int i;
    float f;
    double d;
    char *buf;
} cui__xml_state;

typedef struct cui__xml_states {
    cui__xml_state *items;
    int count, cap;
} cui__xml_states;

static void cui__xml_states_free(cui__xml_states *s) {
    int i;
    for (i = 0; i < s->count; i++) {
        free(s->items[i].key);
        free(s->items[i].buf);
    }
    free(s->items);
    s->items = NULL;
    s->count = s->cap = 0;
}

static void cui__xml_states_save(cui__xml_states *s, cui_xml_node *node) {
    int i;
    if (!node) return;
    if (!node->comment && node->key && *node->key) {
        if (s->count == s->cap) {
            int ncap = s->cap ? s->cap * 2 : 32;
            cui__xml_state *ni = cui__xml_realloc(s->items, sizeof *ni * (size_t)ncap);
            if (!ni) return;
            s->items = ni;
            s->cap = ncap;
        }
        s->items[s->count].key = cui__xml_strdup(node->key);
        s->items[s->count].shown = node->shown;
        s->items[s->count].enabled = node->enabled;
        s->items[s->count].b = node->b;
        s->items[s->count].i = node->i;
        s->items[s->count].f = node->f;
        s->items[s->count].d = node->d;
        s->items[s->count].buf = node->buf ? cui__xml_strdup(node->buf) : NULL;
        s->count++;
    }
    for (i = 0; i < node->kid_count; i++) cui__xml_states_save(s, node->kids[i]);
}

static void cui__xml_states_apply(cui__xml_states *s, cui_xml_node *node) {
    int i;
    if (!node) return;
    if (node->key) {
        for (i = 0; i < s->count; i++) {
            if (!cui__xml_same(s->items[i].key, node->key)) continue;
            node->shown = s->items[i].shown;
            node->enabled = s->items[i].enabled;
            node->b = s->items[i].b;
            node->i = s->items[i].i;
            node->f = s->items[i].f;
            node->d = s->items[i].d;
            if (s->items[i].buf) {
                size_t len = strlen(s->items[i].buf);
                if (!node->buf) {
                    node->buf = cui__xml_strdup(s->items[i].buf);
                    node->buf_cap = len + 1;
                } else if (node->buf_cap > len) {
                    memcpy(node->buf, s->items[i].buf, len + 1);
                }
            }
            break;
        }
    }
    for (i = 0; i < node->kid_count; i++) cui__xml_states_apply(s, node->kids[i]);
}

static void cui__xml_replace_tree(cui_xml *doc, cui_xml *fresh) {
    cui__xml_states states;
    int i;
    memset(&states, 0, sizeof states);
    cui__xml_states_save(&states, doc->root);

    cui__xml_doc_reset(doc);
    cui__xml_free_widgets(doc);
    doc->root = fresh->root;
    fresh->root = NULL;
    doc->title = fresh->title;
    fresh->title = NULL;
    doc->width = fresh->width;
    doc->height = fresh->height;
    doc->theme = fresh->theme;
    doc->has_theme = fresh->has_theme;
    doc->accent = fresh->accent;
    doc->has_accent = fresh->has_accent;
    for (i = 0; i < fresh->widget_count; i++) {
        if (doc->widget_count == doc->widget_cap) {
            int ncap = doc->widget_cap ? doc->widget_cap * 2 : 4;
            cui_xml_widget *nw = cui__xml_realloc(doc->widgets, sizeof *nw * (size_t)ncap);
            if (!nw) break;
            doc->widgets = nw;
            doc->widget_cap = ncap;
        }
        doc->widgets[doc->widget_count++] = fresh->widgets[i];
        memset(&fresh->widgets[i], 0, sizeof fresh->widgets[i]);
    }
    doc->ok = fresh->ok;
    snprintf(doc->last_error, sizeof doc->last_error, "%s", fresh->last_error);
    doc->prepared = false;

    cui__xml_states_apply(&states, doc->root);
    cui__xml_states_free(&states);
}

bool cui_xml_poll(cui_xml *doc) {
    long long size = 0, time = 0;
    size_t text_size = 0;
    char *text;
    cui_xml *fresh;
    if (!doc || !doc->path || !*doc->path) return false;
    if (!cui__xml_file_stamp_of(doc->path, &size, &time)) return false;
    if (doc->has_stamp && size == doc->stamp[0] && time == doc->stamp[1]) return false;
    doc->stamp[0] = size;
    doc->stamp[1] = time;
    doc->has_stamp = true;
    text = cui__xml_read_file(doc->path, &text_size);
    if (!text) {
        if (doc->reload_fn) doc->reload_fn(doc, false, doc->reload_user);
        return false;
    }
    fresh = cui__xml_doc_new();
    if (fresh) {
        /* the folder first: an <Include> in the new text is looked for there */
        cui__xml_doc_set_path(fresh, doc->path);
        cui__xml_build(fresh, text, text_size, NULL);
    }
    free(text);
    if (!fresh || !fresh->root || !fresh->ok) {
        if (fresh) {
            snprintf(doc->last_error, sizeof doc->last_error, "%s", cui_xml_last_error(fresh));
            cui_xml_free(fresh);
        }
        if (doc->reload_fn) doc->reload_fn(doc, false, doc->reload_user);
        return false;
    }
    cui__xml_replace_tree(doc, fresh);
    cui_xml_free(fresh);
    if (doc->reload_fn) doc->reload_fn(doc, true, doc->reload_user);
    return true;
}

void cui_xml_set_watch(cui_xml *doc, bool enabled) { if (doc) doc->watch = enabled; }

bool cui_xml_get_watch(const cui_xml *doc) { return doc ? doc->watch : false; }

void cui_xml_set_reload_callback(cui_xml *doc, cui_xml_reload_fn fn, void *user) {
    if (!doc) return;
    doc->reload_fn = fn;
    doc->reload_user = user;
}

/* ------------------------------------------------------------------------ */
/* Paths                                                                    */
/* ------------------------------------------------------------------------ */

const char *cui_xml_resolve_path(cui_xml *doc, const char *path) {
    char *joined;
    char *scratch;
    if (!doc || !path || !*path) return "";
    joined = cui__xml_resolve_in(doc, path);
    if (!joined) return "";
    if (cui__xml_file_exists(joined)) {
        scratch = cui__xml_scratch_alloc(doc, strlen(joined) + 1);
        if (scratch) snprintf(scratch, strlen(joined) + 1, "%s", joined);
        free(joined);
        return scratch ? scratch : "";
    }
    if (path[0] != '/' && path[0] != '\\' && cui__xml_file_exists(path)) {
        free(joined);
        scratch = cui__xml_scratch_alloc(doc, strlen(path) + 1);
        if (scratch) snprintf(scratch, strlen(path) + 1, "%s", path);
        return scratch ? scratch : "";
    }
    scratch = cui__xml_scratch_alloc(doc, strlen(joined) + 1);
    if (scratch) snprintf(scratch, strlen(joined) + 1, "%s", joined);
    free(joined);
    return scratch ? scratch : "";
}
