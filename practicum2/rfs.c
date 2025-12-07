/*
 * rfs.c -- Remote File System client
 *
 * Sooji Kim | CS5600 | Northeastern University
 * Fall 2025 | Dec 6 2025
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "rfs.h"

/*------------------------------------------------------------*/
/*                   Utility Functions                        */
/*------------------------------------------------------------*/

/**
 * @brief Send exactly @p len bytes over a socket.
 *
 * This function repeatedly calls send(2) until either all @p len bytes
 * from @p buf have been written to @p sockfd or an error/closed
 * connection is encountered.
 *
 * @param sockfd Connected TCP socket file descriptor.
 * @param buf Pointer to the buffer containing data to send.
 * @param len Number of bytes to send from @p buf.
 *
 * @return 0 on success (all bytes sent), or -1 on error or if the
 *         connection is closed prematurely.
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
            fprintf(stderr, "send_all: connection closed unexpectedly\n");
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

/**
 * @brief Receive exactly @p len bytes from a socket.
 *
 * This function repeatedly calls recv(2) until either @p len bytes
 * have been read into @p buf from @p sockfd or an error/closed
 * connection is encountered.
 *
 * @param sockfd Connected TCP socket file descriptor.
 * @param buf Pointer to the destination buffer to fill with data.
 * @param len Number of bytes to receive into @p buf.
 *
 * @return 0 on success (all bytes received), or -1 on error or if the
 *         connection is closed prematurely.
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
            fprintf(stderr, "recv_all: connection closed unexpectedly\n");
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

/**
 * @brief Return a pointer to the basename portion of a path string.
 *
 * This function searches for the last '/' in @p path and returns a
 * pointer to the character after it. If no '/' is found, it returns
 * @p path itself.
 *
 * The returned pointer is into the original string; the caller must
 * not attempt to free it.
 *
 * @param path Null-terminated file system path string.
 *
 * @return Pointer to the basename portion of @p path.
 */
const char *basename_const(const char *path)
{
    const char *slash = strrchr(path, '/');
    if (slash == NULL)
        return path;
    return slash + 1;
}

/**
 * @brief Establish a TCP connection to the remote file system server.
 *
 * This function creates a TCP socket, populates a sockaddr_in using
 * SERVER_IP and SERVER_PORT (from rfs.h), and calls connect(2).
 *
 * On success, the caller is responsible for closing the returned
 * socket descriptor.
 *
 * @return A connected socket file descriptor on success, or -1 on
 *         error (in which case any created socket is closed).
 */
int connect_to_server(void)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/*------------------------------------------------------------*/
/*                         WRITE                              */
/*------------------------------------------------------------*/

/**
 * @brief Implement the WRITE client command.
 *
 * Reads the contents of @p local_path into memory, connects to the
 * remote server, and sends a WRITE request containing @p remote_path
 * and the file data. The server is expected to store the file under
 * the given remote path, possibly creating a versioned file.
 *
 * @param local_path Path to the local file to be uploaded.
 * @param remote_path Remote file path under which the server should
 *                    store the uploaded file.
 *
 * @return 0 on success, or 1 on any error (I/O, allocation, or
 *         networking).
 */
int do_write(const char *local_path, const char *remote_path)
{
    FILE *fp = fopen(local_path, "rb");
    if (!fp)
    {
        perror("fopen local file");
        return 1;
    }

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        perror("fseek");
        fclose(fp);
        return 1;
    }

    long file_size_long = ftell(fp);
    if (file_size_long < 0 || file_size_long > (long)UINT32_MAX)
    {
        fprintf(stderr, "File too large or ftell error\n");
        fclose(fp);
        return 1;
    }
    uint32_t file_size = (uint32_t)file_size_long;
    rewind(fp);

    uint8_t *file_buf = (uint8_t *)malloc(file_size);
    if (!file_buf)
    {
        perror("malloc");
        fclose(fp);
        return 1;
    }

    size_t read_bytes = fread(file_buf, 1, file_size, fp);
    fclose(fp);
    if (read_bytes != file_size)
    {
        fprintf(stderr, "Short read of local file\n");
        free(file_buf);
        return 1;
    }

    int sockfd = connect_to_server();
    if (sockfd < 0)
    {
        free(file_buf);
        return 1;
    }

    printf("Connected (WRITE)\n");

    /* Send command */
    const char cmd[5] = {'W','R','I','T','E'};
    if (send_all(sockfd, cmd, 5) < 0)
    {
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
        send_all(sockfd, file_buf, file_size) < 0)
    {
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
/*      Supports: GET [-v N] remote-path [local-path]         */
/*------------------------------------------------------------*/

/**
 * @brief Implement the GET client command with optional versioning.
 *
 * Requests a file from the remote server and writes it to a local
 * file. If @p version is greater than 0, the function requests a
 * specific version by appending ".v<version>" to @p remote_path
 * when communicating with the server.
 *
 * If @p maybe_local_path is non-NULL, the received data is written
 * to that path. Otherwise, the local file name defaults to the
 * basename of the (possibly versioned) remote path.
 *
 * @param remote_path Base remote path of the file to retrieve.
 * @param maybe_local_path Optional local path to save the file; if
 *                         NULL, basename of the requested remote
 *                         path is used.
 * @param version Version number to retrieve; if > 0, a specific
 *                version is requested, otherwise the newest version
 *                is requested.
 *
 * @return 0 on success, or 1 on any error (not found, I/O, or
 *         networking).
 */
int do_get(const char *remote_path,
           const char *maybe_local_path,
           int version)
{
    char local_path_buf[1024];
    char remote_buf[1024];

    /* Effective remote path to request from server */
    const char *remote_to_send = remote_path;
    if (version > 0)
    {
        if (snprintf(remote_buf, sizeof(remote_buf),
                     "%s.v%d", remote_path, version) >= (int)sizeof(remote_buf))
        {
            fprintf(stderr, "Remote path too long\n");
            return 1;
        }
        remote_to_send = remote_buf;
    }

    /* Local path: explicit or basename of what we're requesting */
    const char *local_path;
    if (maybe_local_path != NULL)
    {
        local_path = maybe_local_path;
    }
    else
    {
        const char *base = basename_const(remote_to_send);
        if (snprintf(local_path_buf, sizeof(local_path_buf),
                     "%s", base) >= (int)sizeof(local_path_buf))
        {
            fprintf(stderr, "Local path too long\n");
            return 1;
        }
        local_path = local_path_buf;
    }

    int sockfd = connect_to_server();
    if (sockfd < 0)
        return 1;

    if (version > 0)
        printf("Connected (GET -v %d)\n", version);
    else
        printf("Connected (GET)\n");

    const char cmd[5] = {'G','E','T',' ',' '};
    if (send_all(sockfd, cmd, 5) < 0)
    {
        close(sockfd);
        return 1;
    }

    uint32_t path_len = (uint32_t)strlen(remote_to_send);
    uint32_t path_len_net = htonl(path_len);

    if (send_all(sockfd, &path_len_net, 4) < 0 ||
        send_all(sockfd, remote_to_send, path_len) < 0)
    {
        close(sockfd);
        return 1;
    }

    /* Receive status */
    uint32_t status_net;
    if (recv_all(sockfd, &status_net, 4) < 0)
    {
        close(sockfd);
        return 1;
    }
    uint32_t status = ntohl(status_net);

    if (status != 0)
    {
        fprintf(stderr, "GET error: remote file not found (%s)\n",
                remote_to_send);
        close(sockfd);
        return 1;
    }

    /* Receive file size */
    uint32_t file_size_net;
    if (recv_all(sockfd, &file_size_net, 4) < 0)
    {
        close(sockfd);
        return 1;
    }
    uint32_t file_size = ntohl(file_size_net);

    uint8_t *buf = (uint8_t *)malloc(file_size);
    if (!buf)
    {
        perror("malloc");
        close(sockfd);
        return 1;
    }

    if (recv_all(sockfd, buf, file_size) < 0)
    {
        free(buf);
        close(sockfd);
        return 1;
    }

    FILE *fp = fopen(local_path, "wb");
    if (!fp)
    {
        perror("fopen local");
        free(buf);
        close(sockfd);
        return 1;
    }

    size_t written = fwrite(buf, 1, file_size, fp);
    fclose(fp);
    free(buf);
    close(sockfd);

    if (written != file_size)
    {
        fprintf(stderr, "Short write to '%s'\n", local_path);
        return 1;
    }

    if (version > 0)
    {
        printf("GET -v %d complete: %s -> %s (%u bytes)\n",
               version, remote_to_send, local_path, file_size);
    }
    else
    {
        printf("GET complete: %s -> %s (%u bytes)\n",
               remote_to_send, local_path, file_size);
    }

    return 0;
}

/*------------------------------------------------------------*/
/*                            RM                              */
/*------------------------------------------------------------*/

/**
 * @brief Implement the RM client command.
 *
 * Sends an RM request for @p remote_path to the server and interprets
 * the returned status code. Depending on the result, it prints a
 * success message or an appropriate error description.
 *
 * @param remote_path Remote path (file or directory) to remove.
 *
 * @return 0 on success, or 1 if the removal fails (not found,
 *         directory not empty, or other server-side error).
 */
int do_rm(const char *remote_path)
{
    int sockfd = connect_to_server();
    if (sockfd < 0)
        return 1;

    printf("Connected (RM)\n");

    const char cmd[5] = {'R','M',' ',' ',' '};
    if (send_all(sockfd, cmd, 5) < 0)
    {
        close(sockfd);
        return 1;
    }

    uint32_t path_len = (uint32_t)strlen(remote_path);
    uint32_t path_len_net = htonl(path_len);

    if (send_all(sockfd, &path_len_net, 4) < 0 ||
        send_all(sockfd, remote_path, path_len) < 0)
    {
        close(sockfd);
        return 1;
    }

    /* Receive status */
    uint32_t status_net;
    if (recv_all(sockfd, &status_net, 4) < 0)
    {
        close(sockfd);
        return 1;
    }
    close(sockfd);

    uint32_t status = ntohl(status_net);

    if (status == 0)
    {
        printf("RM success: '%s' deleted\n", remote_path);
        return 0;
    }
    if (status == 1)
    {
        fprintf(stderr, "RM error: '%s' not found\n", remote_path);
    }
    else if (status == 2)
    {
        fprintf(stderr, "RM error: directory not empty: '%s'\n", remote_path);
    }
    else
    {
        fprintf(stderr, "RM error: removal failed for '%s' (status=%u)\n",
                remote_path, status);
    }
    return 1;
}

/*------------------------------------------------------------*/
/*                             LS                             */
/*------------------------------------------------------------*/

/**
 * @brief Implement the LS client command for version listing.
 *
 * Sends an LS request for @p remote_path to the server, receives a
 * count of available versions, and then iterates through each entry,
 * printing the version name and its last modified timestamp in a
 * tabular format.
 *
 * @param remote_path Remote file path whose versions should be listed.
 *
 * @return 0 on success (including the case of zero versions), or 1 on
 *         any error (networking or allocation).
 */
int do_ls(const char *remote_path)
{
    int sockfd = connect_to_server();
    if (sockfd < 0)
        return 1;

    printf("Connected (LS)\n");

    const char cmd[5] = {'L','S',' ',' ',' '};
    if (send_all(sockfd, cmd, 5) < 0)
    {
        close(sockfd);
        return 1;
    }

    uint32_t path_len = (uint32_t)strlen(remote_path);
    uint32_t path_len_net = htonl(path_len);

    if (send_all(sockfd, &path_len_net, 4) < 0 ||
        send_all(sockfd, remote_path, path_len) < 0)
    {
        close(sockfd);
        return 1;
    }

    /* Receive count */
    uint32_t count_net;
    if (recv_all(sockfd, &count_net, 4) < 0)
    {
        close(sockfd);
        return 1;
    }
    uint32_t count = ntohl(count_net);

    if (count == 0)
    {
        printf("No versions found for '%s'\n", remote_path);
        close(sockfd);
        return 0;
    }

    printf("Versions for '%s':\n", remote_path);
    printf("  %-30s  %s\n", "NAME", "LAST MODIFIED");

    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t name_len_net, ts_len_net;

        if (recv_all(sockfd, &name_len_net, 4) < 0 ||
            recv_all(sockfd, &ts_len_net, 4) < 0)
        {
            close(sockfd);
            return 1;
        }

        uint32_t name_len = ntohl(name_len_net);
        uint32_t ts_len   = ntohl(ts_len_net);

        char *name_buf = (char *)malloc(name_len + 1);
        char *ts_buf   = (char *)malloc(ts_len + 1);
        if (!name_buf || !ts_buf)
        {
            perror("malloc");
            free(name_buf);
            free(ts_buf);
            close(sockfd);
            return 1;
        }

        if (recv_all(sockfd, name_buf, name_len) < 0 ||
            recv_all(sockfd, ts_buf, ts_len) < 0)
        {
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
/*                            STOP                            */
/*------------------------------------------------------------*/

/**
 * @brief Implement the STOP client command.
 *
 * Connects to the server, sends a STOP command, and waits for a
 * status code indicating whether the server will shut down.
 *
 * @return 0 on successful communication with the server (regardless
 *         of whether the shutdown succeeds), or 1 on networking
 *         errors.
 */
int do_stop(void)
{
    int sockfd = connect_to_server();
    if (sockfd < 0)
        return 1;

    printf("Connected (STOP)\n");

    const char cmd[5] = {'S','T','O','P',' '};
    if (send_all(sockfd, cmd, 5) < 0)
    {
        close(sockfd);
        return 1;
    }

    uint32_t status_net;
    if (recv_all(sockfd, &status_net, 4) < 0)
    {
        close(sockfd);
        return 1;
    }

    uint32_t status = ntohl(status_net);
    if (status == 0)
        printf("Server is shutting down.\n");
    else
        printf("STOP command failed (status=%u).\n", status);

    close(sockfd);
    return 0;
}

/*------------------------------------------------------------*/
/*                           main()                           */
/*------------------------------------------------------------*/

/**
 * @brief Entry point for the Remote File System client.
 *
 * Parses command-line arguments and dispatches to the appropriate
 * client handler:
 *  - WRITE local-path [remote-path]
 *  - GET   [-v N] remote-path [local-path]
 *  - RM    remote-path
 *  - LS    remote-path
 *  - STOP
 *
 * On incorrect usage or unknown commands, a usage message is printed
 * to stderr.
 *
 * @param argc Argument count.
 * @param argv Argument vector; argv[0] is the program name, argv[1]
 *             is the command, and subsequent arguments depend on the
 *             command being invoked.
 *
 * @return 0 on successful command execution, or 1 on error or invalid
 *         usage.
 */
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr,
                "Usage:\n"
                "  %s WRITE local-path [remote-path]\n"
                "  %s GET   [-v N] remote-path [local-path]\n"
                "  %s RM    remote-path\n"
                "  %s LS    remote-path\n"
                "  %s STOP\n",
                argv[0], argv[0], argv[0], argv[0], argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "WRITE") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "Usage: %s WRITE local-path [remote-path]\n", argv[0]);
            return 1;
        }
        const char *local_path  = argv[2];
        const char *remote_path = (argc >= 4) ? argv[3] : argv[2];
        return do_write(local_path, remote_path);
    }
    else if (strcmp(cmd, "GET") == 0)
    {
        int version = -1;
        const char *remote_path;
        const char *local_path = NULL;
        int idx = 2;

        if (argc >= 5 && strcmp(argv[2], "-v") == 0)
        {
            version = atoi(argv[3]);
            if (version <= 0)
            {
                fprintf(stderr, "GET: -v requires positive integer version\n");
                return 1;
            }
            idx = 4;
        }

        if (argc <= idx)
        {
            fprintf(stderr,
                    "Usage: %s GET [-v N] remote-path [local-path]\n",
                    argv[0]);
            return 1;
        }

        remote_path = argv[idx];
        if (argc > idx + 1)
            local_path = argv[idx + 1];

        return do_get(remote_path, local_path, version);
    }
    else if (strcmp(cmd, "RM") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "Usage: %s RM remote-path\n", argv[0]);
            return 1;
        }
        return do_rm(argv[2]);
    }
    else if (strcmp(cmd, "LS") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "Usage: %s LS remote-path\n", argv[0]);
            return 1;
        }
        return do_ls(argv[2]);
    }
    else if (strcmp(cmd, "STOP") == 0)
    {
        return do_stop();
    }

    fprintf(stderr, "Unknown command: %s\n", cmd);
    return 1;
}