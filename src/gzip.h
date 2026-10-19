#ifndef GZIP_H_PRETEXTGRAPH
#define GZIP_H_PRETEXTGRAPH

#include "Header.h"

#ifndef _WIN32
#include <sys/wait.h>
#endif

// Result of starting gzip -dc.
enum gzip_open_result
{
    GZIP_OK = 0,
    GZIP_PIPE_FAILED = 1,
    GZIP_FORK_FAILED = 2,
    GZIP_UNSUPPORTED = 3
};

// Result of waiting for the gzip child.
enum gzip_child_result
{
    GZIP_CHILD_OK = 0,
    GZIP_CHILD_NONE = 1,
    GZIP_CHILD_NOT_FOUND = 2,
    GZIP_CHILD_FAILED = 3
};

struct gzip_source
{
    s32 readFd;
    s32 childPid;
    u32 openResult;
};

// Fork gzip -dc and return the read end of its stdout.
// The caller owns readFd and must wait on childPid with FinishGzipChild.
global_function
gzip_source
OpenGzipAsBedgraph(const char *path)
{
    gzip_source source;
    source.readFd = -1;
    source.childPid = 0;
    source.openResult = GZIP_OK;

#ifndef _WIN32
    int pipeFds[2];
    if (pipe(pipeFds) != 0)
    {
        source.openResult = GZIP_PIPE_FAILED;
        return source;
    }

    pid_t child = fork();
    if (child < 0)
    {
        close(pipeFds[0]);
        close(pipeFds[1]);
        source.openResult = GZIP_FORK_FAILED;
        return source;
    }

    if (child == 0)
    {
        dup2(pipeFds[1], STDOUT_FILENO);
        close(pipeFds[0]);
        close(pipeFds[1]);
        execlp("gzip", "gzip", "-dc", "--", path, (char *)0);
        _exit(127);
    }

    close(pipeFds[1]);
    source.readFd = pipeFds[0];
    source.childPid = (s32)child;
    return source;
#else
    (void)path;
    source.openResult = GZIP_UNSUPPORTED;
    return source;
#endif
}

global_function
u32
FinishGzipChild(s32 childPid)
{
    if (childPid <= 0) return GZIP_CHILD_NONE;

#ifndef _WIN32
    int status = 0;
    waitpid((pid_t)childPid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 127) return GZIP_CHILD_NOT_FOUND;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) return GZIP_CHILD_FAILED;
    return GZIP_CHILD_OK;
#else
    return GZIP_CHILD_NONE;
#endif
}

#endif // GZIP_H_PRETEXTGRAPH
