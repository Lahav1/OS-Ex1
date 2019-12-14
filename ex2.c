// Lahav Amsalem, 204632566

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <malloc.h>
#include <stdlib.h>

#define MAX_SIZE 512

struct job {
    // job's process id.
    int pid;
    // job's array of arguments.
    char** args;
};

void removeEndline(char* buffer);
void parseCommand(char* buffer, char** args);
char** copyArgs(char** args);
void executeCommand(char** args, int onBackground, struct job* jobs);
int onBackground(char** args);
void addJob(int pid, char** args, struct job* jobs);
void removeJob(int p, struct job* jobs);
void printJob(int p, struct job* jobs);

/*
 * Main function creates the jobs array, then takes commands as input from user, parses them and executes.
 */
int main() {
    struct job jobs[MAX_SIZE] = {};
    while(1) {
        // create buffers.
        char buffer[MAX_SIZE] = {};
        char* args[MAX_SIZE] = {};
        printf("> ");
        // get a command line as an input from the user. remove newline char and parse the command.
        fgets(buffer, sizeof(buffer), stdin);
        removeEndline(buffer);
        parseCommand(buffer, args);
        // check if the command should be executed on background (in case it ends with "&").
        int bgFlag = onBackground(args);
        // if jobs command had been chosen, scan job list.
        if (strcmp(args[0], "jobs") == 0) {
            // find the jobs' count.
            int jobsNum;
            for (jobsNum = 0; jobsNum < sizeof(jobs); jobsNum++){
                if (jobs[jobsNum].args == NULL) {
                    break;
                }
            }
            int i;
            // for each job, check if it is still running.
            for (i = 0; i < jobsNum; i++) {
                int status;
                pid_t returnPid = waitpid(jobs[i].pid, &status, WNOHANG);
                // if the process is not running, remove it from jobs array.
                if (returnPid != 0) {
                    removeJob(jobs[i].pid, jobs);
                }
            }
            for (i = 0; i < jobsNum; i++) {
                int status;
                pid_t returnPid = waitpid(jobs[i].pid, &status, WNOHANG);
                // if the process is still running, print it's information.
                if (returnPid == 0) {
                    printJob(jobs[i].pid, jobs);
                }
            }
        }
        // if cd command had been chosen, go to requested directory.
        else if (strcmp(args[0], "cd") == 0) {
            int pid = getpid();
            printf("%d\n", pid);
            // if second arg is empty or ~, change directory to "home".
            if (args[1] == NULL) {
                chdir(getenv("HOME"));
            } else if ((strcmp(args[1], "~") == 0) || (strcmp(args[1], "~/") == 0)) {
                chdir(getenv("HOME"));
            } else if (args[1][0] == '"') {
                // point to the address after opening '"'.
                char* temp = &buffer[4];
                char path[MAX_SIZE];
                int i = 0;
                // accumulate all the chars until closing '"'.
                while (temp[i] != '"') {
                    path[i] = temp[i];
                    i++;
                }
                path[i] = '\0';
                chdir(path);
                // if next arg is "-", go to previous directory.
            } else if (strcmp(args[1], "-") == 0){
                char previousPath[MAX_SIZE];
                getcwd(previousPath, sizeof(previousPath));
                if (previousPath != NULL) {
                    int i = (int) (strlen(previousPath) - 1);
                    // start from the end and remove all the current folder from path.
                    while (previousPath[i] != '/') {
                        previousPath[i] = '\0';
                        i--;
                    }
                    // cd to previous folder.
                    chdir(previousPath);
                }
            } else {
                chdir(args[1]);
            }
        }
            // if "exit" command had been chosen, break the while loop and end the program.
        else if (strcmp(args[0], "exit") == 0) {
            break;
        }
        // if no special command had been chosen, execute it.
        else {
            executeCommand(args, bgFlag, jobs);
        }
    }
    return 0;
}

/*
 * Replace '\n' with '\0'.
 */
void removeEndline(char* buffer) {
    if (buffer[strlen(buffer) - 1] == '\n') {
        buffer[strlen(buffer) - 1] = '\0';
    }
}

/*
 * Duplicate the argument list to get new address and use the exact necessary size.
 */
char** copyArgs(char** args) {
    // count the args.
    int argsNum = 0;
    int i;
    while (args[argsNum] != NULL) {
        argsNum++;
    }
    // allocate memory for the copied args array.
    char** newArgs = malloc(argsNum * sizeof(char*));
    // if allocation failed, print a message and exit.
    if (newArgs == NULL) {
        printf("Failed allocating memory.");
        exit(1);
    }
    // add each args in "args" to "newArgs".
    for (i = 0; i < argsNum; i++) {
        newArgs[i] = strdup(args[i]);
    }
    return newArgs;
}

/*
 * Check if a functions runs on background (inserted &) or not.
 */
int onBackground(char** args) {
    // find last arg's index.
    int lastIndex;
    for (lastIndex = 0; lastIndex < sizeof(args); lastIndex++){
        if (args[lastIndex] == '\0') {
            break;
        }
    }
    // if last arg is "&", remove it and return true.
    if (strcmp(args[lastIndex - 1], "&") == 0) {
        args[lastIndex - 1] = NULL;
        return 1;
    }
    return 0;
}

/*
 * Parse the given command and put it in a char* args array.
 */
void parseCommand(char* buffer, char** args) {
    char* token = NULL;
    int i = 0;
    token = strtok(buffer, " ");
    // separate each string by space char.
    while (token != NULL) {
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
}

/*
 * Get the parsed command and execute it.
 */
void executeCommand(char** args, int onBackground, struct job* jobs) {
    // fork the current process.
    pid_t pid = fork();
    // error forking the process.
    if (pid == -1) {
        printf("Failed forking.");
        return;
    }
    // if pid = 0, it's the child's preocess
    else if (pid == 0) {
        // replace the process with the new command execution.
        if ((execvp(args[0], args) < 0)) {
            fprintf(stderr, "Error in system call\n");
        }
        return;
    }
    // if pid > 0, it's the father's process
    else {
        printf("%d\n", pid);
        if (onBackground) {
            addJob(pid, args, jobs);
            return;
        // keep running and wait for the child's process to end.
        } else {
            int childpid;
            waitpid(pid, &childpid, 0);
            return;
        }
    }
}

/*
 * Add a new job to the job list.
 */
void addJob(int pid, char** args, struct job* jobs) {
    // find the last job's index.
    int lastIndex;
    for (lastIndex = 0; lastIndex < sizeof(jobs); lastIndex++){
        if (jobs[lastIndex].args == NULL) {
            break;
        }
    }
    // create a new job and add it to the array.
    struct job j = {pid, copyArgs(args)};
    jobs[lastIndex] = j;
}

/*
 * Remove a job from the job list.
 */
void removeJob(int p, struct job* jobs) {
    int arraySize = 0;
    int deleteIndex = 0;
    // find the last job's index.
    for (arraySize = 0; arraySize < sizeof(jobs); arraySize++){
        if (jobs[arraySize].args == NULL) {
            break;
        }
    }
    // find the deleted job's index.
    for (deleteIndex = 0; deleteIndex < sizeof(jobs); deleteIndex++){
        if (jobs[deleteIndex].pid == p) {
            break;
        }
    }
    int i;
    // free the arg strings.
    int argsNum = sizeof(jobs[deleteIndex].args) / sizeof(char*);
    for (i = 0; i < argsNum; i++) {
        free((jobs[deleteIndex].args)[i]);
    }
    // free the allocated memory.
    free(jobs[deleteIndex].args);
    // from the deleted index and on, move each job 1 cell left.
    for (i = deleteIndex; i < arraySize - 1; i++) {
        jobs[i] = jobs [i + 1];
    }
    // null the last job's index.
    jobs[arraySize - 1].pid = 0;
    jobs[arraySize - 1].args = NULL;
}

/*
 * Print the details of a specific job from the job list.
 */
void printJob(int p, struct job* jobs) {
    // find the index of the printed process by its' pid.
    int printedIndex;
    for (printedIndex = 0; printedIndex < sizeof(jobs); printedIndex++){
        if (jobs[printedIndex].pid == p) {
            break;
        }
    }
    char** tempArgs = jobs[printedIndex].args;
    // if the args of the requested job are empty, return without printing.
    if (tempArgs == NULL){
        return;
    }
    // print the pid.
    printf("%d ", jobs[printedIndex].pid);
    fflush(stdout);
    int i;
    for (i = 0; i < sizeof(tempArgs); i++){
        // when reached an empty arg (in case the array is not full), break the loop.
        if (tempArgs[i] == NULL) {
            break;
        }
        // if arg is not empty, print it.
        printf("%s ", tempArgs[i]);
        fflush(stdout);
    }
    printf("\n");
}
