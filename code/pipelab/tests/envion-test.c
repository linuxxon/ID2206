/* environ-test.c - test för att se ifall pekare mellan parent- och
 * child-processer delar samma environment och därmed kan kommunicera med denna
 */

#include <sys/types.h> /* pid_t */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern char **environ;

pid_t childpid;

int main()
{
    /* print environment */
    printf("env[%i] = %s\n", 7, environ[7]);

    environ[7] = "PAGER=ls  ";

    childpid = fork();
    if (childpid == 0)
    {
        /* Om child */
        sleep(2);
        printf("C - Env[%i] = %s\n", 7, environ[7]);
    }
    else
    {
        /* Om parent */
        if (childpid == -1)
        {
            perror("Could not fork");
            exit(1);
        }
        else
        {
            environ[7] = "PAGER=more";
            sleep(2);
        }
    }
    return 0;
}
