#ifndef BIGWIG_H_PRETEXTGRAPH
#define BIGWIG_H_PRETEXTGRAPH

#include "Header.h"

#ifndef _WIN32
#include <limits.h>
#include <sys/wait.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

// Result of starting bigWigToBedGraph.
enum bigwig_open_result
{
    BIGWIG_OK = 0,
    BIGWIG_PIPE_FAILED = 1,
    BIGWIG_FORK_FAILED = 2,
    BIGWIG_UNSUPPORTED = 3
};

// Result of waiting for the bigWigToBedGraph child.
enum bigwig_child_result
{
    BIGWIG_CHILD_OK = 0,
    BIGWIG_CHILD_NONE = 1,
    BIGWIG_CHILD_NOT_FOUND = 2,
    BIGWIG_CHILD_FAILED = 3
};

struct bigwig_source
{
    s32 readFd;
    s32 childPid;
    u32 openResult;
};

// UCSC bigWigToBedGraph downloaded next to this executable by the build.
global_function
u32
BigWigConverterPath(char *out, u32 outLen)
{
#ifndef _WIN32
    char exe[PATH_MAX];
#ifdef __APPLE__
    uint32_t size = (uint32_t)sizeof(exe);
    if (_NSGetExecutablePath(exe, &size) != 0) return 0;
#else
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (n <= 0) return 0;
    exe[n] = 0;
#endif
    char real[PATH_MAX];
    if (!realpath(exe, real)) return 0;
    char *slash = strrchr(real, '/');
    if (!slash) return 0;
    *slash = 0;
    int wrote = snprintf(out, outLen, "%s/bigWigToBedGraph", real);
    if (wrote < 0 || (u32)wrote >= outLen) return 0;
    if (access(out, X_OK) != 0) return 0;
    return 1;
#else
    (void)out;
    (void)outLen;
    return 0;
#endif
}

// Fork UCSC bigWigToBedGraph and return the read end of its stdout.
// The copy downloaded beside PretextGraph is used first. PATH is the fallback.
// The caller owns readFd and must wait on childPid with FinishBigWigChild.
global_function
bigwig_source
OpenBigWigAsBedgraph(const char *path)
{
    bigwig_source source;
    source.readFd = -1;
    source.childPid = 0;
    source.openResult = BIGWIG_OK;

#ifndef _WIN32
    int pipeFds[2];
    if (pipe(pipeFds) != 0)
    {
        source.openResult = BIGWIG_PIPE_FAILED;
        return source;
    }

    pid_t child = fork();
    if (child < 0)
    {
        close(pipeFds[0]);
        close(pipeFds[1]);
        source.openResult = BIGWIG_FORK_FAILED;
        return source;
    }

    if (child == 0)
    {
        dup2(pipeFds[1], STDOUT_FILENO);
        close(pipeFds[0]);
        close(pipeFds[1]);
        char converter[PATH_MAX];
        if (BigWigConverterPath(converter, sizeof(converter)))
        {
            execl(converter, "bigWigToBedGraph", path, "/dev/stdout", (char *)0);
        }
        else
        {
            execlp("bigWigToBedGraph", "bigWigToBedGraph", path, "/dev/stdout", (char *)0);
        }
        _exit(127);
    }

    close(pipeFds[1]);
    source.readFd = pipeFds[0];
    source.childPid = (s32)child;
    return source;
#else
    (void)path;
    source.openResult = BIGWIG_UNSUPPORTED;
    return source;
#endif
}

global_function
u32
FinishBigWigChild(s32 childPid)
{
    if (childPid <= 0) return BIGWIG_CHILD_NONE;

#ifndef _WIN32
    int status = 0;
    waitpid((pid_t)childPid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 127) return BIGWIG_CHILD_NOT_FOUND;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) return BIGWIG_CHILD_FAILED;
    return BIGWIG_CHILD_OK;
#else
    return BIGWIG_CHILD_NONE;
#endif
}

#endif // BIGWIG_H_PRETEXTGRAPH
