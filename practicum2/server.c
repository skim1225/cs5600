/*
 * server.c -- RFS server with:
 *   - Multi-threaded client support
 *   - WRITE with versioning
 *   - GET returning newest version
 *   - RM removing all versions
 *   - LS listing all versions + timestamps
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
#include <time.h>

#define SERVER_PORT 2000
#define SERVER_ROOT "./rfs_root"

static pthread_mutex_t fs_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Receive exactly len bytes */
static int recv_all(int sockfd, void *buf, size_t len)
{
    uint8_t *p = buf;
    size_t total = 0;
    while (total < len)
    {
        ssize_t n = recv(sockfd, p + total, len - total, 0);
        if (n < 0) { perror("recv"); return -1; }
        if (n == 0) {
            fprintf(stderr, "recv_all: connection closed\n");
            return -1;
        }
        total += n;
    }
    return 0;
}

/* Send exactly len bytes */
static int send_all(int sockfd, const void *buf, size_t len)
{
    const uint8_t *p = buf;
    size_t total = 0;
    while (total < len)
    {
        ssize_t n = send(sockfd, p + total, len - total, 0);
        if (n < 0) { perror("send"); return -1; }
        if (n == 0) {
            fprintf(stderr, "send_all: connection closed\n");
            return -1;
        }
        total += n;
    }
    return 0;
}

/* Ensure directory structure exists for full_path */
static int ensure_directories(const char *full_path)
{
    char tmp[1024];
    size_t len = strlen(full_path);

    if (len >= sizeof(tmp)) return -1;
    strcpy(tmp, full_path);

    /* create rfs_root if missing */
    if (mkdir(SERVER_ROOT, 0755) < 0 && errno != EEXIST)
        return -1;

    /* create intermediate directories */
    for (size_t i = strlen(SERVER_ROOT) + 1; i < len; i++)
    {
        if (tmp[i] == '/')
        {
            tmp[i] = '\0';
            if (mkdir(tmp, 0755) < 0 && errno != EEXIST)
                return -1;
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
        close(client_sock);
        return NULL;
    }

    /*------------------------------------------------------------*/
    /*                         WRITE (versioning)                 */
    /*------------------------------------------------------------*/
    if (memcmp(cmd, "WRITE", 5) == 0)
    {
        uint32_t path_len_net, file_size_net;
        if (recv_all(client_sock, &path_len_net, 4) < 0 ||
            recv_all(client_sock, &file_size_net, 4) < 0)
        {
            close(client_sock);
            return NULL;
        }

        uint32_t path_len  = ntohl(path_len_net);
        uint32_t file_size = ntohl(file_size_net);

        char *remote_path = malloc(path_len + 1);
        uint8_t *file_buf = malloc(file_size);

        if (!remote_path || !file_buf)
        {
            perror("malloc");
            free(remote_path);
            free(file_buf);
            close(client_sock);
            return NULL;
        }

        if (recv_all(client_sock, remote_path, path_len) < 0 ||
            recv_all(client_sock, file_buf, file_size) < 0)
        {
            free(remote_path);
            free(file_buf);
            close(client_sock);
            return NULL;
        }
        remote_path[path_len] = '\0';

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", SERVER_ROOT, remote_path);

        printf("WRITE: %s (%u bytes)\n", full_path, file_size);

        pthread_mutex_lock(&fs_mutex);

        if (ensure_directories(full_path) < 0)
        {
            pthread_mutex_unlock(&fs_mutex);
            free(remote_path);
            free(file_buf);
            close(client_sock);
            return NULL;
        }

        /* --- versioning: if file exists, rename to .vN --- */
        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode))
        {
            int version = 1;
            while (1)
            {
                char version_path[1024];
                snprintf(version_path, sizeof(version_path),
                         "%s.v%d", full_path, version);

                struct stat vst;
                if (stat(version_path, &vst) < 0)
                {
                    if (errno == ENOENT)
                    {
                        rename(full_path, version_path);
                        printf("Saved previous version as %s\n", version_path);
                        break;
                    }
                }
                version++;
            }
        }

        /* --- write newest version --- */
        FILE *fp = fopen(full_path, "wb");
        if (!fp)
        {
            perror("fopen");
            pthread_mutex_unlock(&fs_mutex);
            free(remote_path);
            free(file_buf);
            close(client_sock);
            return NULL;
        }
        fwrite(file_buf, 1, file_size, fp);
        fclose(fp);

        pthread_mutex_unlock(&fs_mutex);

        free(remote_path);
        free(file_buf);
        close(client_sock);
        return NULL;
    }

    /*------------------------------------------------------------*/
    /*                              GET                            */
    /*------------------------------------------------------------*/
    if (memcmp(cmd, "GET  ", 5) == 0)
    {
        uint32_t path_len_net;
        if (recv_all(client_sock, &path_len_net, 4) < 0)
        {
            close(client_sock);
            return NULL;
        }
        uint32_t path_len = ntohl(path_len_net);

        char *remote_path = malloc(path_len + 1);
        if (!remote_path)
        {
            close(client_sock);
            return NULL;
        }

        if (recv_all(client_sock, remote_path, path_len) < 0)
        {
            free(remote_path);
            close(client_sock);
            return NULL;
        }
        remote_path[path_len] = '\0';

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", SERVER_ROOT, remote_path);

        printf("GET: %s\n", full_path);

        pthread_mutex_lock(&fs_mutex);

        FILE *fp = fopen(full_path, "rb");
        if (!fp)
        {
            uint32_t status = htonl(1);
            send_all(client_sock, &status, 4);
            pthread_mutex_unlock(&fs_mutex);
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        rewind(fp);

        uint8_t *buf = malloc(fsize);
        if (!buf)
        {
            fclose(fp);
            pthread_mutex_unlock(&fs_mutex);
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        fread(buf, 1, fsize, fp);
        fclose(fp);

        pthread_mutex_unlock(&fs_mutex);

        uint32_t status = htonl(0);
        send_all(client_sock, &status, 4);

        uint32_t fsize_net = htonl((uint32_t)fsize);
        send_all(client_sock, &fsize_net, 4);

        send_all(client_sock, buf, (size_t)fsize);

        free(remote_path);
        free(buf);
        close(client_sock);
        return NULL;
    }

    /*------------------------------------------------------------*/
    /*                        LS (list versions)                  */
    /*------------------------------------------------------------*/
    if (memcmp(cmd, "LS   ", 5) == 0)
    {
        uint32_t path_len_net;
        if (recv_all(client_sock, &path_len_net, 4) < 0)
        {
            close(client_sock);
            return NULL;
        }
        uint32_t path_len = ntohl(path_len_net);

        char *remote_path = malloc(path_len + 1);
        if (!remote_path)
        {
            close(client_sock);
            return NULL;
        }

        if (recv_all(client_sock, remote_path, path_len) < 0)
        {
            free(remote_path);
            close(client_sock);
            return NULL;
        }
        remote_path[path_len] = '\0';

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", SERVER_ROOT, remote_path);

        printf("LS: %s\n", full_path);

        pthread_mutex_lock(&fs_mutex);

        uint32_t count = 0;
        struct stat st;

        /* Base file (current version) */
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode))
            count++;

        /* Versioned files: file.v1, file.v2, ... */
        int version = 1;
        while (1)
        {
            char v_full_path[1024];
            snprintf(v_full_path, sizeof(v_full_path), "%s.v%d", full_path, version);

            struct stat vst;
            if (stat(v_full_path, &vst) < 0)
            {
                if (errno == ENOENT)
                    break;
                else
                    break;
            }

            if (S_ISREG(vst.st_mode))
                count++;

            version++;
        }

        uint32_t count_net = htonl(count);
        if (send_all(client_sock, &count_net, 4) < 0)
        {
            pthread_mutex_unlock(&fs_mutex);
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        if (count == 0)
        {
            pthread_mutex_unlock(&fs_mutex);
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        /* Send base file info, if it exists */
        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode))
        {
            char name_buf[1024];
            char ts_buf[64];
            struct tm tm_buf;

            snprintf(name_buf, sizeof(name_buf), "%s", remote_path);
            localtime_r(&st.st_mtime, &tm_buf);
            strftime(ts_buf, sizeof(ts_buf), "%Y-%m-%d %H:%M:%S", &tm_buf);

            uint32_t name_len = (uint32_t)strlen(name_buf);
            uint32_t ts_len   = (uint32_t)strlen(ts_buf);
            uint32_t name_len_net = htonl(name_len);
            uint32_t ts_len_net   = htonl(ts_len);

            if (send_all(client_sock, &name_len_net, 4) < 0 ||
                send_all(client_sock, &ts_len_net, 4) < 0 ||
                send_all(client_sock, name_buf, name_len) < 0 ||
                send_all(client_sock, ts_buf, ts_len) < 0)
            {
                pthread_mutex_unlock(&fs_mutex);
                free(remote_path);
                close(client_sock);
                return NULL;
            }
        }

        /* Send each version file info */
        version = 1;
        while (1)
        {
            char v_full_path[1024];
            snprintf(v_full_path, sizeof(v_full_path), "%s.v%d", full_path, version);

            struct stat vst;
            if (stat(v_full_path, &vst) < 0)
            {
                if (errno == ENOENT)
                    break;
                else
                    break;
            }

            if (S_ISREG(vst.st_mode))
            {
                char name_buf[1024];
                char ts_buf[64];
                struct tm tm_buf;

                snprintf(name_buf, sizeof(name_buf), "%s.v%d", remote_path, version);
                localtime_r(&vst.st_mtime, &tm_buf);
                strftime(ts_buf, sizeof(ts_buf), "%Y-%m-%d %H:%M:%S", &tm_buf);

                uint32_t name_len = (uint32_t)strlen(name_buf);
                uint32_t ts_len   = (uint32_t)strlen(ts_buf);
                uint32_t name_len_net = htonl(name_len);
                uint32_t ts_len_net   = htonl(ts_len);

                if (send_all(client_sock, &name_len_net, 4) < 0 ||
                    send_all(client_sock, &ts_len_net, 4) < 0 ||
                    send_all(client_sock, name_buf, name_len) < 0 ||
                    send_all(client_sock, ts_buf, ts_len) < 0)
                {
                    pthread_mutex_unlock(&fs_mutex);
                    free(remote_path);
                    close(client_sock);
                    return NULL;
                }
            }

            version++;
        }

        pthread_mutex_unlock(&fs_mutex);
        free(remote_path);
        close(client_sock);
        return NULL;
    }

    /*------------------------------------------------------------*/
    /*                        RM (remove + versions)               */
    /*------------------------------------------------------------*/
    if (memcmp(cmd, "RM   ", 5) == 0)
    {
        uint32_t path_len_net;
        recv_all(client_sock, &path_len_net, 4);
        uint32_t path_len = ntohl(path_len_net);

        char *remote_path = malloc(path_len + 1);
        recv_all(client_sock, remote_path, path_len);
        remote_path[path_len] = '\0';

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", SERVER_ROOT, remote_path);

        printf("RM: %s\n", full_path);

        uint32_t status = 0;

        pthread_mutex_lock(&fs_mutex);

        struct stat st;
        if (stat(full_path, &st) < 0)
        {
            if (errno == ENOENT) status = 1;   /* not found */
            else status = 5;
        }
        else if (S_ISDIR(st.st_mode))
        {
            if (rmdir(full_path) < 0)
            {
                if (errno == ENOTEMPTY) status = 2;  /* dir not empty */
                else status = 3;
            }
        }
        else
        {
            /* delete base file */
            if (unlink(full_path) < 0)
                status = 4;
            else
                printf("Removed %s\n", full_path);

            /* delete version files: file.v1, file.v2, ... */
            int version = 1;
            while (1)
            {
                char version_path[1024];
                snprintf(version_path, sizeof(version_path),
                         "%s.v%d", full_path, version);

                struct stat vst;
                if (stat(version_path, &vst) < 0)
                {
                    if (errno == ENOENT) break;
                    status = 4;
                    break;
                }

                if (unlink(version_path) == 0)
                    printf("Removed %s\n", version_path);

                version++;
            }
        }

        pthread_mutex_unlock(&fs_mutex);

        uint32_t net = htonl(status);
        send_all(client_sock, &net, 4);

        free(remote_path);
        close(client_sock);
        return NULL;
    }

    close(client_sock);
    return NULL;
}

int main(void)
{
    mkdir(SERVER_ROOT, 0755);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(SERVER_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(sock, 16);

    printf("Server running at port %d\n", SERVER_PORT);

    while (1)
    {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);

        int client = accept(sock, (struct sockaddr *)&caddr, &clen);

        int *arg = malloc(sizeof(int));
        if (!arg) {
            perror("malloc");
            close(client);
            continue;
        }
        *arg = client;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, arg) != 0)
        {
            perror("pthread_create");
            free(arg);
            close(client);
            continue;
        }
        pthread_detach(tid);
    }
}
