/*
 * server.c -- RFS server with:
 *   - Multi-threaded client support
 *   - WRITE with versioning
 *   - GET returning newest version (or specific version via path)
 *   - RM removing all versions
 *   - LS listing all versions + timestamps
 *   - STOP shutting down the server
 *
 * Sooji Kim | CS5600 | Northeastern University
 * Fall 2025 | Dec 6 2025
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

#include "server.h"

static pthread_mutex_t fs_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int server_running = 1;
static int listen_sock = -1;

/**
 * @brief Receive exactly @p len bytes from a socket.
 *
 * This helper repeatedly calls recv(2) until either @p len bytes
 * have been read into @p buf from @p sockfd or an error/connection
 * close is encountered.
 *
 * @param sockfd Connected client socket file descriptor.
 * @param buf Destination buffer to fill with received bytes.
 * @param len Number of bytes that must be received.
 *
 * @return 0 on success (all bytes received), or -1 on error or if
 *         the connection is closed prematurely.
 */
int recv_all(int sockfd, void *buf, size_t len)
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
            fprintf(stderr, "recv_all: connection closed\n");
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

/**
 * @brief Send exactly @p len bytes on a socket.
 *
 * This helper repeatedly calls send(2) until either @p len bytes
 * from @p buf have been written to @p sockfd or an error/connection
 * close is encountered.
 *
 * @param sockfd Connected client socket file descriptor.
 * @param buf Source buffer containing data to send.
 * @param len Number of bytes that must be sent.
 *
 * @return 0 on success (all bytes sent), or -1 on error or if
 *         the connection is closed prematurely.
 */
int send_all(int sockfd, const void *buf, size_t len)
{
    const uint8_t *p = (const uint8_t *)buf;
    size_t total = 0;
    while (total < len)
    {
        ssize_t n = send(sockfd, p + total, len - total, 0);
        if (n < 0)
        {
            perror("send");
            return -1;
        }
        if (n == 0)
        {
            fprintf(stderr, "send_all: connection closed\n");
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

/**
 * @brief Ensure that all directories in a given path exist.
 *
 * Creates SERVER_ROOT if missing, then walks the path in
 * @p full_path and creates intermediate directories as needed.
 * The function assumes @p full_path begins with SERVER_ROOT.
 *
 * @param full_path Full path including SERVER_ROOT and the
 *                  eventual file name.
 *
 * @return 0 on success, or -1 on any error (e.g., mkdir failure
 *         other than EEXIST, or path too long).
 */
int ensure_directories(const char *full_path)
{
    char tmp[1024];
    size_t len = strlen(full_path);

    if (len >= sizeof(tmp))
        return -1;

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

/**
 * @brief Thread entry point for handling a single client connection.
 *
 * This function processes one client request per connection. It reads
 * a 5-byte command and executes one of:
 *  - WRITE: store file with versioning (.vN) under SERVER_ROOT
 *  - GET:   return requested file contents
 *  - LS:    list all versions and timestamps for a path
 *  - RM:    remove a file and all of its versions, or remove a directory
 *  - STOP:  shut down the server (sets @c server_running to 0)
 *
 * Concurrency control over the underlying file system is provided by
 * the global @c fs_mutex.
 *
 * @param arg Pointer to a dynamically allocated integer holding the
 *            client socket file descriptor. This pointer is freed
 *            inside the function.
 *
 * @return Always returns NULL (for pthreads API).
 */
void *handle_client(void *arg)
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

        char *remote_path = (char *)malloc(path_len + 1);
        uint8_t *file_buf = (uint8_t *)malloc(file_size);

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
                        if (rename(full_path, version_path) == 0)
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

        char *remote_path = (char *)malloc(path_len + 1);
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
        snprintf(full_path, sizeof(full_path), "%s/%s",
                 SERVER_ROOT, remote_path);

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

        if (fseek(fp, 0, SEEK_END) != 0)
        {
            uint32_t status = htonl(2);
            send_all(client_sock, &status, 4);
            fclose(fp);
            pthread_mutex_unlock(&fs_mutex);
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        long fsize = ftell(fp);
        if (fsize < 0 || fsize > (long)UINT32_MAX)
        {
            uint32_t status = htonl(3);
            send_all(client_sock, &status, 4);
            fclose(fp);
            pthread_mutex_unlock(&fs_mutex);
            free(remote_path);
            close(client_sock);
            return NULL;
        }
        rewind(fp);

        uint8_t *buf = (uint8_t *)malloc((size_t)fsize);
        if (!buf)
        {
            uint32_t status = htonl(4);
            send_all(client_sock, &status, 4);
            fclose(fp);
            pthread_mutex_unlock(&fs_mutex);
            free(remote_path);
            close(client_sock);
            return NULL;
        }

        size_t read_bytes = fread(buf, 1, (size_t)fsize, fp);
        fclose(fp);
        pthread_mutex_unlock(&fs_mutex);

        if (read_bytes != (size_t)fsize)
        {
            uint32_t status = htonl(5);
            send_all(client_sock, &status, 4);
            free(remote_path);
            free(buf);
            close(client_sock);
            return NULL;
        }

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

        char *remote_path = (char *)malloc(path_len + 1);
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
            snprintf(v_full_path, sizeof(v_full_path), "%s.v%d",
                     full_path, version);

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

                snprintf(name_buf, sizeof(name_buf),
                         "%s.v%d", remote_path, version);
                localtime_r(&vst.st_mtime, &tm_buf);
                strftime(ts_buf, sizeof(ts_buf),
                         "%Y-%m-%d %H:%M:%S", &tm_buf);

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
    /*                        RM (remove + versions)              */
    /*------------------------------------------------------------*/
    if (memcmp(cmd, "RM   ", 5) == 0)
    {
        uint32_t path_len_net;
        if (recv_all(client_sock, &path_len_net, 4) < 0)
        {
            close(client_sock);
            return NULL;
        }
        uint32_t path_len = ntohl(path_len_net);

        char *remote_path = (char *)malloc(path_len + 1);
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
        snprintf(full_path, sizeof(full_path), "%s/%s",
                 SERVER_ROOT, remote_path);

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

    /*------------------------------------------------------------*/
    /*                        STOP (shutdown)                     */
    /*------------------------------------------------------------*/
    if (memcmp(cmd, "STOP ", 5) == 0)
    {
        printf("STOP command received — shutting down server.\n");

        server_running = 0;

        if (listen_sock >= 0)
        {
            close(listen_sock);
            listen_sock = -1;
        }

        uint32_t status = htonl(0);
        send_all(client_sock, &status, 4);

        close(client_sock);
        return NULL;
    }

    fprintf(stderr, "Unknown command received\n");
    close(client_sock);
    return NULL;
}

/**
 * @brief Entry point for the RFS server.
 *
 * Initializes the server root directory, creates a listening socket,
 * binds and listens on SERVER_PORT, and then enters an accept loop
 * while @c server_running is non-zero.
 *
 * For each accepted client connection, a detached thread is spawned
 * running handle_client(), which processes exactly one command per
 * connection.
 *
 * The STOP command causes @c server_running to be set to 0 and the
 * listening socket to be closed, allowing the main loop to exit
 * cleanly.
 *
 * @return 0 on normal shutdown, or 1 if a critical socket, bind, or
 *         listen error occurs at startup.
 */
int main(void)
{
    mkdir(SERVER_ROOT, 0755);

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(SERVER_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(listen_sock);
        return 1;
    }

    if (listen(listen_sock, 16) < 0)
    {
        perror("listen");
        close(listen_sock);
        return 1;
    }

    printf("Server running at port %d\n", SERVER_PORT);

    while (server_running)
    {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);

        int client = accept(listen_sock, (struct sockaddr *)&caddr, &clen);
        if (!server_running)
            break;

        if (client < 0)
        {
            if (!server_running)
                break;
            perror("accept");
            continue;
        }

        int *arg = (int *)malloc(sizeof(int));
        if (!arg)
        {
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

    if (listen_sock >= 0)
        close(listen_sock);

    printf("Server shutting down.\n");
    return 0;
}
