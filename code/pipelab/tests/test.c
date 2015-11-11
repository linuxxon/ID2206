#include <sys/types.h> /* pid_t */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    pid_t pid;

    pid = fork();

    if (pid == 0)
    {
        while (1);
    }
    else
        printf("Pid of child %i, Pid of me %i\n", pid, getpid());
    sleep(1);
    return 0;
}
