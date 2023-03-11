/*
 * @File name (systemcalls.c)
 * @File Description (Redited for AESD Assignment-3 using various systemcalls
 * @Edited by AYSWARIYA KANNAN)
 * @Date (2/04/2023)
 * @Attributions :
 */
#include "systemcalls.h"
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
 */
bool do_system(const char *cmd)
{

    /*
     * TODO  add your code here
     *  Call the system() function with the command set in the cmd
     *   and return a boolean true if the system() call completed with success
     *   or false() if it returned a failure
     */
    if (cmd == NULL) // check if pointer is NULL then returns false
    {
        return false;
    }
    int p = system(cmd); // call system() function
    if (p == -1)         // if it returns -1 means call unsuccessful
    {
        return false;
    }

    return true;
}

/**
 * @param count -The numbers of variables passed to the function. The variables are command to execute.
 *   followed by arguments to pass to the command
 *   Since exec() does not perform path expansion, the command to execute needs
 *   to be an absolute path.
 * @param ... - A list of 1 or more arguments after the @param count argument.
 *   The first is always the full path to the command to execute with execv()
 *   The remaining arguments are a list of arguments to pass to the command in execv()
 * @return true if the command @param ... with arguments @param arguments were executed successfully
 *   using the execv() call, false if an error occurred, either in invocation of the
 *   fork, waitpid, or execv() command, or if a non-zero return value was returned
 *   by the command issued in @param arguments with the specified arguments.
 */

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char *command[count + 1];
    int i;
    for (i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    /*
     * TODO:
     *   Execute a system command by calling fork, execv(),
     *   and wait instead of system (see LSP page 161).
     *   Use the command[0] as the full path to the command to execute
     *   (first argument to execv), and use the remaining arguments
     *   as second argument to the execv() command.
     *
     */
    pid_t pidval = fork(); // to execute a new child process
    if (pidval < 0)        // if negative means child process not created successfully
    {
        perror("fork() unsuccessful");
        exit(1);
    }
    else if (pidval == 0) // on successfull execution returns 0 to child process
    {
        int n = execv(command[0], &command[0]); // do the execution using execv() with the given command
        if (n < 0)                              // negative returns means unsuccessful process
        {
            perror("execv() unsuccessful");
            exit(1);
        }
    }
    else
    {
        int child_process;             // process ID returned to the parent process
        pidval = wait(&child_process); // wait for child process to finish
        if (WIFEXITED(child_process))  // if child process exited sucessfully
        {
            printf("Child process exited with %d status\n", WEXITSTATUS(child_process)); // this checks the exit status
            if (WEXITSTATUS(child_process) == 1)
            {
                return false;
            }
            else if (WEXITSTATUS(child_process) == 0)
            {
                return true;
            }
        }
    }

    va_end(args);

    return true;
}

/**
 * @param outputfile - The full path to the file to write with command output.
 *   This file will be closed at completion of the function call.
 * All other parameters, see do_exec above
 */
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char *command[count + 1];
    int i;
    for (i = 0; i < count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    /*
     * TODO
     *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
     *   redirect standard out to a file specified by outputfile.
     *   The rest of the behaviour is same as do_exec()
     *
     */
    pid_t kidpid;                                                  // for executing system call using execv()
    int c;                                                         // for checking child process status
    int fd = open(outputfile, O_WRONLY | O_TRUNC | O_CREAT, 0644); // opening the outfile where standard out will be redirected
    if (fd < 0)                                                    // in case of an error throws negative value
    {
        perror("open");
        exit(1);
    }
    switch (kidpid = fork()) // creating the child process to do the execution
    {
    case -1: // if it returns -1 means child process failed to be created
        perror("fork");
        exit(1);
    case 0: // if it is returns 0 means child process successfully created
            //  here the copy of the old file descriptor is created with a newfd as specified by user. Here the new fd is the file descriptor of stdout (i.e. 1)
            // This causes all printf() statements to be written in the file referred by the old file descriptor.
        if (dup2(fd, 1) < 0)
        {
            perror("dup2"); // returns -1 means unsuccessful
            exit(1);
        }
        close(fd);
        int n = execv(command[0], &command[0]); // do the execution using execv() with the given command
        if (n < 0)                              // negative returns means unsuccessful process
        {
            perror("execv");
            exit(1);
        }

    default: // process ID returned to the parent process

        kidpid = wait(&c); // wait for child process to finish
        if (WIFEXITED(c))  // if child process exited sucessfully
        {
            printf("Child process exited with %d status\n", WEXITSTATUS(c));
            // this checks the exit status
            if (WEXITSTATUS(c) == 1)
            {
                return false;
            }
            else if (WEXITSTATUS(c) == 0)
            {
                return true;
            }
        }
        close(fd); // close file descriptor
    }
    va_end(args);

    return true;
}
