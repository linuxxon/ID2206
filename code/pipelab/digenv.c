/* digenv.c - Simple C program to study environment variables
 * For KTH course ID2206 - Operatingsystems
 * By Rasmus Linusson 10/11-2015
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
#define GREP        2

/* Array indicies for fd's */
#define PIPEOUT     0
#define PIPEIN      1

/* Function prototypes */
int  waitChild(int childIndex);
int  closePipes(int *keep_open);
void handle_fail(int failedIndex, int grep);
int  startProcess(char *argv[], pid_t *pid, int *io_redir);

/* Global variables */
int retval; /* Variable to hold return values */

pid_t pid[3]; /* PID array for childprocesses */
int pipe_fd[3][2] = { {-1,-1}, {-1,-1}, {-1,-1} }; /* Pipe file descriptors */

/* main - stuff 
 * 
 * If more arguments than the path is provided, output should be filtered
 * through grep. The switch used to invoke grep is therefore  argc > 1
 */
int main(int argc, char* argv[])
{
    char *pager = getenv("PAGER");

    /* Arguments to start processes */
    char *argv_env[] = { "printenv", (char ) 0 };
    char *argv_sort[] = { "sort", (char *) 0 };
    char *argv_pager[] = { pager, (char *) 0};
    
    char **argv_g = argv;
    argv_g[0] = "grep";


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

    if (argc > 1) /* Filter parameters supplied, feed through grep */
    {
        retval = pipe(pipe_fd[ GREP ]);
        if (retval == -1)
        {
            perror("Could not open pipe");
            exit(1);
        }
    }

    /* Run programs default */
    io_redir[STDIN_FILENO] = -1;
    io_redir[STDOUT_FILENO] = pipe_fd[PRINTENV][PIPEIN];
    retval = startProcess(argv_env, &pid[PRINTENV], io_redir);
    if (retval == -1)
    {
        perror("Could not run 'printenv'");
        exit(4);
    }

    if (argc > 1) /* Filter parameters supplied, feed through grep */
    {
        /* Setup redirection for grep and execute */
        io_redir[STDIN_FILENO] = pipe_fd[PRINTENV][PIPEOUT];
        io_redir[STDOUT_FILENO] = pipe_fd[GREP][PIPEIN];
        retval = startProcess(argv_g, &pid[GREP], io_redir);
        if(retval == -1)
        {
            perror("Could not run 'grep'");
            exit(4);
        }

        /* Setup redirection of sort*/
        io_redir[STDIN_FILENO] = pipe_fd[GREP][PIPEOUT];
    } 
    else /* No filter pararameters supplied */
    {
        /* Default pipe setup for running w/o filters */
        io_redir[STDIN_FILENO] = pipe_fd[PRINTENV][PIPEOUT];
    }
    io_redir[STDOUT_FILENO] = pipe_fd[SORT][PIPEIN]; /* Output is always the same */
    retval = startProcess(argv_sort, &pid[SORT], io_redir);
    if (retval == -1)
    {
        perror("Could not run 'sort'");
        exit(4);
    }


    /* Prepair parent for less */
    io_redir[0] = pipe_fd[SORT][PIPEOUT]; /* Reuse io_redir for convenience */
    io_redir[1] = -1;
    closePipes(io_redir);

    retval = dup2(pipe_fd[SORT][PIPEOUT], STDIN_FILENO);
    if (retval == -1)
    {
        perror("Could not redirect stdin");
        exit(2);
    }


    /* Wait for children */
    retval = waitChild(PRINTENV);
    if (retval != 0)
    {
        /* argc == 1 is an indicator to wheter or not grep is running, if argc
         * == 1 then grep is not running and argument will be 0 matching
         * 'handle_fail()'s specifications */
        handle_fail(PRINTENV, argc == 1);
        exit(6);
    }

    if (argc > 1)
    {
        retval = waitChild(GREP);
        if (retval != 0)
        {
            /* As GREP has already failed, we don't want to kill it in
             * 'handle_fail' - might not be best semantics with the second arg */
            handle_fail(GREP, 1);
            exit(6);
        }
    }

    retval = waitChild(SORT);
    if (retval != 0)
    {
        handle_fail(SORT, argc == 1);
        exit(6);
    }

    (void) execvp(argv_pager[0], argv_pager);

    return 0;
}

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
                if (pipe_fd[i][j] == keep_open[k] || pipe_fd[i][j] == -1)
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
 *
 * Parameters:
 *      int childIndex - Index to child pid in pid array. If < 0 wait for any
 *      process, if >= arraysize wait for any pid
 *
 * Return value:
 *      0  - Process terminated sucessfully
 *      <0 - Process terminated by signal or wait failed
 *      -255 - wait system call failed
 *      >0 - Process terminated with error code
 */
int waitChild(int childIndex)
{
    int status;
    pid_t childpid;

    childpid = waitpid(pid[childIndex], &status, 0);
    if (childpid == -1)
    {
        perror("wait() failed unexpecedly");
        return -255;
    }
    else if (WIFEXITED(status)) /* One child is done executing */
    {
        int child_status = WEXITSTATUS(status);
        if (child_status != 0) /* Child had problems */
        {
            /* If grep failed, leave a more informing message */
            if (childpid == pid[GREP])
            {
                fprintf(stderr, "\nCommand arguments caused grep to fail, please "
                                "check your syntax\n");
            }
            else
                fprintf(stderr, "Child (pid %ld) failed with exit code %d\n",
                        (long int) childpid, child_status);
        }

        /* Return a value that adheres to the specification */
        return child_status > 0 ? child_status : child_status * -1;
    }
    else if (WIFSIGNALED(status)) /* Child process was aborted by a signal */
    {
        int child_signal = WTERMSIG(status);
        fprintf(stderr, "Child (pid %ld) was terminadet by signal no. %d\n",
                (long int) childpid, child_signal);

        return child_signal * -1; /*Invert return value to adhere to specification*/
    }

    return -255; /* Somethings wrong if function does not return before this */
}

/* handle_fail - If a process failed, kill the others
 *
 * Parameters:
 *      int failedIndex - Index of faild process in pid array
 *      int grep - != 0 if grep is also running
 */
void handle_fail(int failedIndex, int grep)
{
    /* Something happened, error has been printed to stderr so just handle
     * it - kill printenv, grep and  sort if running */
    int status;

    retval = waitpid(pid[PRINTENV], &status, WNOHANG); 
    if (retval == 0)
    {
        /* Sort is still running */
        retval = kill(pid[PRINTENV], 9);
        if (retval == -1)
        {
            perror("Can't send kill-signal");
            exit(5);
        }
    }

    if (grep)
    {
        retval= waitpid(pid[GREP], &status, WNOHANG); 
        if (retval == 0)
        {
            /* Sort is still running */
            retval = kill(pid[GREP], 9);
            if (retval == -1)
            {
                perror("Can't send kill-signal");
                exit(5);
            }
        }
    }

    retval = waitpid(pid[SORT], &status, WNOHANG); 
    if (retval == 0)
    {
        /* Sort is still running */
        retval = kill(pid[SORT], 9);
        if (retval == -1)
        {
            perror("Can't send kill-signal");
            exit(5);
        }
    }
}

