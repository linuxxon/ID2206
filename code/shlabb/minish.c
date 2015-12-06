/* minish - MiniShell
 * Minimal shell program for course ID2206 at KTH
 * By Rasmus Linusson
 * Date: 26/11-2015
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "digenv.h"

/* Define limits of command line */
#define MAX_LENGTH 71
#define MAX_ARGS 6

/* Define returnvalues for run_builtins */
#define B_EXIT 1
#define B_SUCCESS 0
#define B_FALIURE -1

/* Define return values for running programs */
#define IS_ERROR < 0
#define INVALIDARGS -1
#define PIPEERROR -2
#define FORKERROR -3

typedef struct timeval s_time;

void print_promt();

int run_program(char**);
int run_program_block(char**);
int parse_command(char *, char**);
int builtin(char **);
int run_builtin(char **);
int background(char**);
int check_background_procs();

char *BUILTINS[] = { "exit", "cd", "digenv" };
char *USER;

/* signal_handler - Handler for all your signal needs
 * Parameters:
 *      int signal - Number of the signal that was cought
 */
void signal_handler(int signal)
{
    if (signal == SIGINT)
    {
        printf("\n");
        print_promt();
        fflush(stdout);
    }
    else if (signal == SIGCHLD) /* Notification that a child has terminated */
    {
        check_background_procs();
        fflush(stdout);
    }
}

/* register_signals - Register proper signals to be handled by the signal
 * handler */
void register_signals()
{
    struct sigaction sig;
    sig.sa_handler = signal_handler;
    sigfillset(&sig.sa_mask);

    if (sigaction(SIGINT, &sig, NULL) == -1)
        perror("Can't register signal");

#ifdef SIGNALDETECTION
    if (retval = sigaction(SIGCHLD, &sig, NULL) == -1)
        perror("Can't register signal");
#endif

}

/* print_promt - Print the shell prompt */
void print_promt()
{
    printf("[%s]$ ", USER);
}

/* main - Start of shell, doesn't care for arguments */
int main(int argc, char* argv[])
{
    int retval;

    char cmd_line[MAX_LENGTH];
    /* Allocate memory for argument vector */
    char **args = (char **) malloc(MAX_ARGS * sizeof(char*));

    /* Register proper signals to handle */
    register_signals();

    /* Set string for promt */
    USER = getenv("USER");

    /* Main loop */
    while (1)
    {

#ifndef SIGNALDETECTION
        /* Check if any background processes has exited */
        retval = check_background_procs();
        if (retval == -1 && errno != ECHILD)
            perror("wait failed");
#endif

        /* Print a new promt */
        print_promt();

        /* Fetch new command from terminal */
        if (fgets(cmd_line, MAX_LENGTH, stdin) != NULL) /* Success */
        {
            /* Parse user command */
            retval = parse_command(cmd_line, args);

            if (retval == 1) /* Empty cmd */
                continue; /* Restart loop, for checking background etc.. */

            /* If the command is available as a builtin, use that */
            if (builtin(args))
            {
                retval = run_builtin(args);
                if (retval == B_EXIT) /* EXIT */
                {
                    retval = check_background_procs();
                    /* retval == 0 means no error for having no children were
                     * returned */
                    if (retval == 0)
                        fprintf(stderr, "Background processes are still running,"\
                                        "close them before exiting!\n");
                    else
                        break;
                }
                else if (retval == B_FALIURE) /* Something went wrong */
                    fprintf(stderr, "Command, %s, could not be executed\n", args[0]);
            }
            else /* Run a command */
            {
                /* Run in the background */
                if (background(args))
                {
                    run_program(args);
                }
                else /* Run blocking */
                {
                    retval = run_program_block(args);
                    if (retval IS_ERROR)
                    {
                        if (retval == INVALIDARGS)
                            fprintf(stderr, "Invalid argument\n");
                        else if (retval == PIPEERROR)
                            fprintf(stderr, "Pipe error\n");
                        else if (retval == FORKERROR)
                            fprintf(stderr, "Fork error\n");
                    }
                }
            }
        }
    }

    /* Free memmory */
    free(args);
    
    return 0;
}

/* run_program - Run a program with argument vector in background. Return a pid
 * to the started process. (NON BLOCKING)
 * Parameters:
 *      char **argv - argument vector. First element is path of program
 *
 * Returns:
 *      int > 0 - Pid of new process running command
 *      int < 0 - Errors regarding executing the command
 */
int run_program(char **argv)
{
    int pid = fork();
    if (pid == 0) /* Child */
    {
        (void) execvp(argv[0], argv);
        exit(255);
    }
    if (pid == -1)
        return FORKERROR;

    fprintf(stderr, "Started %s with pid %i\n", argv[0], pid);

    return pid;
}

/* run_program_block - Run a program according to an argument vector, blocking until
 * return 
 * Arguments:
 *      char **argv - argumentvector as usual, first element is the path to the
 *      program to be executed
 * Returns:
 *      int - exit status of program run. Negative returns are errors, positive
 *          are return values of program
 */
int run_program_block(char **argv)
{
    int retval, status, pid;

    s_time start, end;
    time_t run_time;

    retval = gettimeofday(&start, NULL);
    pid = run_program(argv);


    /* Check retval after forking as not to skew results */
    if (retval == -1)
        fprintf(stderr, "Could not get time of day\n");

    /* Wait until child has exited */
    do 
        waitpid(pid, &status, 0);
    while (!WIFEXITED(status) && !WIFSIGNALED(status));
    retval = gettimeofday(&end, NULL);
    if (retval == -1)
        fprintf(stderr, "Could not get time of day\n");

    /* Calculate elapsed time in milliseconds */
    run_time = end.tv_sec*1e6+end.tv_usec - start.tv_sec*1e6 + start.tv_usec;
    run_time /= 1e3;

    fprintf(stderr, "Pid %d finished executing after %li ms\n", pid, run_time);
    
    return 255 == WEXITSTATUS(status) ? INVALIDARGS : WEXITSTATUS(status);
}

/* parse_command - Parse command into string array, separate arguments by
 * whitespace.
 *
 * Important - command_line will be modified!!
 *
 * Parameters:
 *      char *command_line - String to parse for command
 *      char **args - Array to place parsed string in
 *
 *  Return:
 *      int 1 if empty
 *      int 0 if successful
 *      int -1 if nothing could be parsed
 */
int parse_command(char *command_line, char **args)
{
    int i;
    char *retval; /* Pointer to start of substring */
    char *token = " ";

    /* Remove character thurst upon the return key */
    token = "\n";
    retval = strtok(command_line, token);
    if (retval)
        retval = "\0";
    token = " ";

    if (strlen(command_line) == 1)
        return 1;

    /* Separate arguments into separte strings */
    retval = strtok(command_line, token);
    if (retval == NULL)
    {
        args[0] = command_line;
    }

    i = 0;
    do
    {
        args[i++] = retval;
    } while ((retval = strtok(NULL, token)) && i < MAX_ARGS -1);

    /* As args is used with execvp, the last argument needs to be a null
     * terminator */
    args[i] = (char *) 0; 

    return 0;
}

/* builtin - Check if arguments match a builtin command
 * Parameters:
 *      char ** args - Array of pointers to program arguments
 *
 * Returns:
 *      int 0 - Not a builtin command
 *      int 1 - Arguents descripte a builtin command
 */
int builtin(char **args)
{
    if (args[0] != NULL)
    {
        int i;
        for (i = 0; i < sizeof(BUILTINS)/sizeof(BUILTINS[0]); i++)
        {
            if (strcmp(args[0], BUILTINS[i]) == 0)
                return 1;
        }
    }
    return 0;
}

/* background - Check if program should run as a background process
 * Parameters:
 *      char **args - Array of pointers to program arguments
 *
 * Returns:
 *      int 0 - False
 *      int 1 - True
 */
int background(char **args)
{
    int i;
    int last_arg = -1;
    int last_index = -1;
    char last_char;

    /* Find last argument */
    for (i = 0; i < MAX_ARGS; i++)
    {
        if (args[i] == NULL)
        {
            last_arg = i-1;
            break;
        }
    }
    if (last_arg == -1)
        last_arg = MAX_ARGS - 1;

    /* Check if last argument is ampersand alone */
    if (strcmp(args[last_arg], "&") == 0)
    {
        /* Remove ampersand */
        args[last_arg] = (char *) 0;
        return 1;
    }

    /* If no lone ampersand - check the last character of the string */
    last_index = strlen(args[last_arg]) - 1;
    last_char = args[last_arg][last_index];

    if (last_char == '&')
    {
        /* Remove ampersand */
        args[last_arg][last_index] = '\0';
        return 1;
    }
    return 0;

}

/* run_builtin - Execute a builtin command
 * Parameters:
 *      char ** args - Array of pointers to program arguments
 *
 * Returns:
 *      int  0 - Success
 *      int -1 - Failure
 *      int  1 - Stop shell
 */
int run_builtin(char **args)
{
    int retval;

    if (strcmp(args[0], "exit") == 0)
        return B_EXIT;
    else if (strcmp(args[0], "cd") == 0)
    {
        if (args[1] == NULL)
        {
            args[1] = getenv("HOME");
            if (args[1] == NULL)
                return -1; /* Couldn't do anything usefull */
        }
        retval = chdir(args[1]);

        if (retval == 0)
        {
            /* Change the PWD, uses allocated memory so fixit */
            return 0;
        }
        return -1;
    }
    else if (strcmp(args[0], "digenv") == 0)
    {
        int num_args, i;
        num_args = MAX_ARGS-1;
        for (i = 0; i < MAX_ARGS; i++)
        {
            if (args[i] == NULL)
            {
                num_args = i;
                break;
            }
        }

        /* Call digenv */
        return digenv(num_args, args);
    }
    return -1;
}

/* check_background_procs - Check if background processes have finished/crashed
 * and update b_pid array accordingly and print information to stderr
 */
int check_background_procs()
{
    int pid, status, num = 0;

    while ((pid = waitpid(-1, &status, WNOHANG)) != 0)
    {
        if (pid <= -1) /* Error */
            return pid;
        else /* A process has changed status */
        {
            /* Only do prints if process terminated */
            if (WIFEXITED(status))
            {
                int exit_status = WEXITSTATUS(status);
                printf("Process %d has exited with status %d\n", pid, exit_status);
            }
            else if (WIFSIGNALED(status))
            {
                int signal = WTERMSIG(status);
                printf("Process %d was terminated by signal %d\n", pid, signal);
            }
            num++;
        }
    }
    return pid < 0 ? pid : num;
}
