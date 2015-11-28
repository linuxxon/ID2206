/* digenv.c - Simple C program to study environment variables
 * For KTH course ID2206 - Operatingsystems
 * By Rasmus Linusson 10/11-2015
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PIPEOUT     0
#define PIPEIN      1

#define PRINTENV    0
#define GREP        1
#define SORT        2
#define PAGER       3

int pid[4];

void    fork_error();
void    pipe_error();
void    run_error(int);
void    fatal_error(char *);
int     setup(char**, int, int, int);
void    handle_exit(int childpid, int status);

int main(int argc, char* argv[])
{
    int pipe;   /* File descriptor for piping output */
    int childpid, status;

    /* iterator for for-loop */
    int i;

    /* Number of steps in pipeline */
    int num_proc = 3;
    if (argc > 1) ++num_proc;

    char *pager = getenv("PAGER");  /* Get pager from environment */

    if (pager == NULL) pager = "less"; /* If no pager is prefeered, use less */

    /* Arguments for programs to run */
    char *arg_env[] = { "printenv", (char *) 0 };
    char **arg_grep = argv;
    char *arg_sort[] = { "sort", (char *) 0 };
    char *arg_pager[] = { pager, (char *) 0 };

    /* As grep uses all parameters, just substitute the path */
    arg_grep[0] = "grep";

    
    /* Start processes */
    pipe = setup(arg_env, PRINTENV, -1, 1);
    if (pipe == -1) pipe_error();
    if (pipe == -2) fork_error();

    /* If grep should be used - use grep */
    if (argc > 1)
    {
        pipe = setup(arg_grep, GREP, pipe, 1);
        if (pipe == -1) pipe_error();
        if (pipe == -2) fork_error();
    }

    pipe = setup(arg_sort, SORT, pipe, 1);
    if (pipe == -1) pipe_error();
    if (pipe == -2) fork_error();

    if (argc > 1)
    {
        /* Wait for grep to finish before starting the pager, this because we don't
         * want to kill the pager as it will not let go of the screen space */
        do
        {
            childpid = waitpid(pid[GREP], &status, 0);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        handle_exit(childpid, status);

        /* As grpe has already exited, decrement the number of processes to
         * wait on */
        num_proc--;
    }


    /* Start a pager */
    pipe = setup(arg_pager, PAGER, pipe, 0);
    if (pipe == -1) pipe_error();
    if (pipe == -2) fork_error();

    /* Wait for all processes to exit, except for grep */
    for (i = 0; i < num_proc; i++)
    {
        childpid = wait(&status);
        if (!WIFEXITED(status) || !WIFSIGNALED(status))
            handle_exit(childpid, status);
        else
            i--; /* Child changed state but didn't exit */
    }

    return 0;
}

/* hande_exit - Check exit values to see if they are okay
 * Parameters:
 *      int childpid - pid of the child that wait returned
 *      int status - the status as set by wait
*/
void handle_exit(int childpid, int status)
{
    if (WIFEXITED(status))
    {
        if (childpid == pid[GREP] && WEXITSTATUS(status) == 1)
        { /* No results, not a problem */ }
        else if (WEXITSTATUS(status) != 0)
            run_error(childpid);
    }
    else if (WIFSIGNALED(status))
        run_error(childpid);
}

/* Error while forking occured */
void fork_error()
{
    fatal_error("Error creating new fork");
}

/* If error happend with a pipe, print and shut down */
void pipe_error()
{
    fatal_error("Could not open/dup/close pipe");
}

/* Error in some stage of the pipeline, retreat */
void run_error(int error_pid)
{
    if (error_pid == pid[PRINTENV])
        fatal_error("printenv failed");
    if (error_pid == pid[GREP])
        fatal_error("grep failed");
    if (error_pid == pid[SORT])
        fatal_error("sort failed");
    if (error_pid == pid[PAGER])
        fatal_error("pager failed");
}

/* Fatal error, kill all children and terminate */
void fatal_error(char *error)
{
    int i;

    fprintf(stderr, "Fatal error occured: %s\n", error);

    for (i = 0; i < sizeof(pid)/sizeof(pid[0]); i++)
    {
        int status, retval;

        retval = waitpid(pid[i], &status, WNOHANG);
        if (retval == 0)
        {
            /* Process is still running */
/*            if (i == PAGER)
                kill(pid[i], 15);
            else
*/                kill(pid[i], 9);
        }
    }

    exit(1);
}

/*
 * pipe_in, any redirect to stdin
 * pipe_in, do you want to redir stdout?, 0 - no, else yes
 * returns outpipe */
int setup(char **argv, int program_index, int pipe_in, int pipe_out)
{
    int retval;
    int pipe_fd[2] = { -1, -1 };

    /* Open new pipe if needed */
    if (pipe_out != 0)
    {
        retval = pipe(pipe_fd);
        if (retval == -1) pipe_error();
    }
    
    /* Fork and execute */
    pid[program_index] = fork();
    if (pid[program_index] == 0) /* Child */
    {
        /* Redir pipe_in to stdin */
        if (pipe_in > 0)
        {
            retval = dup2(pipe_in, STDIN_FILENO);
            if (retval == -1) pipe_error();

            if (pipe_in > 2)
            {
                retval = close(pipe_in);
                if (retval == -1) pipe_error();
            }
        }

        /* Redir stdout to return value of function */
        if (pipe_out != 0)
        {
            /* Open pipe for output */
            retval = dup2(pipe_fd[PIPEIN], STDOUT_FILENO);
            if (retval == -1) pipe_error();

            retval = close(pipe_fd[PIPEIN]);
            if (retval == -1) pipe_error();

            retval = close(pipe_fd[PIPEOUT]);
            if (retval == -1) pipe_error();
        }

        (void) execvp(argv[0], argv);

        if (program_index == PAGER) /* Failed to start pager, try default ones */
        {
            execlp("less", "less", (char *) 0);

            /* If less failed, try more, if more fails, give up */
            execlp("more", "more", (char *) 0);
        }

        exit(200);
    }
    if (pid[program_index] == -1)
    {
        if (pipe_in > 2)
        {
            retval = close(pipe_in);
            if (retval == -1) pipe_error();
        }

        return -2;
    }

    /* Close pipes opened */
    if (pipe_out != 0)
    {
        retval = close(pipe_fd[PIPEIN]);
        if (retval == -1) pipe_error();
    }

    /* If pipe to process was provided, close it. 2 Because we don't close std */
    if (pipe_in > 2)
    {
        retval = close(pipe_in);
        if (retval == -1) pipe_error();
    }

    return pipe_fd[PIPEOUT] != -1 ? pipe_fd[PIPEOUT] : 0;
}
