/* sigkill.c - Testcode for pipelab pre-requisite */

#include <sys/types.h> /* pid_t */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PIPE_OUTPUT 0
#define PIPE_INPUT 1

int main()
{
    int pipe_fd[2]; /* Pipe file descriptors */
    int ret_val; /* Return value of pipe creation */

    ret_val = pipe(pipe_fd); /* Create a pipe */
    if (ret_val == -1)
    {
        perror("Could not create pipe...");
        exit(1);
    }

    pid_t  childpid; /* Pid child or <1 */

    childpid = fork();
    if (childpid == 0) /* child process */
    {
        /* Install pipe input as stdout for process */
        ret_val = dup2(pipe_fd[PIPE_INPUT], STDOUT_FILENO);
        if (ret_val == -1)
        {
            perror("Could not dup pipe");
            exit(2);
        }

        /* Close pipe output */
        ret_val = close(pipe_fd[PIPE_OUTPUT]);
        if (ret_val == -1)
        {
            perror("Coult not close pipe write");
            exit(3);
        }

        (void) execlp("ls", "ls", (char *) 0);
    }
    
    /* Parent code as child code is exchanged by exec call */
    if (childpid == -1) /* Something went wrong */
    {
        perror("Could not fork");
        exit(2);
    }

    /* start wc */
    childpid = fork();
    if (childpid == 0)
    {
        ret_val = dup2(pipe_fd[PIPE_OUTPUT], STDIN_FILENO);
        if (ret_val == -1)
        {
            perror("Could not dup pipe");
            exit(2);
        }

        /* Close pipe input */
        ret_val = close(pipe_fd[PIPE_INPUT]);
        if (ret_val == -1)
        {
            perror("Could not close pipe");
            exit(3);
        }

        (void) execlp("wc", "wc", (char *) 0);
        
        perror("Cannot exec wc");
    }
    if (childpid == -1)
    {
        perror("Could not fork");
        exit(1);
    }

    /* Parent should not use the pipe */
    ret_val = close(pipe_fd[PIPE_INPUT]);
    if (ret_val == -1)
    {
        perror("Could not close pipe");
        exit(3);
    }

    ret_val = close(pipe_fd[PIPE_OUTPUT]);
    if (ret_val == -1)
    {
        perror("Could not close pipe");
        exit(3);
    }

    /* Wait for children to run */
}
