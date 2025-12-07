/*
 * rfs.c -- Remote File System client
 *
 * Commands:
 *   ./rfs WRITE local-file [remote-file]
 *   ./rfs GET   [-v N] remote-file [local-file]
 *   ./rfs RM    remote-file
 *   ./rfs LS    remote-file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP   "34.19.98.211"
#define SERVER_PORT 2000

/*------------------------------------------------------------*/
/*                   Utility Functions                        */
/*------------------------------------------------------------*/

/* Send exactly len bytes */
static int send_all(int sockfd, const void *buf, size_t len) {
    const uint8_t *p = (const uint8_t *)buf;
    size_t total = 0;

    while (total < len) {
        ssize_t n = send(sockfd, p + total, len - total, 0);
        if (n < 0) {
            perror("send");
            return -1;
        }
        if (n == 0) {
            fprintf(stderr, "send_all: connection closed unexpectedly\n");
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

/* Receive exactly len bytes */
static int recv_all(int sockfd, void *buf, size_t len) {
    uint8_t *p = (uint8_t *)buf;
    size_t total = 0;

    while (total < len) {
        ssize_t n = recv(sockfd, p + total, len - total, 0);
        if (n < 0) {
            perror("recv");
            return -1;
        }
        if (n == 0) {
            fprintf(stderr, "recv_all: connection closed unexpectedly\n");
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

/* Get basename of a path */
static const char *basename_const(const char *path) {
    const char *slash = strrchr(path, '/');
    if (slash == NULL) return path;
    return slash + 1;
}

/* Connect to server */
static int connect_to_server(void) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/*------------------------------------------------------------*/
/*                         WRITE                              */
/*------------------------------------------------------------*/

static int do_write(const char *local_path, const char *remote_path) {
    FILE *fp = fopen(local_path, "rb");
    if (!fp) {
        perror("fopen local file");
        return 1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(fp);
        return 1;
    }

    long file_size_long = ftell(fp);
    if (file_size_long < 0 || file_size_long > (long)UINT32_MAX) {
        fprintf(stderr, "File too large or ftell error\n");
        fclose(fp);
        return 1;
    }
    uint32_t file_size = (uint32_t)file_size_long;
    rewind(fp);

    uint8_t *file_buf = malloc(file_size);
    if (!file_buf) {
        perror("malloc");
        fclose(fp);
        return 1;
    }

    size_t read_bytes = fread(file_buf, 1, file_size, fp);
    fclose(fp);
    if (read_bytes != file_size) {
        fprintf(stderr, "Short read of local file\n");
        free(file_buf);
        return 1;
    }

    int sockfd = connect_to_server();
    if (sockfd < 0) {
        free(file_buf);
        return 1;
    }

    printf("Connected (WRITE)\n");

    /* Send command */
    const char cmd[5] = {'W','R','I','T','E'};
    if (send_all(sockfd, cmd, 5) < 0) {
        close(sockfd);
        free(file_buf);
        return 1;
    }

    /* Send lengths */
    uint32_t path_len = (uint32_t)strlen(remote_path);
    uint32_t path_len_net  = htonl(path_len);
    uint32_t file_size_net = htonl(file_size);

    if (send_all(sockfd, &path_len_net, 4) < 0 ||
        send_all(sockfd, &file_size_net, 4) < 0 ||
        send_all(sockfd, remote_path, path_len) < 0 ||
        send_all(sockfd, file_buf, file_size) < 0) {
        close(sockfd);
        free(file_buf);
        return 1;
    }

    printf("WRITE complete: %s -> %s (%u bytes)\n",
           local_path, remote_path, file_size);

    close(sockfd);
    free(file_buf);
    return 0;
}

/*------------------------------------------------------------*/
/*                           GET                              */
/*      Now supports: GET [-v N] remote-path [local-path]     */
/*------------------------------------------------------------*/

static int do_get(const char *remote_path,
                  const char *maybe_local_path,
                  int version) {
    char local_path_buf[1024];
    char remote_buf[1024];

    /* Effective remote path to request from server */
    const char *remote_to_send = remote_path;
    if (version > 0) {
        if (snprintf(remote_buf, sizeof(remote_buf),
                     "%s.v%d", remote_path, version) >= (int)sizeof(remote_buf)) {
            fprintf(stderr, "Remote path too long\n");
            return 1;
        }
        remote_to_send = remote_buf;
    }

    /* Local path: explicit or basename of what we're requesting */
    const char *local_path;
    if (maybe_local_path != NULL) {
        local_path = maybe_local_path;
    } else {
        const char *base = basename_const(remote_to_send);
        if (snprintf(local_path_buf, sizeof(local_path_buf),
                     "%s", base) >= (int)sizeof(local_path_buf)) {
            fprintf(stderr, "Local path too long\n");
            return 1;
        }
        local_path = local_path_buf;
    }

    int sockfd = connect_to_server();
    if (sockfd < 0) return 1;

    if (version > 0) {
        printf("Connected (GET -v %d)\n", version);
    } else {
        printf("Connected (GET)\n");
    }

    const char cmd[5] = {'G','E','T',' ',' '};
    if (send_all(sockfd, cmd, 5) < 0) {
        close(sockfd);
        return 1;
    }

    uint32_t path_len = (uint32_t)strlen(remote_to_send);
    uint32_t path_len_net = htonl(path_len);

    if (send_all(sockfd, &path_len_net, 4) < 0 ||
        send_all(sockfd, remote_to_send, path_len) < 0) {
        close(sockfd);
        return 1;
    }

    /* Receive status */
    uint32_t status_net;
    if (recv_all(sockfd, &status_net, 4) < 0) {
        close(sockfd);
        return 1;
    }
    uint32_t status = ntohl(status_net);

    if (status != 0) {
        fprintf(stderr, "GET error: remote file not found (%s)\n", remote_to_send);
        close(sockfd);
        return 1;
    }

    /* Receive file size */
    uint32_t file_size_net;
    if (recv_all(sockfd, &file_size_net, 4) < 0) {
        close(sockfd);
        return 1;
    }
    uint32_t file_size = ntohl(file_size_net);

    uint8_t *buf = malloc(file_size);
    if (!buf) {
        perror("malloc");
        close(sockfd);
        return 1;
    }

    if (recv_all(sockfd, buf, file_size) < 0) {
        free(buf);
        close(sockfd);
        return 1;
    }

    FILE *fp = fopen(local_path, "wb");
    if (!fp) {
        perror("fopen local");
        free(buf);
        close(sockfd);
        return 1;
    }

    size_t written = fwrite(buf, 1, file_size, fp);
    fclose(fp);
    free(buf);
    close(sockfd);

    if (written != file_size) {
        fprintf(stderr, "Short write to '%s'\n", local_path);
        return 1;
    }

    if (version > 0) {
        printf("GET -v %d complete: %s -> %s (%u bytes)\n",
               version, remote_to_send, local_path, file_size);
    } else {
        printf("GET complete: %s -> %s (%u bytes)\n",
               remote_to_send, local_path, file_size);
    }

    return 0;
}

/*------------------------------------------------------------*/
/*                            RM                              */
/*------------------------------------------------------------*/

static int do_rm(const char *remote_path) {
    int sockfd = connect_to_server();
    if (sockfd < 0) return 1;

    printf("Connected (RM)\n");

    const char cmd[5] = {'R','M',' ',' ',' '};
    if (send_all(sockfd, cmd, 5) < 0) {
        close(sockfd);
        return 1;
    }

    uint32_t path_len = (uint32_t)strlen(remote_path);
    uint32_t path_len_net = htonl(path_len);

    if (send_all(sockfd, &path_len_net, 4) < 0 ||
        send_all(sockfd, remote_path, path_len) < 0) {
        close(sockfd);
        return 1;
    }

    /* Receive status */
    uint32_t status_net;
    if (recv_all(sockfd, &status_net, 4) < 0) {
        close(sockfd);
        return 1;
    }
    close(sockfd);

    uint32_t status = ntohl(status_net);

    if (status == 0) {
        printf("RM success: '%s' deleted\n", remote_path);
        return 0;
    }
    if (status == 1) {
        fprintf(stderr, "RM error: '%s' not found\n", remote_path);
    } else if (status == 2) {
        fprintf(stderr, "RM error: directory not empty: '%s'\n", remote_path);
    } else {
        fprintf(stderr, "RM error: removal failed for '%s' (status=%u)\n",
                remote_path, status);
    }
    return 1;
}

/*------------------------------------------------------------*/
/*                             LS                             */
/*------------------------------------------------------------*/

static int do_ls(const char *remote_path) {
    int sockfd = connect_to_server();
    if (sockfd < 0) return 1;

    printf("Connected (LS)\n");

    const char cmd[5] = {'L','S',' ',' ',' '};
    if (send_all(sockfd, cmd, 5) < 0) {
        close(sockfd);
        return 1;
    }

    uint32_t path_len = (uint32_t)strlen(remote_path);
    uint32_t path_len_net = htonl(path_len);

    if (send_all(sockfd, &path_len_net, 4) < 0 ||
        send_all(sockfd, remote_path, path_len) < 0) {
        close(sockfd);
        return 1;
    }

    /* Receive count */
    uint32_t count_net;
    if (recv_all(sockfd, &count_net, 4) < 0) {
        close(sockfd);
        return 1;
    }
    uint32_t count = ntohl(count_net);

    if (count == 0) {
        printf("No versions found for '%s'\n", remote_path);
        close(sockfd);
        return 0;
    }

    printf("Versions for '%s':\n", remote_path);
    printf("  %-30s  %s\n", "NAME", "LAST MODIFIED");

    for (uint32_t i = 0; i < count; i++) {
        uint32_t name_len_net, ts_len_net;

        if (recv_all(sockfd, &name_len_net, 4) < 0 ||
            recv_all(sockfd, &ts_len_net, 4) < 0) {
            close(sockfd);
            return 1;
        }

        uint32_t name_len = ntohl(name_len_net);
        uint32_t ts_len   = ntohl(ts_len_net);

        char *name_buf = malloc(name_len + 1);
        char *ts_buf   = malloc(ts_len + 1);
        if (!name_buf || !ts_buf) {
            perror("malloc");
            free(name_buf);
            free(ts_buf);
            close(sockfd);
            return 1;
        }

        if (recv_all(sockfd, name_buf, name_len) < 0 ||
            recv_all(sockfd, ts_buf, ts_len) < 0) {
            free(name_buf);
            free(ts_buf);
            close(sockfd);
            return 1;
        }

        name_buf[name_len] = '\0';
        ts_buf[ts_len]     = '\0';

        printf("  %-30s  %s\n", name_buf, ts_buf);

        free(name_buf);
        free(ts_buf);
    }

    close(sockfd);
    return 0;
}

/*------------------------------------------------------------*/
/*                           main()                           */
/*------------------------------------------------------------*/

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
            "Usage:\n"
            "  %s WRITE local-path [remote-path]\n"
            "  %s GET   [-v N] remote-path [local-path]\n"
            "  %s RM    remote-path\n"
            "  %s LS    remote-path\n",
            argv[0], argv[0], argv[0], argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "WRITE") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s WRITE local-path [remote-path]\n", argv[0]);
            return 1;
        }
        const char *local_path  = argv[2];
        const char *remote_path = (argc >= 4) ? argv[3] : argv[2];
        return do_write(local_path, remote_path);

    } else if (strcmp(cmd, "GET") == 0) {
        int version = -1;
        const char *remote_path;
        const char *local_path = NULL;
        int idx = 2;

        if (argc >= 5 && strcmp(argv[2], "-v") == 0) {
            version = atoi(argv[3]);
            if (version <= 0) {
                fprintf(stderr, "GET: -v requires positive integer version\n");
                return 1;
            }
            idx = 4;
        }

        if (argc <= idx) {
            fprintf(stderr,
                "Usage: %s GET [-v N] remote-path [local-path]\n",
                argv[0]);
            return 1;
        }

        remote_path = argv[idx];
        if (argc > idx + 1) {
            local_path = argv[idx + 1];
        }

        return do_get(remote_path, local_path, version);

    } else if (strcmp(cmd, "RM") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s RM remote-path\n", argv[0]);
            return 1;
        }
        return do_rm(argv[2]);

    } else if (strcmp(cmd, "LS") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s LS remote-path\n", argv[0]);
            return 1;
        }
        return do_ls(argv[2]);
    }

    fprintf(stderr, "Unknown command: %s\n", cmd);
    return 1;
}