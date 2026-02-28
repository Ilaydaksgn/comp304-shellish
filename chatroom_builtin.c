#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

//global flag used to stop loops when ctrl+c is pressed.

static volatile sig_atomic_t g_stop = 0;

static void on_sigint(int sig) {
    (void)sig;
    g_stop = 1;
}
//creates or verifies chatroom directory
// /tmp/chatroom-<roomname>
static int ensure_room_dir(const char *room, char *out_dir, size_t out_sz) {
    if (!room || !*room) return -1;
    snprintf(out_dir, out_sz, "/tmp/chatroom-%s", room);

    struct stat st;
    //If directory already exists , check it is a directory
    if (stat(out_dir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "chatroom: %s exists but is not a directory\n", out_dir);
            return -1;
        }
        return 0;
    }
    //otherwise create  a dirctory 
    if (mkdir(out_dir, 0777) < 0) {
        perror("mkdir");
        return -1;
    }
    return 0;
}

//creates  FIFO file for a user inside the room directory 
//each  user has its own FIFO
static int ensure_user_fifo(const char *room_dir, const char *user, char *out_fifo, size_t out_sz) {
    if (!user || !*user) return -1;
    snprintf(out_fifo, out_sz, "%s/%s", room_dir, user);

    struct stat st;
    //If FFO already exists , verify type  
    if (lstat(out_fifo, &st) == 0) {
        if (!S_ISFIFO(st.st_mode)) {
            fprintf(stderr, "chatroom: %s exists but is not a FIFO\n", out_fifo);
            return -1;
        }
        return 0;
    }
    //create FIFO 
    if (mkfifo(out_fifo, 0666) < 0) {
        perror("mkfifo");
        return -1;
    }
    return 0;
}
//sen message to all users in the room 
static void send_to_all(const char *room_dir, const char *self_user, const char *msg) {
    DIR *d = opendir(room_dir);
    if (!d) { perror("opendir"); return; }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        //skip special entries 
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        if (strcmp(ent->d_name, self_user) == 0) continue; // don't send to self

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", room_dir, ent->d_name);

        struct stat st;
        if (lstat(path, &st) != 0) continue;
        if (!S_ISFIFO(st.st_mode)) continue;

        // Open FIFO in non-blocking mode so if receiver is not listening we don't hang forever
        int fd = open(path, O_WRONLY | O_NONBLOCK);
        if (fd < 0) {
            // If no reader, ENXIO is common. Ignore silently.
            continue;
        }
        (void)write(fd, msg, strlen(msg));
        close(fd);
    }
    closedir(d);
}

static void receiver_loop(const char *fifo_path) {
    // Open FIFO for reading. If no writer exists, open() may block.
    // open read end in blocking mode; writers open O_NONBLOCK so they won't stall.
    int fd = open(fifo_path, O_RDONLY);
    if (fd < 0) { perror("open read fifo"); exit(1); }

    char buf[1024];
    while (!g_stop) {
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            fputs(buf, stdout);
            fflush(stdout);
        } else if (n == 0) {
            // writer closed; keep waiting
            // reopening helps if FIFO got into EOF state
            close(fd);
            fd = open(fifo_path, O_RDONLY);
            if (fd < 0) { perror("open read fifo"); break; }
        } else {
            if (errno == EINTR) continue;
            perror("read fifo");
            break;
        }
    }

    close(fd);
}

int builtin_chatroom(int argc, char **argv) {
    // usage: chatroom <roomname> <username>
    if (argc != 3) {
        fprintf(stderr, "usage: chatroom <roomname> <username>\n");
        return 1;
    }

    const char *room = argv[1];
    const char *user = argv[2];

    char room_dir[256];
    char my_fifo[512];

    if (ensure_room_dir(room, room_dir, sizeof(room_dir)) != 0) return 1;
    if (ensure_user_fifo(room_dir, user, my_fifo, sizeof(my_fifo)) != 0) return 1;

    // Handle Ctrl+C to exit 
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigaction(SIGINT, &sa, NULL);

    printf("Welcome to %s!\n", room);
    printf("[%s] %s > ", room, user);
    fflush(stdout);

    pid_t rx = fork();
    if (rx < 0) { perror("fork"); return 1; }

    if (rx == 0) {
        // child: receiver
        receiver_loop(my_fifo);
        exit(0);
    }

    // parent: sender loop (read from stdin, broadcast)
    char *line = NULL;
    size_t cap = 0;

    while (!g_stop) {
        ssize_t n = getline(&line, &cap, stdin);
        if (n < 0) break;

        // If user just pressed Enter, re-print prompt
        if (n == 1 && line[0] == '\n') {
            printf("[%s] %s > ", room, user);
            fflush(stdout);
            continue;
        }

        // Format: [room] user: message
        char msg[1400];
        snprintf(msg, sizeof(msg), "[%s] %s: %s", room, user, line);

        send_to_all(room_dir, user, msg);

        printf("[%s] %s > ", room, user);
        fflush(stdout);
    }

    free(line);
    g_stop = 1;
    kill(rx, SIGINT); // stop receiver process
    return 0;
}
