/*
 * rfs.c -- Remote File System client
 *
 * Usage:
 *   ./rfs WRITE local-path [remote-path]
 *   ./rfs GET   remote-path [local-path]
 *
 * For WRITE:
 *   - If remote-path is omitted, it defaults to local-path.
 *
 * For GET:
 *   - If local-path is omitted, the file is written in the current
 *     directory using the basename of remote-path.
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

/* Get basename of a path (after last '/') */
static const char *basename_const(const char *path) {
    const char *slash = strrchr(path, '/');
    if (slash == NULL) {
        return path;
    }
    return slash + 1;
}

/* Connect to SERVER_IP:SERVER_PORT */
static int connect_to_server(void) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

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

/* Handle WRITE command */
static int do_write(const char *local_path, const char *remote_path) {
    /* Open local file */
    FILE *fp = fopen(local_path, "rb");
    if (!fp) {
        perror("fopen local file");
        return 1;
    }

    /* Get file size */
    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(fp);
        return 1;
    }

    long file_size_long = ftell(fp);
    if (file_size_long < 0) {
        perror("ftell");
        fclose(fp);
        return 1;
    }
    if (file_size_long > (long)UINT32_MAX) {
        fprintf(stderr, "File too large\n");
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
        fprintf(stderr, "Short read: expected %u, got %zu\n",
                file_size, read_bytes);
        free(file_buf);
        return 1;
    }

    int sockfd = connect_to_server();
    if (sockfd < 0) {
        free(file_buf);
        return 1;
    }

    printf("Connected to server %s:%d (WRITE)\n", SERVER_IP, SERVER_PORT);

    /* 1. Command "WRITE" */
    const char *cmd = "WRITE";
    if (send_all(sockfd, cmd, 5) < 0) {
        fprintf(stderr, "Failed to send WRITE command\n");
        close(sockfd);
        free(file_buf);
        return 1;
    }

    /* 2. Path length */
    uint32_t path_len = (uint32_t)strlen(remote_path);
    uint32_t path_len_net = htonl(path_len);
    if (send_all(sockfd, &path_len_net, sizeof(path_len_net)) < 0) {
        fprintf(stderr, "Failed to send path length\n");
        close(sockfd);
        free(file_buf);
        return 1;
    }

    /* 3. File size */
    uint32_t file_size_net = htonl(file_size);
    if (send_all(sockfd, &file_size_net, sizeof(file_size_net)) < 0) {
        fprintf(stderr, "Failed to send file size\n");
        close(sockfd);
        free(file_buf);
        return 1;
    }

    /* 4. Remote path */
    if (send_all(sockfd, remote_path, path_len) < 0) {
        fprintf(stderr, "Failed to send remote path\n");
        close(sockfd);
        free(file_buf);
        return 1;
    }

    /* 5. File data */
    if (send_all(sockfd, file_buf, file_size) < 0) {
        fprintf(stderr, "Failed to send file data\n");
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

/* Handle GET command */
static int do_get(const char *remote_path, const char *maybe_local_path) {
    char local_path_buf[1024];

    /* If local path omitted, use basename of remote in current directory */
    const char *local_path;
    if (maybe_local_path != NULL) {
        local_path = maybe_local_path;
    } else {
        const char *base = basename_const(remote_path);
        if (snprintf(local_path_buf, sizeof(local_path_buf), "%s", base)
            >= (int)sizeof(local_path_buf)) {
            fprintf(stderr, "Local path too long\n");
            return 1;
        }
        local_path = local_path_buf;
    }

    int sockfd = connect_to_server();
    if (sockfd < 0) {
        return 1;
    }

    printf("Connected to server %s:%d (GET)\n", SERVER_IP, SERVER_PORT);

    /* 1. Command "GET  " (5 bytes) */
    const char cmd[5] = { 'G', 'E', 'T', ' ', ' ' };
    if (send_all(sockfd, cmd, 5) < 0) {
        fprintf(stderr, "Failed to send GET command\n");
        close(sockfd);
        return 1;
    }

    /* 2. Path length */
    uint32_t path_len = (uint32_t)strlen(remote_path);
    uint32_t path_len_net = htonl(path_len);
    if (send_all(sockfd, &path_len_net, sizeof(path_len_net)) < 0) {
        fprintf(stderr, "Failed to send path length\n");
        close(sockfd);
        return 1;
    }

    /* 3. Remote path */
    if (send_all(sockfd, remote_path, path_len) < 0) {
        fprintf(stderr, "Failed to send remote path\n");
        close(sockfd);
        return 1;
    }

    /* ---- Receive response ---- */

    /* 1. Status */
    uint32_t status_net;
    if (recv_all(sockfd, &status_net, sizeof(status_net)) < 0) {
        fprintf(stderr, "Failed to receive status\n");
        close(sockfd);
        return 1;
    }
    uint32_t status = ntohl(status_net);
    if (status != 0) {
        fprintf(stderr, "Server reported error (status=%u) for '%s'\n",
                status, remote_path);
        close(sockfd);
        return 1;
    }

    /* 2. File size */
    uint32_t file_size_net;
    if (recv_all(sockfd, &file_size_net, sizeof(file_size_net)) < 0) {
        fprintf(stderr, "Failed to receive file size\n");
        close(sockfd);
        return 1;
    }
    uint32_t file_size = ntohl(file_size_net);

    uint8_t *file_buf = malloc(file_size);
    if (!file_buf) {
        perror("malloc");
        close(sockfd);
        return 1;
    }

    /* 3. File data */
    if (recv_all(sockfd, file_buf, file_size) < 0) {
        fprintf(stderr, "Failed to receive file data\n");
        free(file_buf);
        close(sockfd);
        return 1;
    }

    close(sockfd);

    /* Write to local file */
    FILE *fp = fopen(local_path, "wb");
    if (!fp) {
        perror("fopen local_path");
        free(file_buf);
        return 1;
    }

    size_t written = fwrite(file_buf, 1, file_size, fp);
    fclose(fp);
    free(file_buf);

    if (written != file_size) {
        fprintf(stderr, "Short write to '%s': expected %u, wrote %zu\n",
                local_path, file_size, written);
        return 1;
    }

    printf("GET complete: %s -> %s (%u bytes)\n",
           remote_path, local_path, file_size);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr,
            "Usage:\n"
            "  %s WRITE local-path [remote-path]\n"
            "  %s GET   remote-path [local-path]\n",
            argv[0], argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "WRITE") == 0) {
        if (argc < 3) {
            fprintf(stderr,
                "Usage: %s WRITE local-path [remote-path]\n",
                argv[0]);
            return 1;
        }
        const char *local_path  = argv[2];
        const char *remote_path = (argc >= 4) ? argv[3] : argv[2]; /* default to local */
        return do_write(local_path, remote_path);

    } else if (strcmp(cmd, "GET") == 0) {
        if (argc < 3) {
            fprintf(stderr,
                "Usage: %s GET remote-path [local-path]\n",
                argv[0]);
            return 1;
        }
        const char *remote_path = argv[2];
        const char *local_path  = (argc >= 4) ? argv[3] : NULL;    /* default to current dir */
        return do_get(remote_path, local_path);

    } else {
        fprintf(stderr, "Unknown command: %s (expected WRITE or GET)\n", cmd);
        return 1;
    }
}
