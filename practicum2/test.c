/*
 * test.c -- Automated tests for RFS client/server practicum
 *
 * This file is intended to be compiled and run on the CLIENT side.
 * It assumes that:
 *   - The RFS client binary is named "./rfs"
 *   - The server is already running on the GCP VM
 *   - rfs.h was configured with the correct SERVER_IP / SERVER_PORT
 *
 * Each test below is annotated with the Practicum question it targets:
 *   Q1: WRITE
 *   Q2: GET
 *   Q3: RM
 *   Q4: Multithreading / concurrency
 *   Q5: Versioning on WRITE
 *   Q6: LS version listing
 *   Q7: GET -v (specific version retrieval)
 *   Q7+: STOP (extra command you implemented)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Path to the RFS client binary. Adjust if needed. */
#define RFS_CMD "./rfs"

/* ------------------------------------------------------------------ */
/*                    Helper functions / utilities                     */
/* ------------------------------------------------------------------ */

/* Write a small text file with the given content. */
static int write_local_file(const char *path, const char *content)
{
    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("fopen(write_local_file)");
        return -1;
    }
    if (fputs(content, fp) == EOF) {
        perror("fputs(write_local_file)");
        fclose(fp);
        return -1;
    }
    if (fclose(fp) != 0) {
        perror("fclose(write_local_file)");
        return -1;
    }
    return 0;
}

/* Read entire file into a dynamically allocated buffer. */
static int read_whole_file(const char *path, char **out_buf, size_t *out_len)
{
    *out_buf = NULL;
    *out_len = 0;

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        perror("fopen(read_whole_file)");
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("fseek(read_whole_file)");
        fclose(fp);
        return -1;
    }

    long len = ftell(fp);
    if (len < 0) {
        perror("ftell(read_whole_file)");
        fclose(fp);
        return -1;
    }
    rewind(fp);

    char *buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        perror("malloc(read_whole_file)");
        fclose(fp);
        return -1;
    }

    size_t nread = fread(buf, 1, (size_t)len, fp);
    fclose(fp);

    if (nread != (size_t)len) {
        fprintf(stderr, "read_whole_file: short read\n");
        free(buf);
        return -1;
    }

    buf[len] = '\0';

    *out_buf = buf;
    *out_len = (size_t)len;
    return 0;
}

/* Compare a file on disk with an expected string. */
static int file_equals_string(const char *path, const char *expected)
{
    char *buf = NULL;
    size_t len = 0;
    if (read_whole_file(path, &buf, &len) < 0) {
        return 0;
    }

    size_t exp_len = strlen(expected);
    int equal = (len == exp_len && memcmp(buf, expected, len) == 0);

    free(buf);
    return equal;
}

/*
 * Run a shell command and return 1 on success (exit code 0),
 * 0 on failure.
 *
 * This is used to invoke "./rfs ..." for many tests.
 */
static int run_cmd(const char *fmt, ...)
{
    char cmd[1024];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(cmd, sizeof(cmd), fmt, ap);
    va_end(ap);

    printf("  [CMD] %s\n", cmd);

    int status = system(cmd);
    if (status == -1) {
        perror("system");
        return 0;
    }
    if (!WIFEXITED(status)) {
        fprintf(stderr, "  [ERR] Command did not exit normally\n");
        return 0;
    }
    if (WEXITSTATUS(status) != 0) {
        fprintf(stderr, "  [ERR] Command exit status = %d\n", WEXITSTATUS(status));
        return 0;
    }
    return 1;
}

/*
 * Spawn a process that directly execs RFS_CMD with given arguments.
 * This is used for the Q4 concurrency test.
 *
 * Returns child's PID on success, -1 on failure.
 */
static pid_t spawn_rfs(const char *arg0,
                       const char *arg1,
                       const char *arg2,
                       const char *arg3,
                       const char *arg4)
{
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork(spawn_rfs)");
        return -1;
    }

    if (pid == 0) {
        /* Child: execlp(RFS_CMD, "rfs", arg0, arg1, arg2, arg3, arg4, NULL) */
        execlp(RFS_CMD,
               "rfs",
               arg0,
               arg1,
               arg2,
               arg3,
               arg4,
               (char *)NULL);
        perror("execlp");
        _exit(127);
    }

    /* Parent returns child's PID */
    return pid;
}

/* ------------------------------------------------------------------ */
/*                             Tests                                  */
/* ------------------------------------------------------------------ */

/*
 * Q1: WRITE basic functionality
 *
 * - Test writing a file with explicit remote path:
 *     rfs WRITE local_q1.txt practicum/q1_basic.txt
 */
static int test_Q1_write_basic(void)
{
    printf("=== Q1: WRITE basic ===\n");

    const char *local = "local_q1.txt";
    const char *remote = "practicum/q1_basic.txt";
    const char *content = "Q1 basic WRITE test\n";

    if (write_local_file(local, content) < 0) {
        fprintf(stderr, "  [FAIL] Could not create local file %s\n", local);
        return 0;
    }

    if (!run_cmd("%s WRITE %s %s", RFS_CMD, local, remote)) {
        fprintf(stderr, "  [FAIL] WRITE command failed\n");
        return 0;
    }

    printf("  [PASS] Q1 basic WRITE succeeded\n");
    return 1;
}

/*
 * Q2: GET basic functionality
 *
 * - First write a file to the server.
 * - Then GET it back under a different local name and compare contents.
 *
 * This corresponds to:
 *   rfs WRITE local_q2_src.txt practicum/q2_get.txt
 *   rfs GET practicum/q2_get.txt local_q2_dst.txt
 */
static int test_Q2_get_basic(void)
{
    printf("=== Q2: GET basic ===\n");

    const char *local_src = "local_q2_src.txt";
    const char *local_dst = "local_q2_dst.txt";
    const char *remote = "practicum/q2_get.txt";
    const char *content = "Q2 GET test content\n";

    if (write_local_file(local_src, content) < 0) {
        fprintf(stderr, "  [FAIL] Could not create %s\n", local_src);
        return 0;
    }

    if (!run_cmd("%s WRITE %s %s", RFS_CMD, local_src, remote)) {
        fprintf(stderr, "  [FAIL] WRITE for Q2 failed\n");
        return 0;
    }

    if (!run_cmd("%s GET %s %s", RFS_CMD, remote, local_dst)) {
        fprintf(stderr, "  [FAIL] GET for Q2 failed\n");
        return 0;
    }

    if (!file_equals_string(local_dst, content)) {
        fprintf(stderr, "  [FAIL] Downloaded file does not match original\n");
        return 0;
    }

    printf("  [PASS] Q2 GET retrieved correct contents\n");
    return 1;
}

/*
 * Q3: RM (remove file) functionality
 *
 * - Write a file to the server.
 * - Remove it using:
 *      rfs RM practicum/q3_rm.txt
 * - Then attempt a GET; we expect GET to fail (non-zero status), so
 *   we treat a failing GET as success for this test.
 */
static int test_Q3_rm_basic(void)
{
    printf("=== Q3: RM basic ===\n");

    const char *local = "local_q3.txt";
    const char *remote = "practicum/q3_rm.txt";
    const char *content = "Q3 RM test content\n";

    if (write_local_file(local, content) < 0) {
        fprintf(stderr, "  [FAIL] Could not create %s\n", local);
        return 0;
    }

    if (!run_cmd("%s WRITE %s %s", RFS_CMD, local, remote)) {
        fprintf(stderr, "  [FAIL] WRITE for Q3 failed\n");
        return 0;
    }

    if (!run_cmd("%s RM %s", RFS_CMD, remote)) {
        fprintf(stderr, "  [FAIL] RM for Q3 failed\n");
        return 0;
    }

    /* Now try GET: we EXPECT this to fail, so do a raw system() call. */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s GET %s local_q3_after_rm.txt", RFS_CMD, remote);
    printf("  [CMD] %s (expected to FAIL)\n", cmd);

    int status = system(cmd);
    if (status != -1 && WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        printf("  [PASS] GET failed as expected after RM\n");
        return 1;
    }

    fprintf(stderr, "  [FAIL] GET unexpectedly succeeded after RM\n");
    return 0;
}

/*
 * Q4: Multithreading / concurrency
 *
 * From the rubric: the server must handle multiple clients simultaneously
 * and maintain file integrity when multiple clients write to the same file.
 *
 * This test:
 *   - Creates several local files with different contents.
 *   - Uses fork()+execlp() to start N concurrent WRITE clients to the SAME
 *     remote path: practicum/q4_concurrent.txt
 *   - Waits for all children.
 *   - Then does:
 *       rfs GET practicum/q4_concurrent.txt q4_latest.txt
 *   - Verifies that the final file on the server is EXACTLY equal to the
 *     content of *one* of the clients (0..N-1), i.e., no corruption.
 *
 * IMPORTANT: Because of scheduling, we CANNOT assume which client is
 * "last" — the OS decides that. So instead of checking that the file
 * matches client N-1, we check that it matches *some* complete client
 * write and not a mixture of multiple clients.
 *
 * This demonstrates:
 *   - Multiple clients can connect and write in parallel (Q4).
 *   - The file remains coherent and not corrupted by interleaved writes.
 */
static int test_Q4_concurrency(void)
{
    printf("=== Q4: Concurrency (multiple simultaneous WRITEs) ===\n");

    const int N = 4;
    const char *remote = "practicum/q4_concurrent.txt";
    pid_t pids[N];

    /* Prepare local files with distinct content */
    char expected_contents[N][128];
    for (int i = 0; i < N; i++) {
        char local[64];
        snprintf(local, sizeof(local), "local_q4_%d.txt", i);
        snprintf(expected_contents[i], sizeof(expected_contents[i]),
                 "Q4 concurrent write from client %d\n", i);
        if (write_local_file(local, expected_contents[i]) < 0) {
            fprintf(stderr, "  [FAIL] Could not create %s\n", local);
            return 0;
        }
    }

    /* Spawn N concurrent WRITE clients */
    for (int i = 0; i < N; i++) {
        char local[64];
        snprintf(local, sizeof(local), "local_q4_%d.txt", i);
        pids[i] = spawn_rfs("WRITE", local, remote, NULL, NULL);
        if (pids[i] < 0) {
            fprintf(stderr, "  [FAIL] spawn_rfs failed for client %d\n", i);
            return 0;
        }
    }

    /* Wait for all children */
    int all_ok = 1;
    for (int i = 0; i < N; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
            all_ok = 0;
            continue;
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "  [ERR] child %d exit status %d\n",
                    i, WEXITSTATUS(status));
            all_ok = 0;
        }
    }

    if (!all_ok) {
        fprintf(stderr, "  [FAIL] One or more concurrent WRITE clients failed\n");
        return 0;
    }

    /* Get the newest version and ensure it matches ONE of the clients exactly */
    const char *local_latest = "local_q4_latest.txt";
    if (!run_cmd("%s GET %s %s", RFS_CMD, remote, local_latest)) {
        fprintf(stderr, "  [FAIL] GET after concurrent WRITEs failed\n");
        return 0;
    }

    /* Check against each expected content */
    int match = 0;
    for (int i = 0; i < N; i++) {
        if (file_equals_string(local_latest, expected_contents[i])) {
            printf("  [INFO] Latest file matches client %d's write\n", i);
            match = 1;
            break;
        }
    }

    if (!match) {
        fprintf(stderr, "  [FAIL] Latest file does not match any single client write\n");
        return 0;
    }

    printf("  [PASS] Q4 concurrency: multiple clients, file is coherent (one whole write)\n");
    return 1;
}


/*
 * Q5 + Q7: Versioning on WRITE and GET -v
 *
 * IMPORTANT NOTE ABOUT YOUR VERSIONING SEMANTICS:
 *   With your current server logic, after N WRITEs to the same remote
 *   path, you end up with:
 *     - Base file: newest version (vN)
 *     - file.v1 : oldest previous version
 *     - file.v2 : second oldest previous version
 *     - ...
 *
 * So after 3 WRITEs you have:
 *   WRITE #1 -> v1 in base
 *   WRITE #2 -> v1 saved as .v1, v2 in base
 *   WRITE #3 -> v2 moved to .v2, v1 still in .v1, v3 in base
 *
 * This test:
 *   - Removes any prior remote file.
 *   - Performs THREE WRITEs of different content:
 *         v1, v2, v3
 *   - Then verifies:
 *       GET -v 1 => content_v1
 *       GET -v 2 => content_v2
 *       GET (no -v) => content_v3 (newest)
 *
 * That directly exercises:
 *   - Q5: multiple previous versions are preserved (.v1, .v2, ...)
 *   - Q7: GET -v retrieves a specific historical version.
 */
static int test_Q5_Q7_versioning_and_get_v(void)
{
    printf("=== Q5/Q7: Versioning + GET -v (3 WRITEs) ===\n");

    const char *remote = "practicum/q5_versioned.txt";

    const char *local_v1 = "local_q5_v1.txt";
    const char *local_v2 = "local_q5_v2.txt";
    const char *local_v3 = "local_q5_v3.txt";

    const char *content_v1 = "Q5/Q7 version 1 content\n";
    const char *content_v2 = "Q5/Q7 version 2 content\n";
    const char *content_v3 = "Q5/Q7 version 3 content (latest)\n";

    /* Try to remove any prior file/versions; ignore success/failure */
    (void)system(RFS_CMD " RM practicum/q5_versioned.txt > /dev/null 2>&1");

    /* Create three distinct local versions */
    if (write_local_file(local_v1, content_v1) < 0 ||
        write_local_file(local_v2, content_v2) < 0 ||
        write_local_file(local_v3, content_v3) < 0) {
        fprintf(stderr, "  [FAIL] Could not create Q5/Q7 local files\n");
        return 0;
    }

    /* WRITE #1 -> base = v1 */
    if (!run_cmd("%s WRITE %s %s", RFS_CMD, local_v1, remote)) {
        fprintf(stderr, "  [FAIL] WRITE v1 failed\n");
        return 0;
    }
    /* WRITE #2 -> base v1 -> .v1, base = v2 */
    if (!run_cmd("%s WRITE %s %s", RFS_CMD, local_v2, remote)) {
        fprintf(stderr, "  [FAIL] WRITE v2 failed\n");
        return 0;
    }
    /* WRITE #3 -> base v2 -> .v2 (v1 still in .v1), base = v3 */
    if (!run_cmd("%s WRITE %s %s", RFS_CMD, local_v3, remote)) {
        fprintf(stderr, "  [FAIL] WRITE v3 failed\n");
        return 0;
    }

    /* Now GET each historical version explicitly using -v */
    const char *out_v1 = "q5_v1_out.txt";
    const char *out_v2 = "q5_v2_out.txt";
    const char *out_latest = "q5_latest_out.txt";

    /* Q7: GET -v 1 should retrieve the first content (now in .v1) */
    if (!run_cmd("%s GET -v 1 %s %s", RFS_CMD, remote, out_v1)) {
        fprintf(stderr, "  [FAIL] GET -v 1 failed\n");
        return 0;
    }

    /* Q7: GET -v 2 should retrieve the second content (now in .v2) */
    if (!run_cmd("%s GET -v 2 %s %s", RFS_CMD, remote, out_v2)) {
        fprintf(stderr, "  [FAIL] GET -v 2 failed\n");
        return 0;
    }

    /* Optional: GET newest (no -v) should be v3 */
    if (!run_cmd("%s GET %s %s", RFS_CMD, remote, out_latest)) {
        fprintf(stderr, "  [FAIL] GET (latest) failed\n");
        return 0;
    }

    if (!file_equals_string(out_v1, content_v1)) {
        fprintf(stderr, "  [FAIL] Version 1 contents mismatch\n");
        return 0;
    }
    if (!file_equals_string(out_v2, content_v2)) {
        fprintf(stderr, "  [FAIL] Version 2 contents mismatch\n");
        return 0;
    }
    if (!file_equals_string(out_latest, content_v3)) {
        fprintf(stderr, "  [FAIL] Latest version contents mismatch\n");
        return 0;
    }

    printf("  [PASS] Q5/Q7: versioning and GET -v work correctly with 3 WRITEs\n");
    return 1;
}

/*
 * Q6: LS (list versions)
 *
 * - Reuse the versioned file from Q5/Q7 if it exists.
 * - Run:
 *      rfs LS practicum/q5_versioned.txt
 *   We only check that LS returns success (exit code 0). The human
 *   reviewer can also visually inspect the output to confirm that
 *   all versions and timestamps are listed correctly.
 */
static int test_Q6_ls_versions(void)
{
    printf("=== Q6: LS (list versions) ===\n");

    const char *remote = "practicum/q5_versioned.txt";

    if (!run_cmd("%s LS %s", RFS_CMD, remote)) {
        fprintf(stderr, "  [FAIL] LS command failed\n");
        return 0;
    }

    printf("  [PASS] Q6 LS command succeeded; check output for versions + timestamps\n");
    return 1;
}

/*
 * Q7+: STOP command (not part of original rubric, but tests your extra feature)
 *
 * - Run:
 *      rfs STOP
 *   and check for success.
 *
 * Note: This should be the LAST test, because it will shut down the server.
 *       If you want to keep the server running for manual tests, you can
 *       comment this test out.
 */
static int test_STOP_command(void)
{
    printf("=== Q7+: STOP server ===\n");

    if (!run_cmd("%s STOP", RFS_CMD)) {
        fprintf(stderr, "  [FAIL] STOP command failed\n");
        return 0;
    }

    printf("  [PASS] STOP command succeeded; server should be shutting down\n");
    return 1;
}

/* ------------------------------------------------------------------ */
/*                               main                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    int total = 0;
    int passed = 0;

    /* Q1: WRITE */
    total++;
    if (test_Q1_write_basic()) passed++;

    /* Q2: GET */
    total++;
    if (test_Q2_get_basic()) passed++;

    /* Q3: RM */
    total++;
    if (test_Q3_rm_basic()) passed++;

    /* Q4: concurrency */
    total++;
    if (test_Q4_concurrency()) passed++;

    /* Q5 + Q7: versioning + GET -v (3 WRITEs) */
    total++;
    if (test_Q5_Q7_versioning_and_get_v()) passed++;

    /* Q6: LS */
    total++;
    if (test_Q6_ls_versions()) passed++;

    /* Q7+: STOP (run last) */
    total++;
    if (test_STOP_command()) passed++;

    printf("\n=== SUMMARY ===\n");
    printf("  Passed %d / %d tests\n", passed, total);

    return (passed == total) ? 0 : 1;
}
