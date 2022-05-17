// C Program to design a shell in Linux, Project 2

// CS 4348.004
// David Nguyen : dxn180015
// Dueñes Gomez : lxd180007
// Luiz Astorga : laa180001

// fixes implicit declaration of functions error (getline, strdup)
#define _GNU_SOURCE 
// fd for stdin
#define READ_END 0
// fd for stdout
#define WRITE_END 1
// used for arrays
#define MAX 100

#include <stdio.h> // fgets() , stdin, stderr
#include <stdlib.h> // basic
#include <string.h> // strcmp()
#include <sys/types.h> // pid_t
#include <sys/wait.h> // wait()
#include <unistd.h> // getpid(), execvp()
#include <sys/stat.h> // mkdir
#include <fcntl.h> // visible from sys/stat.h, unistd.h

// prototypes

/**
printShellPromt : this simply prints out the directory and prompts the user like a command
prompt would.
**/
void printShellPrompt();

/**
tokenizeInput: Takes in the user input, which is a shell command, and tokenizes it. This
Will count the number of arguments and store the arguements in an array.
**/
int tokenizeInput(char* input, char* argv[MAX]);

/**
checkPipes: parses the user input and documents each '|' location. Then sets the
location to where '|' was to NULL
**/
void checkPipes(char* argv[MAX], int* numPipes, int pipeIndexes[MAX], int numArgs);

/**
checkInOut : parses the user input and documents the location of each '<', '>', and '>>'.
then sets the location to where I/O commands were to NULL. Also determines to
which command section the redirection operations are applied to. Each section is separated by a pipe '|'.
**/
void checkInOut(char* argv[MAX], int* numInOut, int inOutIndexes[MAX], int numArgs, int inOutType[MAX], int pipeIndexes[MAX], int inOutSection[MAX]);

/**
checkCustomCommands: for commands that execvp doesn't cover, a custom approach was taken
to cover for that, such as 'cd', 'exit', 'quit', ctrl + d, pushd, popd, or dirs
**/
int checkCustomCommands(char* argv[MAX], char* dirs[MAX], int* numDirs);

/**
executeCommand: This is the main program that parses through the pipe(s) and redirection(s)
operetors to execute the commands
**/
void executeCommand(char* argv[MAX], int numPipes, int pipeIndexes[MAX], int numInOut, int inOutIndexes[MAX], int inOutType[MAX], int inOutSections[MAX]);



// Driver function
int main() {
    // holds user input and arguments
    char inputBuffer[MAX];
    char* argv[MAX];

    // for pushd, popd, dirs stack
    char* dirs[MAX];
    int numDirs = 0;
    dirs[0] = NULL;

    // infinite loop until the user chooses to exit
    while (1) {
        // reset start of both arrays to null
        inputBuffer[0] = '\0';
        argv[0] = NULL;

        // prompts user for input
        printShellPrompt();

        int numPipes = 0;
        int pipeIndexes[MAX] = { 0 };

        // holds how many I/O operations are in the entire input
        int numInOut = 0;
        // holds the index of each of the I/O operators
        int inOutIndexes[MAX] = { 0 };
        // hold the "type" ie <, >, >> of each I/O operator
        int inOutType[MAX] = { 0 };
        // holds what section the I/O operator is being applied to 
        int inOutSection[MAX] = { 0 };

        // gets user input
        if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != 0) {
            // tokenizes input string
            int numArgs = tokenizeInput(inputBuffer, argv);

            // go to next loop interration when user enters \n only
            if (numArgs == 0)
                continue;

            // check for pipes
            checkPipes(argv, &numPipes, pipeIndexes, numArgs);

            // check for I/O
            checkInOut(argv, &numInOut, inOutIndexes, numArgs, inOutType, pipeIndexes, inOutSection);

            // if no custom user command, execute command
            if (!checkCustomCommands(argv, dirs, &numDirs))
                executeCommand(argv, numPipes, pipeIndexes, numInOut, inOutIndexes, inOutType, inOutSection);
        }
        else
        {
            // exits program when users enter ctrl + d (Null/EOF)
            if (strcmp(inputBuffer, "\0") == 0) {
                printf("\nGoodbye\n");
                exit(0);
            }
        }
    }
    return 0;
}

// print shell prompt
void printShellPrompt() {
    // get username
    char* username = getenv("USER");

    // get current working directory
    char currWorkDir[MAX];
    getcwd(currWorkDir, sizeof(currWorkDir));

    // print shell
    printf("@%s:%s$ ", username, currWorkDir);
}

// tokenizes the input into words
int tokenizeInput(char* input, char* argv[MAX]) {
    int i = 0;
    char* words[MAX];
    char* piece;

    // places next word into piece
    piece = strtok(input, " \n");

    // transfer word pieces into words[]
    while (piece != NULL) {
        words[i++] = strdup(piece);
        //printf("|%s|\n", piece);
        piece = strtok(NULL, " \n");
    }

    // each word copied to argv[] (index 0 contains command)
    int j;
    for (j = 0; j < i; j++)
        argv[j] = words[j];

    // NULL terminate argv array
    argv[i] = NULL;

    return i;
}

// finds the number and location of pipes
void checkPipes(char* argv[MAX], int* numPipes, int pipeIndexes[MAX], int numArgs)
{
    // first command index
    pipeIndexes[0] = 0;

    // finds number of pipes
    int i;
    for (i = 0; i < numArgs; i++) {
        if (strcmp(argv[i], "|") == 0) {
            // set | to null
            argv[i] = NULL;

            // increment numPipes and use that as the index
            // in pipeIndexes array and set value there to i + 1 
            // (where command word would be at)
            numPipes[0] = numPipes[0] + 1;
            pipeIndexes[numPipes[0]] = i + 1;
        }
    }

}

// finds the location of I/O redirection, saves their index, type, and applied section
void checkInOut(char* argv[MAX], int* numInOut, int inOutIndexes[MAX], int numArgs, int inOutType[MAX], int pipeIndexes[MAX], int inOutSection[MAX])
{
    // first command index
    inOutIndexes[0] = 0;
    // starts at first section 0, ie cdm0 would be section 0 in the input cdm0 | cdm1 | ... | cdmn
    int section = 0;

    // finds number of I/O operators
    int i, j;
    for (i = 0, j = 1; i < numArgs; i++) {
        // checks if there is an I/O in input, skips indexes where pipes are located at do avoid strcmp(NULL)
        // and determines which pipe section the redirection operators are in
        if (i == pipeIndexes[j] - 1) {
            j++;
            section++;
        }
        else {
            if ((strcmp(argv[i], "<") == 0) || (strcmp(argv[i], ">") == 0) || (strcmp(argv[i], ">>") == 0))
            {
                // keeps track of what TYPE of I/O argument was used with the follwoing values:
                // < : 1, > : 2, >> : 3
                if (strcmp(argv[i], "<") == 0)
                {
                    inOutType[*numInOut] = 1;
                    inOutSection[*numInOut] = section;
                }
                else if (strcmp(argv[i], ">") == 0)
                {
                    inOutType[*numInOut] = 2;
                    inOutSection[*numInOut] = section;
                }
                else
                {
                    inOutType[*numInOut] = 3;
                    inOutSection[*numInOut] = section;
                }

                // set I/O to null
                argv[i] = NULL;

                // increment numInOut and use that as the index
                // in inOutIndexes array and set value there to i + 1 
                // (where command word would be at)
                (*numInOut)++;
                inOutIndexes[*numInOut] = i + 1;
            }
        }
    }
}

// checks for custom commands
int checkCustomCommands(char* argv[MAX], char* dirs[MAX], int* numDirs) {
    int hasCustomCommand = 0;

    // doesn't catch ctrl + d for some reason
    // exits program when users enter ctrl + d (Null/EOF)
    if (argv[0] == NULL || strcmp(argv[0], "\0") == 0) {
        printf("\nGoodbye\n");
        exit(0);
    }

    // exits program when user enters exit or quit
    if (argv[1] == NULL) {
        if (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "quit") == 0) {
            printf("Goodbye\n");
            exit(0);
        }
    }

    // changes the directory in parent when command is 'cd'
    if (strcmp(argv[0], "cd") == 0) {
        hasCustomCommand = 1;
        chdir(argv[1]);
    }

    // pushd
    if (strcmp(argv[0], "pushd") == 0) {
        hasCustomCommand = 1;

        // push current working directory to stack and increment numDirs
        char currWorkDir[MAX];
        dirs[(*numDirs)] = strdup(get_current_dir_name());
        (*numDirs)++;

        // changes directory to user's inputted path
        chdir(argv[1]);

    }

    // popd
    if (strcmp(argv[0], "popd") == 0) {
        hasCustomCommand = 1;

        // nothing in stack
        if ((*numDirs) == 0) {
            printf("popd: directory empty\n");
        }
        // pop stack and chdir
        else {
            (*numDirs)--;
            chdir(dirs[(*numDirs)]);
            //printf("|%s|\n", dirs[(*numDirs)]);
        }
    }

    // dirs
    if (strcmp(argv[0], "dirs") == 0) {
        hasCustomCommand = 1;

        // get current working directory
        char currWorkDir[MAX];
        getcwd(currWorkDir, sizeof(currWorkDir));

        // stack empty
        if ((*numDirs) == 0) {
            // print current directory
            printf("%s\n", currWorkDir);
        }
        // stack not empty
        else {
            // print current directory
            printf("%s ", currWorkDir);

            // print dirs array stack (top to bottom)
            for (int i = (*numDirs) - 1; i >= 0; i--) {
                printf("%s ", dirs[i]);
            }

            printf("\n");
        }
    }

    return hasCustomCommand;
}

// fork and execute command in child process
void executeCommand(char* argv[MAX], int numPipes, int pipeIndexes[MAX], int numInOut, int inOutIndexes[MAX], int inOutType[MAX], int inOutSection[MAX]) {
    int fdRightPipe[2] = { -1, -1 };
    int fdLeftPipe[2] = { -1, -1 };
    pid_t pid;

    // hold file descriptor for inout file and output file
    int fdin;
    int fdout;

    // go through X total number of commands and execute
    int i;
    for (i = 0; i < numPipes + 1; i++) {
        // create pipe if have multible commands and is not last command
        if (i != numPipes) {
            if (pipe(fdRightPipe) == -1) {
                fprintf(stderr, "Pipe creation failed.\n");
                exit(1);
            }
        }

        // split into 2 processes (parent and child have same copy of the pipe)
        pid = fork();

        // check if fork worked
        if (pid < 0) {
            perror("forking error");
            exit(1);
        }

        // child process
        else if (pid == 0) {

            // if is not first command
            if (i != 0) {
                // link file descriptor for std input to read end of left pipe
                dup2(fdLeftPipe[READ_END], 0);

                // close the unneeded descriptor
                close(fdLeftPipe[READ_END]);
            }

            // if have multible commands and is not last command,
            if (i != numPipes) {
                // close read end of right pipe
                close(fdRightPipe[READ_END]);

                // link file descriptor for std output to write end of right pipe
                dup2(fdRightPipe[WRITE_END], 1);

                // close the unneeded descriptor
                close(fdRightPipe[WRITE_END]);
            }

            // get starting index of next command
            int cmdIndex = pipeIndexes[i];

            // checks if a redirection operator is in the current pipe/command section
            // b used to iterate though inOutSection to check if the currect section being
            // exectued, is the same section as where I/O operator was found
            int b;
            for (b = 0; b < numInOut; b++) {
                if (inOutSection[b] == i)
                {
                    // argv[inOutIndexes[i+1]] hold the name of the file
                    // switch input from STDIN to input file if '<' is present
                    if (inOutType[(b)] == 1) {
                        fdin = open(argv[inOutIndexes[(b)+1]], O_RDONLY, 0644);
                        dup2(fdin, 0);
                        close(fdin);

                        // if < and > are in the same command
                        if (numPipes < numInOut) {
                            if (inOutType[(b)+1] == 2) {
                                fdout = open(argv[inOutIndexes[(b)+2]], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                dup2(fdout, 1);
                                close(fdout);
                            }

                            if (inOutType[(b)+1] == 3) {
                                fdout = open(argv[inOutIndexes[(b)+2]], O_WRONLY | O_CREAT | O_APPEND, 0644);
                                dup2(fdout, 1);
                                close(fdout);
                            }
                        }
                    }

                    // if '>' redirection is present
                    if (inOutType[(b)] == 2) {
                        fdout = open(argv[inOutIndexes[(b)+1]], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        dup2(fdout, 1);
                        close(fdout);
                    }

                    // if '>>' redirection is present
                    if (inOutType[(b)] == 3) {
                        fdout = open(argv[inOutIndexes[(b)+1]], O_WRONLY | O_CREAT | O_APPEND, 0644);
                        dup2(fdout, 1);
                        close(fdout);
                    }
                }
            }

            // executes the shell command
            if (execvp(argv[cmdIndex], &argv[cmdIndex]) < 0)
                perror("exec error");

            exit(0); // exits if error
        }

        // parent waits for child process to finish
        else
        {
            wait(NULL);

            // printf("\n%d\n", (*x));
            // close write end of right pipe descriptor in the parent process
            close(fdRightPipe[WRITE_END]);

            // if not first command, close previous left pipe's read end
            if (i != 0)
                close(fdLeftPipe[READ_END]);

            // saves right pipe with only read end open as left pipe
            fdLeftPipe[READ_END] = fdRightPipe[READ_END];
        }
    }
}