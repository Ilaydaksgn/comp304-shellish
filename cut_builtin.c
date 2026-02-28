#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// parse a string into an integer array .
//field indices are 1-based
static int parse_fields(const char *s, int **out_fields, int *out_n) {
    // parses "1,3,10" into int array (1-based indices)
    int cap = 8, n = 0; //start with a small capacity
    int *arr = malloc(sizeof(int) * cap);
    if (!arr) return -1;

    const char *p = s;
    while (*p) {
        //skip spaces and commas
        while (*p == ' ' || *p == '\t' || *p == ',') p++;
        if (!*p) break;
	//must start with a digit
        if (!isdigit((unsigned char)*p)) { free(arr); return -1; }
	//conver number manually
        long v = 0;
        while (isdigit((unsigned char)*p)) {
            v = v * 10 + (*p - '0');
            p++;
        }
	//validation
        if (v <= 0 || v > 1000000) { free(arr); return -1; }
	//grow array if needed
        if (n == cap) {
            cap *= 2;
            int *tmp = realloc(arr, sizeof(int) * cap);
            if (!tmp) { free(arr); return -1; }
            arr = tmp;
        }
        arr[n++] = (int)v;
	//skip optional spaces
        while (*p == ' ' || *p == '\t') p++;
        if (*p == ',') p++;
    }

    if (n == 0) { free(arr); return -1; }
    *out_fields = arr;
    *out_n = n;
    return 0;
}

static void print_selected_fields(const char *line, char delim, const int *fields, int nf) {
    // We scan tokens in one pass and print those whose index is in fields list.
    // fields are 1-based. Output delimiter same as input delimiter.
    int target_i = 0;
    int field_idx = 1;

    const char *p = line;
    const char *tok_start = p;

    // helper to emit token [tok_start, tok_end)
    auto void emit(const char *a, const char *b, int *printed_any) {
        if (*printed_any) putchar(delim);
        fwrite(a, 1, (size_t)(b - a), stdout);
        *printed_any = 1;
    }

    int printed_any = 0;

    while (1) {
        if (*p == delim || *p == '\0' || *p == '\n') {
            const char *tok_end = p;

            // emit if this field is requested (fields list may be unsorted; but typical is sorted)
            // We'll check by scanning the list (nf is small). Simple and safe.
            for (int i = 0; i < nf; i++) {
                if (fields[i] == field_idx) {
                    emit(tok_start, tok_end, &printed_any);
                    break;
                }
            }

            if (*p == '\0' || *p == '\n') break;

            // move to next field
            p++;
            tok_start = p;
            field_idx++;
            continue;
        }
        p++;
    }

    putchar('\n');
}

int builtin_cut(int argc, char **argv) {
    // argv[0] == "cut" 
    char delim = '\t';
    const char *fields_str = NULL;

    // parse  command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "cut: -d requires a delimiter character\n");
                return 1;
            }
            if (argv[i + 1][0] == '\0' || argv[i + 1][1] != '\0') {
                fprintf(stderr, "cut: delimiter must be a single character\n");
                return 1;
            }
            delim = argv[i + 1][0];
            i++;
        } else if (strncmp(argv[i], "-d", 2) == 0) {
            // support -d:
            if (argv[i][2] == '\0' || argv[i][3] != '\0') {
                fprintf(stderr, "cut: delimiter must be a single character\n");
                return 1;
            }
            delim = argv[i][2];
        } else if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "cut: -f requires a field list (e.g., 1,3)\n");
                return 1;
            }
            fields_str = argv[i + 1];
            i++;
        } else if (strncmp(argv[i], "-f", 2) == 0) {
            fields_str = argv[i] + 2;
            if (!*fields_str) {
                fprintf(stderr, "cut: -f requires a field list (e.g., 1,3)\n");
                return 1;
            }
        } else {
            fprintf(stderr, "cut: unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    if (!fields_str) {
        fprintf(stderr, "cut: missing -f fields\n");
        return 1;
    }
    //covert field string into integer array
    int *fields = NULL, nf = 0;
    if (parse_fields(fields_str, &fields, &nf) != 0) {
        fprintf(stderr, "cut: invalid field list: %s\n", fields_str);
        return 1;
    }
    //read input line by line from stdin
    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    while ((len = getline(&line, &cap, stdin)) != -1) {
        (void)len;
        print_selected_fields(line, delim, fields, nf);
    }

    free(fields);
    free(line);
    return 0;
}
