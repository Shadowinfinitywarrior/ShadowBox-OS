#ifndef SHADOWBOX_WAIT_H
#define SHADOWBOX_WAIT_H

#define WNOHANG     1
#define WUNTRACED   2
#define WCONTINUED  8

#define WEXITSTATUS(s)  ((s) & 0xFF)
#define WTERMSIG(s)     ((s) & 0x7F)
#define WSTOPSIG(s)     WEXITSTATUS(s)
#define WIFEXITED(s)    (WTERMSIG(s) == 0)
#define WIFSTOPPED(s)   ((WTERMSIG(s) & 0x80) != 0)
#define WIFSIGNALED(s)  (!WIFSTOPPED(s) && !WIFEXITED(s))

#endif
