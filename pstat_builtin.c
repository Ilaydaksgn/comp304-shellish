#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//read the entire file into a buffer 
static int read_file_to_buf(const char *path, char *buf, size_t bufsz) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    size_t n = fread(buf, 1, bufsz - 1, f);
    buf[n] = '\0'; //make it a valid C string 
    fclose(f);
    return 0;
}

//Read only the first line of a file 
static int read_first_line(const char *path, char *line, size_t linesz) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(line, (int)linesz, f)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    // remove trailing newline 
    size_t L = strlen(line);
    if (L && line[L - 1] == '\n') line[L - 1] = '\0';
    return 0;
}
//find and print a specific key : line inside /proc/<pid>/status text.


static void print_status_kv(const char *status_buf, const char *key) {
    // prints lines like "Name:\txyz"
    size_t keylen = strlen(key);
    const char *p = status_buf;
    while (*p) {
        const char *line_start = p;
        const char *nl = strchr(p, '\n');
        size_t linelen = nl ? (size_t)(nl - p) : strlen(p);
        //if line starts with the key,print that full line
        if (linelen > keylen && strncmp(line_start, key, keylen) == 0) {
            // print whole line
            fwrite(line_start, 1, linelen, stdout);
            putchar('\n');
            return;
        }

        if (!nl) break;
        p = nl + 1;
    }
}

static int parse_stat(const char *stat_line,
                      int *out_pid,
                      char *out_comm, size_t comm_sz,
                      char *out_state,
                      int *out_ppid) {
    // /proc/<pid>/stat format: pid (comm) state ppid ...
    // comm can contain spaces, enclosed in parentheses.
    const char *lparen = strchr(stat_line, '(');
    const char *rparen = strrchr(stat_line, ')');
    if (!lparen || !rparen || rparen < lparen) return -1;

    // pid is before '('
    char pidbuf[32];
    size_t pidlen = (size_t)(lparen - stat_line);
    if (pidlen >= sizeof(pidbuf)) return -1;
    memcpy(pidbuf, stat_line, pidlen);
    pidbuf[pidlen] = '\0';

    int pid = atoi(pidbuf);
    if (pid <= 0) return -1;

    // comm between parentheses
    size_t comm_len = (size_t)(rparen - lparen - 1);
    if (comm_len >= comm_sz) comm_len = comm_sz - 1;
    memcpy(out_comm, lparen + 1, comm_len);
    out_comm[comm_len] = '\0';

    // after ") " comes: state ppid ...
    const char *after = rparen + 1;
    while (*after == ' ') after++;

    // state is first token
    if (!*after) return -1;
    char state = *after;
    *out_state = state;

    // move to next token (ppid)
    after++;
    while (*after == ' ') after++;

    // ppid token
    int ppid = atoi(after);
    *out_pid = pid;
    *out_ppid = ppid;
    return 0;
}

int builtin_pstat(int argc, char **argv) {
    // usage: pstat <pid>
    if (argc != 2) {
        fprintf(stderr, "usage: pstat <pid>\n");
        return 1;
    }
    //ensure pid is numeric
    const char *pid_str = argv[1];
    for (const char *p = pid_str; *p; p++) {
        if (*p < '0' || *p > '9') {
            fprintf(stderr, "pstat: pid must be a number\n");
            return 1;
        }
    }
    //build /proc paths
    char stat_path[256], status_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%s/stat", pid_str);
    snprintf(status_path, sizeof(status_path), "/proc/%s/status", pid_str);
    //read and parse 
    char stat_line[4096];
    if (read_first_line(stat_path, stat_line, sizeof(stat_line)) != 0) {
        fprintf(stderr, "pstat: cannot read %s: %s\n", stat_path, strerror(errno));
        return 1;
    }

    int pid = 0, ppid = 0;
    char comm[256];
    char state = '?';

    if (parse_stat(stat_line, &pid, comm, sizeof(comm), &state, &ppid) != 0) {
        fprintf(stderr, "pstat: failed to parse %s\n", stat_path);
        return 1;
    }

    char status_buf[16384];
    if (read_file_to_buf(status_path, status_buf, sizeof(status_buf)) != 0) {
        fprintf(stderr, "pstat: cannot read %s: %s\n", status_path, strerror(errno));
        return 1;
    }
  
   //print summary
    printf("=== pstat for PID %d ===\n", pid);
    printf("Comm: %s\n", comm);
    printf("State: %c\n", state);
    printf("PPid: %d\n", ppid);

    // Useful fields from /proc/<pid>/status
    print_status_kv(status_buf, "Name:");
    print_status_kv(status_buf, "State:");
    print_status_kv(status_buf, "PPid:");
    print_status_kv(status_buf, "Threads:");
    print_status_kv(status_buf, "VmSize:");
    print_status_kv(status_buf, "VmRSS:");
    print_status_kv(status_buf, "RssAnon:");
    print_status_kv(status_buf, "RssFile:");
    print_status_kv(status_buf, "Uid:");
    print_status_kv(status_buf, "Gid:");

    return 0;
}
