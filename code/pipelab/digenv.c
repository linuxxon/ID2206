/* digenv.c - Simple C program to study environment variables
 * For KTH course ID2206 - Operatingsystems
 * By Rasmus Linusson 10/11-2016
 */

#include <sys/types.h> /* pid_t */
#include <sys/wait.h> /* wait */
#include <errno.h> /* errno */
#include <stdio.h> /* printf */
#include <stdlib.h> /* getenv */
#include <unistd.h> /* fork, exec, pipe, dup */

/* Array indicies for PIDs and pipe fd's */
#define PRINTENV    0
#define SORT        1

/* Array indicies for fd's */
#define PIPEOUT     0
#define PIPEIN      1

int retval; /* Variable to hold return values */

int pipe_fd[2][2]; /* Pipe file descriptors */

/* closePipes - Close all open pipes (defined by pipe_fd) except those supplied
 * in parameters
 * Parameters:
 *      int *keep_open - array of pipe fd's not to close
 */
int closePipes(int *keep_open)
{
    int i, j, k, remove;

    for (i = 0; i < sizeof(pipe_fd)/sizeof(pipe_fd[0]); i++)
    {
        for (j = 0; j < sizeof(pipe_fd[i])/sizeof(int); j++)
        {
            remove = 1; /* If remove == 0 keep file descriptor */

            for (k = 0; k < sizeof(keep_open)/sizeof(int); k++)
            {
                if (pipe_fd[i][j] == keep_open[k])
                    remove = 0;
            }

            if (remove)
            {
                retval = close(pipe_fd[i][j]);
                if (retval == -1)
                {
                    perror("Could not close pipe");
                    exit(2);
                }
            }
        }
    }
    return 0;
}


/* startProcess - Fork process and execute program
 * Parameters:
 *      char *argv[] - argument vector to pass onto program
 *      pid_t *pid - pointer to pid
 *      int io_redir[2] - Pipe file descriptors to redirect.
 *                      Value = -1 - Do nothing
 *                      Index 0 - Redirect stdout
 *                      Index 1 - Redirect stdin
 */
int startProcess(char *argv[], pid_t *pid, int *io_redir)
{
    *pid = fork();
    if (*pid == 0) /* Child process */
    {
        /* Setup pipes */
        if (io_redir[PIPEOUT] != -1)
        {
            retval = dup2(io_redir[PIPEOUT], STDIN_FILENO);
            if (retval == -1)
            {
                perror("Could not redirect stdin");
                exit(1);
            }
        }
        if (io_redir[PIPEIN] != -1)
        {
            retval = dup2(io_redir[PIPEIN], STDOUT_FILENO);
            if (retval == -1)
            {
                perror("Could not redirect stdout");
                exit(1);
            }
        }

        
        /* Close unused pipe file descriptor for this process */
        closePipes(io_redir);

        /* Execute new program */
        (void) execvp(argv[0], argv);
        return -1;
    }
    if (*pid == -1)
        return -1;
    return 0;
}

/* waitChild - Wait for a child process to stop executing and report any
 * problems to stderr
 */
void waitChild()
{
    int status;
    pid_t childpid;

    childpid = wait(&status);
    if (childpid == -1)
    {
        perror("wait() failed unexpectedly");
        exit(5);
    }
    if (WIFEXITED(status)) /* One child is done executing */
    {
        int child_status = WEXITSTATUS(status);
        if (child_status != 0) /* Child had problems */
        {
            fprintf(stderr, "Child (pid %ld) failed with exit code %d\n",
                    (long int) childpid, child_status);
        }
    }
    else if (WIFSIGNALED(status)) /* Child process was aborted by a signal */
    {
        int child_signal = WTERMSIG(status);
        fprintf(stderr, "Child (pid %ld) was terminadet by signal no. %d\n",
                (long int) childpid, child_signal);
    }
}

int main(int argc, char* argv[])
{
    /* PID array for childprocesses */
    pid_t pid[2];

    char *pager = getenv("PAGER");

    /* Arguments to start processes */
    char *argv1[] = { "printenv", (char ) 0 };
    char *argv2[] = { "sort", (char *) 0 };
    char *argv3[] = { pager, (char *) 0};

    int io_redir[2];

    /* Setup pipes */
    retval = pipe(pipe_fd[ PRINTENV ]);
    if (retval == -1)
    {
        perror("Could not open pipe");
        exit(1);
    }

    retval = pipe(pipe_fd[ SORT ]);
    if (retval == -1)
    {
        perror("Could not open pipe");
        exit(1);
    }

    /* Run programs default */
    io_redir[STDIN_FILENO] = -1;
    io_redir[STDOUT_FILENO] = pipe_fd[PRINTENV][PIPEIN];
    retval = startProcess(argv1, &pid[PRINTENV], io_redir);
    if (retval == -1)
    {
        perror("Could not run 'printenv'");
        exit(4);
    }

    io_redir[STDIN_FILENO] = pipe_fd[PRINTENV][PIPEOUT];
    io_redir[STDOUT_FILENO] = pipe_fd[SORT][PIPEIN];
    retval = startProcess(argv2, &pid[SORT], io_redir);
    if (retval == -1)
    {
        perror("Could not run 'sort'");
        exit(4);
    }

    /* Prepair parent for less */
    io_redir[0] = pipe_fd[SORT][PIPEOUT]; /* Reuse io_redir */
    io_redir[1] = -1;
    closePipes(io_redir);

    retval = dup2(pipe_fd[SORT][PIPEOUT], STDIN_FILENO);
    if (retval == -1)
    {
        perror("Could not redirect stdin");
        exit(2);
    }

    (void) execvp(argv3[0], argv3);

    /* Wait for children */
    waitChild();
    waitChild();

    return 0;
}
