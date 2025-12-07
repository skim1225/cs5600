/*
 * server.c -- RFS server with WRITE (versioning), GET, and RM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SERVER_PORT 2000
#define SERVER_ROOT "./rfs_root"

static pthread_mutex_t fs_mutex = PTHREAD_MUTEX_INITIALIZER;

static int recv_all(int sockfd, void *buf, size_t len)
{
    uint8_t *p = (uint8_t *)buf;
    size_t total = 0;

    while (total < len)
    {
        ssize_t n = recv(sockfd, p + total, len - total, 0);
        if (n < 0)
        {
            perror("recv");
            return -1;
        }
        if (n == 0)
        {
            fprintf(stderr, "recv_all: connection closed unexpectedly\n");
            return -1;
        }
        total += (size_t)n;
    }

    return 0;
}

/* Ensure SERVER_ROOT and subdirs exist for full_path */
static int ensure_directories(const char *full_path)
{
    char tmp[1024];
    size_t len = strlen(full_path);

    if (len >= sizeof(tmp))
    {
        fprintf(stderr, "Path too long\n");
        return -1;
    }

    strcpy(tmp, full_path);

    if (mkdir(SERVER_ROOT, 0755) < 0 && errno != EEXIST)
    {
        perror("mkdir SERVER_ROOT");
        return -1;
    }

    for (size_t i = strlen(SERVER_ROOT) + 1; i < len; i++)
    {
        if (tmp[i] == '/')
        {
            tmp[i] = '\0';
            if (mkdir(tmp, 0755) < 0)
            {
                if (errno != EEXIST)
                {
                    perror("mkdir");
                    return -1;
                }
            }
            tmp[i] = '/';
        }
    }

    return 0;
}

static void *handle_client(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);

    char cmd[5];

    if (recv_all(client_sock, cmd, 5) < 0)
    {
        fprintf(stderr, "Failed to read command\n");
        close(client_sock);
        return NULL;
    }

    /* ---------- WRITE (now with versioning) ---------- */
    if (memcmp(cmd, "WRITE", 5) == 0)
    {
        uint32_t path_len_net;
        uint32_t file_size_net;

        if (recv_all(client_sock, &path_len_net, sizeof(path_len_net)) < 0)
        {
            fprintf(stderr, "Failed to read path_len\n");
            close(client_sock);
            return NULL;
        }
        if (recv_all(client_sock, &file_size_net, sizeof(file_size_net)) < 0)
        {
            fprintf(stderr, "Failed to read file_size\n");
            close(client_sock);
            return NULL;
        }

        uint32_t path_len = ntohl(path_len_net);
        uint32_t file_size = ntohl(file_size_net);

        if (path_len == 0 || path_len > 1000)
        {
            fprintf(stderr, "Invalid path_len: %u\n", path_len);
            close(client_sock);
            return NULL;
        }

        char *remote_path = malloc(path_len + 1);
        if (remote_path == NULL)
        {
            perror("malloc remote_path");
            close(client_sock);
            return NULL;
        }

        if (recv_all(client_sock, remote_path, path_len) < 0)
        {
            fprintf(stderr, "Failed to read remote_path\n");
            free(remote_path);
            close(client_sock);
            return NULL;
        }
        remote_path[path_len] = '\0';

        uint8_t *file_buf = malloc(file_size);
        if (file_buf == NULL)
        {
            perror("malloc file_buf");
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        if (recv_all(client_sock, file_buf, file_size) < 0)
        {
            fprintf(stderr, "Failed to read file data\n");
            free(remote_path);
            free(file_buf);
            close(client_sock);
            return NULL;
        }

        char full_path[1024];
        if (snprintf(full_path, sizeof(full_path), "%s/%s",
                     SERVER_ROOT, remote_path) >= (int)sizeof(full_path))
        {
            fprintf(stderr, "Full path too long\n");
            free(remote_path);
            free(file_buf);
            close(client_sock);
            return NULL;
        }

        printf("WRITE: '%s' (%u bytes)\n", full_path, file_size);

        pthread_mutex_lock(&fs_mutex);

        /* Ensure directory structure exists */
        if (ensure_directories(full_path) < 0)
        {
            fprintf(stderr, "Failed to ensure directories\n");
            pthread_mutex_unlock(&fs_mutex);
            free(remote_path);
            free(file_buf);
            close(client_sock);
            return NULL;
        }

        /* ---- Versioning logic: if file exists, rename it to .vN ---- */
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode))
        {
            int version = 1;
            char version_path[1024];

            for (;;)
            {
                if (snprintf(version_path, sizeof(version_path),
                             "%s.v%d", full_path, version) >= (int)sizeof(version_path))
                {
                    fprintf(stderr, "Version path too long for '%s'\n", full_path);
                    break;
                }

                struct stat vst;
                if (stat(version_path, &vst) != 0)
                {
                    /* ENOENT means this version file does not exist yet */
                    if (errno == ENOENT)
                    {
                        if (rename(full_path, version_path) < 0)
                        {
                            perror("rename old version");
                        }
                        else
                        {
                            printf("Saved previous version as '%s'\n", version_path);
                        }
                        break;
                    }
                    else
                    {
                        perror("stat version_path");
                        break;
                    }
                }

                version++;
            }
        }

        /* Now write new data to full_path (current version) */
        FILE *fp = fopen(full_path, "wb");
        if (fp == NULL)
        {
            perror("fopen full_path");
            pthread_mutex_unlock(&fs_mutex);
            free(remote_path);
            free(file_buf);
            close(client_sock);
            return NULL;
        }

        size_t written = fwrite(file_buf, 1, file_size, fp);
        fclose(fp);
        pthread_mutex_unlock(&fs_mutex);

        if (written != file_size)
        {
            fprintf(stderr, "Short write to '%s'\n", full_path);
        }
        else
        {
            printf("Saved file to '%s'\n", full_path);
        }

        free(remote_path);
        free(file_buf);
        close(client_sock);
        return NULL;
    }

    /* ---------- GET ---------- */
    if (memcmp(cmd, "GET  ", 5) == 0)
    {
        uint32_t path_len_net;
        if (recv_all(client_sock, &path_len_net, sizeof(path_len_net)) < 0)
        {
            fprintf(stderr, "Failed to read path_len (GET)\n");
            close(client_sock);
            return NULL;
        }
        uint32_t path_len = ntohl(path_len_net);

        if (path_len == 0 || path_len > 1000)
        {
        fprintf(stderr, "Invalid path_len in GET: %u\n", path_len);
            close(client_sock);
            return NULL;
        }

        char *remote_path = malloc(path_len + 1);
        if (remote_path == NULL)
        {
            perror("malloc remote_path");
            close(client_sock);
            return NULL;
        }

        if (recv_all(client_sock, remote_path, path_len) < 0)
        {
            fprintf(stderr, "Failed to read remote_path (GET)\n");
            free(remote_path);
            close(client_sock);
            return NULL;
        }
        remote_path[path_len] = '\0';

        char full_path[1024];
        if (snprintf(full_path, sizeof(full_path), "%s/%s",
                     SERVER_ROOT, remote_path) >= (int)sizeof(full_path))
        {
            fprintf(stderr, "Full path too long\n");
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        printf("GET: '%s'\n", full_path);

        pthread_mutex_lock(&fs_mutex);
        FILE *fp = fopen(full_path, "rb");
        if (fp == NULL)
        {
            uint32_t status_net = htonl(1);
            send(client_sock, &status_net, sizeof(status_net), 0);
            pthread_mutex_unlock(&fs_mutex);
            perror("fopen full_path");
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        if (fseek(fp, 0, SEEK_END) != 0)
        {
            fclose(fp);
            pthread_mutex_unlock(&fs_mutex);
            uint32_t status_net = htonl(2);
            send(client_sock, &status_net, sizeof(status_net), 0);
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        long file_size_long = ftell(fp);
        if (file_size_long < 0 || file_size_long > (long)UINT32_MAX)
        {
            fclose(fp);
            pthread_mutex_unlock(&fs_mutex);
            uint32_t status_net = htonl(3);
            send(client_sock, &status_net, sizeof(status_net), 0);
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        uint32_t file_size = (uint32_t)file_size_long;
        rewind(fp);

        uint8_t *buf = malloc(file_size);
        if (buf == NULL)
        {
            fclose(fp);
            pthread_mutex_unlock(&fs_mutex);
            uint32_t status_net = htonl(4);
            send(client_sock, &status_net, sizeof(status_net), 0);
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        size_t read_bytes = fread(buf, 1, file_size, fp);
        fclose(fp);
        pthread_mutex_unlock(&fs_mutex);

        if (read_bytes != file_size)
        {
            uint32_t status_net = htonl(5);
            send(client_sock, &status_net, sizeof(status_net), 0);
            fprintf(stderr, "Short read of '%s'\n", full_path);
            free(remote_path);
            free(buf);
            close(client_sock);
            return NULL;
        }

        uint32_t status_net = htonl(0);
        send(client_sock, &status_net, sizeof(status_net), 0);

        uint32_t file_size_net = htonl(file_size);
        send(client_sock, &file_size_net, sizeof(file_size_net), 0);

        size_t total_sent = 0;
        while (total_sent < file_size)
        {
            ssize_t n = send(client_sock,
                             buf + total_sent,
                             file_size - total_sent,
                             0);
            if (n <= 0)
            {
                perror("send file data");
                break;
            }
            total_sent += (size_t)n;
        }

        printf("Sent %u bytes for '%s'\n", file_size, full_path);

        free(remote_path);
        free(buf);
        close(client_sock);
        return NULL;
    }

    /* ---------- RM (files + empty dirs) ---------- */
    if (memcmp(cmd, "RM   ", 5) == 0)
    {
        uint32_t path_len_net;
        if (recv_all(client_sock, &path_len_net, sizeof(path_len_net)) < 0)
        {
            fprintf(stderr, "Failed to read path_len (RM)\n");
            close(client_sock);
            return NULL;
        }
        uint32_t path_len = ntohl(path_len_net);

        if (path_len == 0 || path_len > 1000)
        {
            fprintf(stderr, "Invalid path_len in RM: %u\n", path_len);
            close(client_sock);
            return NULL;
        }

        char *remote_path = malloc(path_len + 1);
        if (remote_path == NULL)
        {
            perror("malloc remote_path");
            close(client_sock);
            return NULL;
        }

        if (recv_all(client_sock, remote_path, path_len) < 0)
        {
            fprintf(stderr, "Failed to read remote_path (RM)\n");
            free(remote_path);
            close(client_sock);
            return NULL;
        }
        remote_path[path_len] = '\0';

        char full_path[1024];
        if (snprintf(full_path, sizeof(full_path), "%s/%s",
                     SERVER_ROOT, remote_path) >= (int)sizeof(full_path))
        {
            fprintf(stderr, "Full path too long\n");
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        printf("RM: '%s'\n", full_path);

        uint32_t status = 0;

        pthread_mutex_lock(&fs_mutex);

        struct stat st;
        if (stat(full_path, &st) < 0)
        {
            if (errno == ENOENT)
            {
                status = 1;  /* not found */
            }
            else
            {
                status = 5;  /* generic stat error */
            }
        }
        else if (S_ISDIR(st.st_mode))
        {
            if (rmdir(full_path) < 0)
            {
                if (errno == ENOTEMPTY || errno == EEXIST)
                {
                    status = 2;  /* directory not empty */
                }
                else
                {
                    status = 3;  /* other directory error */
                }
            }
        }
        else
        {
            if (unlink(full_path) < 0)
            {
                if (errno == ENOENT)
                {
                    status = 1;  /* not found */
                }
                else
                {
                    status = 4;  /* other file error */
                }
            }
        }

        pthread_mutex_unlock(&fs_mutex);

        uint32_t status_net = htonl(status);
        send(client_sock, &status_net, sizeof(status_net), 0);

        free(remote_path);
        close(client_sock);
        return NULL;
    }

    fprintf(stderr, "Unknown command from client\n");
    close(client_sock);
    return NULL;
}

int main(void)
{
    if (mkdir(SERVER_ROOT, 0755) < 0 && errno != EEXIST)
    {
        perror("mkdir SERVER_ROOT");
        return 1;
    }

    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0)
    {
        perror("socket");
        return 1;
    }

    int optval = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_desc, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(socket_desc);
        return 1;
    }

    if (listen(socket_desc, 16) < 0)
    {
        perror("listen");
        close(socket_desc);
        return 1;
    }

    printf("RFS server listening on port %d, root: %s\n",
           SERVER_PORT, SERVER_ROOT);

    for (;;)
    {
        struct sockaddr_in client_addr;
        socklen_t client_size = sizeof(client_addr);

        int client_sock = accept(socket_desc,
                                 (struct sockaddr *)&client_addr,
                                 &client_size);
        if (client_sock < 0)
        {
            perror("accept");
            continue;
        }

        printf("Client connected from %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        int *arg = malloc(sizeof(int));
        if (arg == NULL)
        {
            perror("malloc");
            close(client_sock);
            continue;
        }
        *arg = client_sock;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, arg) != 0)
        {
            perror("pthread_create");
            free(arg);
            close(client_sock);
            continue;
        }
        pthread_detach(tid);
    }

    close(socket_desc);
    return 0;
}
