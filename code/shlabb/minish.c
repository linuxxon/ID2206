/* minish - MiniShell
 * Minimal shell program for course ID2206 at KTH
 * By Rasmus Linusson
 * Date: 26/11-2015
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Define limits of background prcs */
#define MAX_BACKGROUND_PROCS 20

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

int run_program(char**);
int run_program_block(char**);
int parse_command(char *, char**);
int builtin(char **);
int run_builtin(char **);
int background(char**);
int check_background_procs();

char *BUILTINS[] = { "exit", "cd" };
char *USER;

int main(int argc, char* argv[])
{
    int retval;

    char cmd_line[MAX_LENGTH];
    char **args = (char **) malloc(MAX_ARGS * sizeof(char*));

    USER = getenv("USER");

    while (1)
    {
        /* Check if any background processes has exited */
        check_background_procs();
        printf("[%s]$ ", USER);

        if (fgets(cmd_line, MAX_LENGTH, stdin) != NULL) /* Success */
        {
            retval = parse_command(cmd_line, args);

            if (retval == 1) /* Empty cmd */
                continue; /* Restart loop, for checking background etc.. */

            /* If the command is available as a builtin, use that */
            if (builtin(args))
            {
                retval = run_builtin(args);
                if (retval == B_EXIT) /* EXIT */
                    break;
                else if (retval == B_FALIURE) /* Something went wrong */
                    fprintf(stderr, "Command, %s, could not be executed\n", args[0]);
            }
            else /* Run a command */
            {
                if (background(args)) /* Run in the background */
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
        else
            perror("Failed to read terminal input");
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

    return pid;
}

/* run_program_block - Run a program according to an argument vector, blocking until
 * return
 * Arguments:
 *      char **argv - argumentvector as usual, first element is the path to the
 *      program to be run
 * Returns:
 *      int - exit status of program run. Negative returns are errors, positive
 *          are return values of program
 */
int run_program_block(char **argv)
{
    int status;

    int pid = fork();
    if (pid == 0) /* Child */
    {
        (void) execvp(argv[0], argv);
        exit(255);
    }
    if (pid == -1) /* Parent */
        return FORKERROR;

    /* Wait until child has exited */
    do 
        waitpid(pid, &status, 0);
    while (!WIFEXITED(status) && !WIFSIGNALED(status));

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

/* background - Check if program chould run as a background process
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
        return 1;
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
        if (pid == -1) /* Error */
            return -1;
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
    return num;
}
