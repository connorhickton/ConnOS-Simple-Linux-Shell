/* 
 * ConnOS Shell
 * A very basic Linux shell, demonstrating use of multiprocessing and system calls.
 * Improved version of an assignment created for an Operating Systems computer science class.
 * Written by Connor Hickton
 * With help from code snippets from Professor Zastre, the class slides, lectures, and tutorials.
 * Any other sources of help will be cited near the code they helped with.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>



// string-related number limits
#define MAX_ARGS 7
#define MAX_IN_CHARS 80
#define MAX_LINE_LEN 80
#define MAX_PROMPT_LEN 10
#define MAX_DIRS 10


char returnPath[MAX_LINE_LEN];




// getPrompt
// opens .sh360rc and prints the prompt
// File input code is modified, but originally taken from this URL:
// https://www.programiz.com/c-programming/examples/read-file
// No parameters, no return values
void getPrompt() {

    char prompt[MAX_PROMPT_LEN];
    FILE *shrc;

    // exit program if something messes up with file opening
    if ((shrc = fopen(".sh360rc", "r")) == NULL ) {
        
        printf("Error: .sh360rc could not be opened. Please make sure the file exists and is properly formatted. Exiting program.\n");    
        exit(1);
    }

    fscanf(shrc, "%[^\n]", prompt);
    printf("%s ", prompt);
    fclose(shrc);

}




// getFileLoc
// Opens .sh360rc, scans the lines after line 1 to find a path, and checks if the path is valid
// Input: inString, which stores the command given (without any arguments)
// Returns nothing directly, but updates returnPath with the absolute path of the given command

void getFileLoc(char inString[]) {
    char path[MAX_LINE_LEN];
    char fullPath[MAX_LINE_LEN];
    char comm[MAX_LINE_LEN];
    int i;
    FILE *shrc;

    // exit program if something messes up when opening file
    if ((shrc = fopen(".sh360rc", "r")) == NULL ) {
        printf("Error occurred when attempting to open .sh360rc.");
	printf("Please make sure the file exists, and is properly formatted. Exiting program.\n");
        exit(1);
    }


    // start i at 1 to skip the first line (which contains the prompt)
    i = 1;

    while (i < MAX_DIRS) {
        
        if (fgets(path, MAX_LINE_LEN, shrc) == NULL) {
            break;
        }
            
        path[strcspn(path, "\n")] = 0;
        
	// copies contents of path to fullPath
        strcpy(fullPath, path);

	// in case .sh360rc doesn't have the / after the PATH.
	// Two /'s doesn't affect the path, so it's better to be prepared.
        strcat(fullPath, "/");

	// appends inString onto the path to make the fullPath
        strcat(fullPath, inString);

	// I suppose the x_ok command would do, but this works so I won't poke it for now
        if (access(fullPath, F_OK) != -1 && access(fullPath, X_OK) != -1) {
            
            strcpy(returnPath, fullPath);
            break;
        }

        i = i + 1;
        
    } // while

    
    if (i >= MAX_DIRS) {
        printf("The PATH for the given command could not be located. This may cause instability in the program.");
    }

}



// emptyReturnPath
// no inputs, no returns
// Clears the global string returnPath so that the shell can loop without risk of keeping data from the last loop.
void emptyReturnPath() {
    int i;
    for(i=0; i < MAX_LINE_LEN; i = i + 1) {
        returnPath[i] = 0;
    }
}



// main
// way too big, and should be broken up into many more functions
// But it mostly works as is right now, so unfortunately this is how it'll have to stay.
int main() {

    int exitCmd = 0;                    // if "exit" is typed, this changes to 1 and the running loop closes
    //char inString[MAX_IN_CHARS];        // string for collecting input, to parse


    // main running loop
    while (exitCmd == 0) {

        // Variables from Appendix B, by Zastre
        // char *args[] = { "/bin/ls", "-1", NULL };
        char *envp[] = { 0 };
        int pid;
        int status;

        // Variables from Appendix E, by Zastre
        char inString[MAX_IN_CHARS];
        char inStringDup[MAX_IN_CHARS];
        char *token[MAX_ARGS] = {0,0,0,0,0,0,0};
        char *t;
        int  i;
        int  line_len;
        int  num_tokens;


        // print the contents of .sh360rc, then gets input
        getPrompt();
        fgets(inString, MAX_IN_CHARS, stdin);               // read input

        // removes \n from end of line - found at link https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
        inString[strcspn(inString, "\n")] = 0;

        strcpy(inStringDup, inString);

        // Appendix E logic, by Zastre - tokenizing the input
        num_tokens = 0;

        t = strtok(inString, " ");

        while (t != NULL && num_tokens < MAX_ARGS) {

            token[num_tokens] = t;
            num_tokens++;
            t = strtok(NULL, " ");

        }

        /* Note that an address stored in some token[i] is actually an address
        * to a char location within the input array.
        */
        for (i = 0; i < num_tokens; i++) {
            // printf("%d: %s\n", i, token[i]);        // for debugging. todo: remove
        } 


        // if exit is typed in, close the loop and program
        if(strcmp(inString, "exit") == 0) {               // exit condition
            exitCmd = 1;            
            break;                                          // this makes exitCmd redundant, but feels more elegant. May change later

        } else if (strcmp(inString, "") == 0) {             // the program would seg fault if given no commands, this fixes that
            continue;
        
        // checks for special case if Output Redirect (OR) command is used
        } else if (strcmp(token[0], "OR") == 0) {           

            char *ORToken[MAX_ARGS] = {0,0,0,0,0,0,0};
            char *cleanTok[MAX_ARGS] = {0,0,0,0,0,0,0};
            // Appendix C variables, by Zastre
            int pid, fd;
            int status;

            int outTokID = -1;
            

            int found = 0;                                  // becomes 1 if/when the "->" token is found

            num_tokens = 0;

            // Heavily modified Appendix E logic
            t = strtok(inStringDup, " ");
            t = strtok(NULL, " ");                          // skip first one since it's just OR

            while (t != NULL && num_tokens < MAX_ARGS) {

                ORToken[num_tokens] = t;                    // create new token array that doesn't have the OR stamped onto the front

                if(strcmp(ORToken[num_tokens], "->") != 0 && found != 1) {
                    cleanTok[num_tokens] = t;                                   // create token array that ignores everything but the basic command and args
                    // printf("%s was added to cleanTok!\n", cleanTok[num_tokens]);
                } else {
                    found = 1;
                    if (outTokID == -1) {
                        outTokID = i;           // if outTokID hasn't been touched yet, set it to i
                                                // THIS SHOULD BE A BUG BUT IF I CHANGE IT THE PROGRAM BREAKS
                                                // i ISN'T EVEN THE ITERATOR IN THIS LOOP!
                    }
                    
                }
                
                num_tokens++;
                t = strtok(NULL, " ");
                
            }


            // prints error and loops without executing if no redirect token was found.
            if (outTokID > 0) {
               // printf("Redirect Token Found at Index %i\n", outTokID);  // todo: remove when done
            } else {
                printf("Error: No Redirect Token Found. Please check your formatting.\n");
                continue;
            }

            getFileLoc(cleanTok[0]);


            // prints error if no PATH was found for the given command
            // this is me repeating myself, and don't worry, I hate it too
            // todo: remove that comment
            if (returnPath[0] == '\0') {
                printf("%s: Command not found.\n", cleanTok[0]);
                continue;
            }

            

            // Appendix C logic, by Zastre
            if ((pid = fork()) == 0) {
                // printf("child: about to start...\n");
                // printf("ORToken: %s\n", ORToken[outTokID - 2]);
                fd = open(ORToken[outTokID -2], O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
                if (fd == -1) {
                    fprintf(stderr, "cannot open %s for writing\n", ORToken[outTokID-2]);
                    exit(1);
                }
                dup2(fd, 1);
                dup2(fd, 2); 
                execve(returnPath, cleanTok, envp);
                printf("Error: Child process reached somewhere it shouldn't be.\n");
            }

            //printf("parent: waiting for child to finish...\n");
            waitpid(pid, &status, 0);
            //printf("parent: child is finished.\n");




            
            

        } else if (strcmp(token[0], "PP") == 0) {           // checks for special case if Pipe command (PP) is used

            char *PPTokenA[MAX_ARGS] = {0,0,0,0,0,0,0};
            char *PPTokenB[MAX_ARGS] = {0,0,0,0,0,0,0};
            char *cleanTok[MAX_ARGS] = {0,0,0,0,0,0,0};

            int found = 0;                                  // becomes 1 if/when the "->" token is found
            int outTokID = -1;

            num_tokens = 0;

            t = strtok(inStringDup, " ");
            t = strtok(NULL, " ");                          // skip first one since it's just PP

            // this is heavily modified Appendix E logic.
            while (t != NULL && num_tokens < MAX_ARGS) {

		// create new token array that doesn't have the PP stamped onto the front
                cleanTok[num_tokens] = t;

                if(strcmp(cleanTok[num_tokens], "->") != 0 && found != 1) {
                    
                    
                    PPTokenA[num_tokens] = t; 
                    
                    
                } else {
                    found = 1;
                    
		    // if outTokID hasn't been touched yet, set it to i
                    if (outTokID == -1) {
                        outTokID = num_tokens;
                        
                    } else {
                        PPTokenB[-1 + num_tokens - outTokID] = t;
                        // printf("PPB: %s & i: %i\n", PPTokenB[-1 + num_tokens - outTokID], (-1 + num_tokens - outTokID));                                   // if the token is after the "->" then add it to the second array
                    }
                    
                }
                
                num_tokens++;
                t = strtok(NULL, " ");
                
            }

            // prints error and loops without executing if no redirect token was found.
            if (outTokID > 0) {
               // printf("Redirect Token Found at Index %i\n", outTokID);  // todo: remove when done
            } else {
                printf("Error: No Redirect Token Found. Please check your formatting.\n");
                continue;
            }

            






            // Appendix D variables, by Zastre
            int status;
            int pid_head, pid_tail;
            int fd[2];

            // Appendix D logic, by Zastre
            pipe(fd);
            
            /* After this point, we now have a pipe in the form of two file
            * descriptors. The end of the pipe from which we can read is
            * fd[0], and the end of the pipe to which we write is fd[1].
            *
            * Any child created after this point will have these file descriptors
            * for the pipe.
            */
            // printf("PPA Before getFileLoc: %s\n", PPTokenA[0]);
            getFileLoc(PPTokenA[0]);

            // prints error if no PATH was found for the given command
            // this is me repeating myself, and don't worry, I hate it too
            // todo: remove that comment
            if (returnPath[0] == '\0') {
                printf("%s: Command not found.\n", PPTokenA[0]);
                continue;
            }


            // printf("parent: setting up piped commands...\n");
            if ((pid_head = fork()) == 0) {
                // printf("child (head): re-routing plumbing; STDOUT to pipe.\n");
                dup2(fd[1], 1);
                close(fd[0]);
                execve(returnPath, PPTokenA, envp);
                printf("Error: Child Reached Somewhere it Shouldn't Be.\n");
            }

            emptyReturnPath();

            // printf("PPB Before getFileLoc: %s\n", PPTokenB[0]);
            getFileLoc(PPTokenB[0]);

            // prints error if no PATH was found for the given command
            // this is me repeating myself, and don't worry, I hate it too
            // todo: remove that comment
            if (returnPath[0] == '\0') {
                printf("%s: Command not found.\n", PPTokenB[0]);
                continue;
            }

            if ((pid_tail = fork()) == 0) {
                //printf("child (tail): re-routing plumbing; pipe to STDIN.\n");
                dup2(fd[0], 0);
                close(fd[1]);
                execve(returnPath, PPTokenB, envp);
                printf("Error: Child Reached Somewhere it Shouldn't.\n");
            }

            /* One last detail: At this point we are running within code used
            * by the parent. The parent does *not* need the pipe after the
            * children are started, but the file system does not know this
            * and so detects additional links to the open pipe. Therefore the
            * parent will close its copies of the file descriptors. (Note that
            * the file system will only close the pipe for good once all processes
            * having the pipe in the file-descriptor table have either terminated
            * or have closed the pipe.
            */
        
            close(fd[0]);
            close(fd[1]);

            //printf("parent: waiting for child (head) to finish...\n");
            waitpid(pid_head, &status, 0);
            //printf("parent: child (head) is finished.\n");

            //printf("parent: waiting for child (tail) to finish...\n");
            waitpid(pid_tail, &status, 0); 
            //printf("parent: child (tail) is finished.\n");



            // printf("Have yet to implement PP\n");           // TODO
            // continue;

        } else {

            getFileLoc(token[0]);

            // prints error if no PATH was found for the given command
            if (returnPath[0] == '\0') {
                printf("%s: Command not found.\n", token[0]);
                continue;
            }

            // printf("This is returnPath: %s\n", returnPath);      // todo: remove when done

            // Code from Appendix B, by Zastre
            if ((pid = fork()) == 0) {

                //printf("child: about to start...\n");               // todo: remove when done

                /* todo remove this too
                for (i = 0; i < MAX_ARGS; i = i + 1) {
                    printf("for loop output: %s\n",token[i]);
                }
                */

                if ((execve(returnPath, token, envp)) == -1) {

                    printf("Error: Child reached somewhere it shouldn't have.\n");                // todo: remove when done
                }
                //printf("child: SHOULDN'T BE HERE.\n");              // todo: output errors to file stderr
                
            } // if


            while (wait(&status) > 0) {


            } // while


        } // else


        emptyReturnPath();      // clears the returnPath variable for the next loop, in case it's different next time

    } // while (main running loop)



    return 0;           // close program
}
