/*
 * rfs.h -- Remote File System client
 *
 * Sooji Kim | CS5600 | Northeastern University
 * Fall 2025 | Dec 6 2025
 *
 */

#ifndef RFS_H
#define RFS_H

#include <stddef.h>
#include <stdint.h>

#define SERVER_IP   "34.19.98.211"
#define SERVER_PORT 2000

/**
 * @brief Send exactly len bytes over a connected socket.
 *
 * This helper repeatedly calls send(2) until either all @p len bytes
 * from @p buf have been written to @p sockfd or an error/closed
 * connection occurs.
 *
 * @param sockfd Connected TCP socket file descriptor.
 * @param buf Pointer to the buffer containing data to send.
 * @param len Number of bytes to send from @p buf.
 *
 * @return 0 on success (all bytes sent), or -1 on error or if the
 *         connection is closed prematurely.
 */
int send_all(int sockfd, const void *buf, size_t len);

/**
 * @brief Receive exactly len bytes from a connected socket.
 *
 * This helper repeatedly calls recv(2) until either @p len bytes
 * have been read into @p buf from @p sockfd or an error/closed
 * connection occurs.
 *
 * @param sockfd Connected TCP socket file descriptor.
 * @param buf Pointer to the destination buffer to fill with data.
 * @param len Number of bytes to receive into @p buf.
 *
 * @return 0 on success (all bytes received), or -1 on error or if the
 *         connection is closed prematurely.
 */
int recv_all(int sockfd, void *buf, size_t len);

/**
 * @brief Return a pointer to the basename portion of a path string.
 *
 * Searches for the last '/' in @p path and returns a pointer to the
 * character after it. If no '/' is found, @p path itself is returned.
 *
 * The returned pointer aliases the original string; it must not be
 * freed by the caller.
 *
 * @param path Null-terminated file system path string.
 *
 * @return Pointer to the basename portion of @p path.
 */
const char *basename_const(const char *path);

/**
 * @brief Establish a TCP connection to the remote file system server.
 *
 * Creates a TCP socket and connects it to SERVER_IP:SERVER_PORT.
 * On success, the caller owns the returned socket descriptor and is
 * responsible for closing it.
 *
 * @return A connected socket file descriptor on success, or -1 on
 *         error (any created socket is closed on failure).
 */
int connect_to_server(void);

/**
 * @brief Execute the WRITE client command.
 *
 * Reads the contents of @p local_path and sends a WRITE request to
 * the server, storing the data under @p remote_path on the remote
 * file system.
 *
 * @param local_path Path to the local file to upload.
 * @param remote_path Remote path under which the server should store
 *                    the file (possibly as a versioned object).
 *
 * @return 0 on success, or 1 on I/O, allocation, or networking error.
 */
int do_write(const char *local_path, const char *remote_path);

/**
 * @brief Execute the GET client command with optional versioning.
 *
 * Requests a file from the server and writes it to a local file.
 * If @p version is greater than 0, a specific version is requested;
 * otherwise, the newest version is retrieved.
 *
 * @param remote_path Base remote path of the file to retrieve.
 * @param maybe_local_path Optional local path to save the file; if
 *                         NULL, the basename of the requested remote
 *                         path is used.
 * @param version Version number to retrieve; if > 0, a specific
 *                version is requested, otherwise the newest version
 *                is requested.
 *
 * @return 0 on success, or 1 on error (not found, I/O, or networking).
 */
int do_get(const char *remote_path, const char *maybe_local_path, int version);

/**
 * @brief Execute the RM client command.
 *
 * Sends an RM request to delete @p remote_path on the server and
 * prints a message describing the outcome (success or error).
 *
 * @param remote_path Remote path (file or directory) to remove.
 *
 * @return 0 on success, or 1 if the removal fails or an error occurs.
 */
int do_rm(const char *remote_path);

/**
 * @brief Execute the LS client command to list file versions.
 *
 * Requests versioning information for @p remote_path from the server,
 * then prints each version name and its last modified timestamp.
 *
 * @param remote_path Remote file path whose versions should be listed.
 *
 * @return 0 on success (including when no versions exist), or 1 on
 *         networking or allocation error.
 */
int do_ls(const char *remote_path);

/**
 * @brief Execute the STOP client command.
 *
 * Connects to the server, sends a STOP command, and reports whether
 * the server indicates it will shut down.
 *
 * @return 0 if the command exchange succeeds, or 1 on networking
 *         error.
 */
int do_stop(void);

#endif /* RFS_H */