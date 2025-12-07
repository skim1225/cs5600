/*
 * rfs.c -- Remote File System client
 *
 * Commands:
 *   ./rfs WRITE local-file [remote-file]
 *   ./rfs GET   remote-file [local-file]
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
        total += n;
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

    fseek(fp, 0, SEEK_END);
    long file_size_long = ftell(fp);
    rewind(fp);

    if (file_size_long < 0 || file_size_long > (long)UINT32_MAX) {
        fprintf(stderr, "File too large\n");
        fclose(fp);
        return 1;
    }
    uint32_t file_size = (uint32_t)file_size_long;

    uint8_t *file_buf = malloc(file_size);
    if (!file_buf) {
        perror("malloc");
        fclose(fp);
        return 1;
    }

    fread(file_buf, 1, file_size, fp);
    fclose(fp);

    int sockfd = connect_to_server();
    if (sockfd < 0) {
        free(file_buf);
        return 1;
    }

    printf("Connected (WRITE)\n");

    /* Send command */
    const char cmd[5] = {'W','R','I','T','E'};
    send_all(sockfd, cmd, 5);

    /* Send lengths */
    uint32_t path_len = strlen(remote_path);
    uint32_t path_len_net  = htonl(path_len);
    uint32_t file_size_net = htonl(file_size);

    send_all(sockfd, &path_len_net, 4);
    send_all(sockfd, &file_size_net, 4);

    /* Send remote path + data */
    send_all(sockfd, remote_path, path_len);
    send_all(sockfd, file_buf, file_size);

    printf("WRITE complete: %s -> %s (%u bytes)\n",
           local_path, remote_path, file_size);

    close(sockfd);
    free(file_buf);
    return 0;
}

/*------------------------------------------------------------*/
/*                           GET                              */
/*------------------------------------------------------------*/

static int do_get(const char *remote_path, const char *maybe_local_path) {
    char local_path_buf[1024];

    const char *local_path =
        (maybe_local_path != NULL)
        ? maybe_local_path
        : (snprintf(local_path_buf, sizeof(local_path_buf),
                    "%s", basename_const(remote_path)), local_path_buf);

    int sockfd = connect_to_server();
    if (sockfd < 0) return 1;

    printf("Connected (GET)\n");

    const char cmd[5] = {'G','E','T',' ',' '};
    send_all(sockfd, cmd, 5);

    uint32_t path_len = strlen(remote_path);
    uint32_t path_len_net = htonl(path_len);

    send_all(sockfd, &path_len_net, 4);
    send_all(sockfd, remote_path, path_len);

    /* Receive status */
    uint32_t status_net;
    recv_all(sockfd, &status_net, 4);
    uint32_t status = ntohl(status_net);

    if (status != 0) {
        fprintf(stderr, "GET error: remote file not found (%s)\n", remote_path);
        close(sockfd);
        return 1;
    }

    /* Receive file size */
    uint32_t file_size_net;
    recv_all(sockfd, &file_size_net, 4);
    uint32_t file_size = ntohl(file_size_net);

    uint8_t *buf = malloc(file_size);
    recv_all(sockfd, buf, file_size);

    FILE *fp = fopen(local_path, "wb");
    if (!fp) {
        perror("fopen local");
        free(buf);
        close(sockfd);
        return 1;
    }

    fwrite(buf, 1, file_size, fp);
    fclose(fp);

    printf("GET complete: %s -> %s (%u bytes)\n",
           remote_path, local_path, file_size);

    free(buf);
    close(sockfd);
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
    send_all(sockfd, cmd, 5);

    uint32_t path_len = strlen(remote_path);
    uint32_t path_len_net = htonl(path_len);

    send_all(sockfd, &path_len_net, 4);
    send_all(sockfd, remote_path, path_len);

    /* Receive status */
    uint32_t status_net;
    recv_all(sockfd, &status_net, 4);
    uint32_t status = ntohl(status_net);

    close(sockfd);

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

    /* Send LS command */
    const char cmd[5] = {'L','S',' ',' ',' '};
    send_all(sockfd, cmd, 5);

    uint32_t path_len = strlen(remote_path);
    uint32_t path_len_net = htonl(path_len);

    send_all(sockfd, &path_len_net, 4);
    send_all(sockfd, remote_path, path_len);

    /* Receive count */
    uint32_t count_net;
    recv_all(sockfd, &count_net, 4);
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

        recv_all(sockfd, &name_len_net, 4);
        recv_all(sockfd, &ts_len_net, 4);

        uint32_t name_len = ntohl(name_len_net);
        uint32_t ts_len   = ntohl(ts_len_net);

        char *name_buf = malloc(name_len + 1);
        char *ts_buf   = malloc(ts_len + 1);

        recv_all(sockfd, name_buf, name_len);
        recv_all(sockfd, ts_buf, ts_len);

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
            "  %s GET   remote-path [local-path]\n"
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
        if (argc < 3) {
            fprintf(stderr, "Usage: %s GET remote-path [local-path]\n", argv[0]);
            return 1;
        }
        const char *remote_path = argv[2];
        const char *local_path  = (argc >= 4) ? argv[3] : NULL;
        return do_get(remote_path, local_path);

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
