/* Two example programs, one window each, drawn by --shot and compared.
 *
 *   cui_xml_pair_shots <exe-a> <exe-b> [dir]
 *
 * The pair is a C example and the same interface in XML (examples/hello.c and
 * examples/helloxml); both write a PNG with the software renderer and this
 * program says whether the two files are the same bytes, which is what "the
 * document draws the example" means here. The pictures stay in `dir` (the
 * current directory by default) with the names of the two programs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  define SEP "\\"
#else
#  define SEP "/"
#endif

static const char *base_name(const char *path) {
    const char *slash = strrchr(path, '/');
#ifdef _WIN32
    const char *back = strrchr(path, '\\');
    if (back && (!slash || back > slash)) slash = back;
#endif
    return slash ? slash + 1 : path;
}

static void stem_of(const char *path, char *out, size_t cap) {
    const char *name = base_name(path);
    size_t n = 0;
    while (name[n] && name[n] != '.' && n + 1 < cap) {
        out[n] = name[n];
        n++;
    }
    out[n] = 0;
}

static unsigned char *read_file(const char *path, long *size) {
    FILE *f = fopen(path, "rb");
    unsigned char *data;
    long n;
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    data = malloc((size_t)n + 1);
    if (!data) {
        fclose(f);
        return NULL;
    }
    if (n > 0 && fread(data, 1, (size_t)n, f) != (size_t)n) {
        free(data);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *size = n;
    return data;
}

static int shoot(const char *exe, const char *png) {
    char command[4096];
    int rc;
#ifdef _WIN32
    /* cmd strips the first and the last quote of a command that begins with
     * one, so a quoted program path is run through `call` */
    snprintf(command, sizeof command, "call \"%s\" --shot \"%s\"", exe, png);
#else
    snprintf(command, sizeof command, "\"%s\" --shot \"%s\"", exe, png);
#endif
    rc = system(command);
    if (rc != 0) {
        fprintf(stderr, "%s --shot %s: exit %d\n", exe, png, rc);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    const char *dir = argc > 3 ? argv[3] : ".";
    char stem_a[128], stem_b[128];
    char png_a[1024], png_b[1024];
    unsigned char *a, *b;
    long na = 0, nb = 0;
    int fails = 0;

    if (argc < 3) {
        fprintf(stderr, "usage: %s <exe-a> <exe-b> [dir]\n", argv[0]);
        return 2;
    }
    stem_of(argv[1], stem_a, sizeof stem_a);
    stem_of(argv[2], stem_b, sizeof stem_b);
    snprintf(png_a, sizeof png_a, "%s%s%s.png", dir, SEP, stem_a);
    snprintf(png_b, sizeof png_b, "%s%s%s.png", dir, SEP, stem_b);
    remove(png_a);
    remove(png_b);
    fails += shoot(argv[1], png_a);
    fails += shoot(argv[2], png_b);
    if (fails) return 1;
    a = read_file(png_a, &na);
    b = read_file(png_b, &nb);
    if (!a || !b) {
        fprintf(stderr, "%s or %s was not written\n", png_a, png_b);
        free(a);
        free(b);
        return 1;
    }
    if (na != nb) {
        printf("%s and %s differ in size: %ld against %ld bytes\n", stem_a, stem_b, na, nb);
        fails++;
    } else if (memcmp(a, b, (size_t)na) != 0) {
        long i, first = -1, count = 0;
        for (i = 0; i < na; i++) {
            if (a[i] != b[i]) {
                if (first < 0) first = i;
                count++;
            }
        }
        printf("%s and %s differ: %ld of %ld bytes, first at %ld\n", stem_a, stem_b, count, na, first);
        fails++;
    } else {
        printf("%s and %s draw the same %ld bytes\n", stem_a, stem_b, na);
    }
    free(a);
    free(b);
    return fails ? 1 : 0;
}
