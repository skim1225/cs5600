/*
 * server.h -- RFS server with:
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
#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>
#include <stdint.h>

#define SERVER_PORT 2000
#define SERVER_ROOT "./rfs_root"

/**
 * @brief Receive exactly @p len bytes from a socket.
 *
 * Repeatedly calls recv(2) until @p len bytes have been read into
 * @p buf from @p sockfd, or an error/connection close occurs.
 *
 * @param sockfd Connected socket file descriptor.
 * @param buf Destination buffer to store the received data.
 * @param len Number of bytes that must be received.
 *
 * @return 0 on success (all bytes received), or -1 on error or if the
 *         connection is closed prematurely.
 */
int recv_all(int sockfd, void *buf, size_t len);

/**
 * @brief Send exactly @p len bytes on a socket.
 *
 * Repeatedly calls send(2) until @p len bytes from @p buf have been
 * written to @p sockfd, or an error/connection close occurs.
 *
 * @param sockfd Connected socket file descriptor.
 * @param buf Source buffer containing the data to send.
 * @param len Number of bytes that must be sent.
 *
 * @return 0 on success (all bytes sent), or -1 on error or if the
 *         connection is closed prematurely.
 */
int send_all(int sockfd, const void *buf, size_t len);

/**
 * @brief Ensure that all directories in a given path exist.
 *
 * Creates SERVER_ROOT if it does not exist, then walks through
 * @p full_path and creates intermediate subdirectories as needed.
 * The function assumes @p full_path starts with SERVER_ROOT.
 *
 * @param full_path Full path including SERVER_ROOT and the
 *                  eventual file name.
 *
 * @return 0 on success, or -1 on error (e.g., mkdir failure or
 *         path too long).
 */
int ensure_directories(const char *full_path);

/**
 * @brief Thread entry point for serving a single client connection.
 *
 * Reads a command from the client socket and handles one of the
 * supported operations (WRITE, GET, LS, RM, STOP). The argument
 * is expected to be a pointer to a dynamically allocated integer
 * holding the client socket descriptor, which is freed inside the
 * function.
 *
 * @param arg Pointer to an int containing the client socket file
 *            descriptor, allocated on the heap.
 *
 * @return Always returns NULL (suitable for use with pthread_create()).
 */
void *handle_client(void *arg);

#endif /* SERVER_H */
