#include "../include/checkArguments.h"

// check if passing arguments are right. flag = 0 -> jobcommander, flag = 1 -> jobExecutorServer
int checkArguments(int flag, int argc, char const *argv[], char** commandArguments){

    if (flag == 0){
        
        if ( (strcmp(argv[3], "setConcurrency") == 0) && (atoi(argv[4]) == 0)){     //if user types setConcurrency then only a number must follow
            printf("Invalid number of arguments\n");
            return -2;
        }
        else if( ((strcmp(argv[3], "poll") == 0) || strcmp(argv[3], "exit") == 0) && argc > 4){     // if user types poll or exit then nothing must follow
            printf("Invalid number arguments\n");
            return -2;
        }
        else if( (strcmp(argv[3], "stop") == 0) && (argc > 5 || argc < 5)){     //if user types setConcurrency then only a number must follow(jobId)
            printf("Invalid number arguments\n");
            return -2;
        } 
        else if( (strcmp(argv[3], "issueJob") == 0) && (argc < 5)){     //if user types issueJob then at least 1 argument must follow (command)
            printf("Invalid number arguments\n");
            return -2;
        } 
        
        int arraySize = argc - 3;
        if (arraySize == 0){
            printf("Wrong number of parameters");
            return -2;
        }

        //loop through input to place the command/number/jobid in a buffer
        for (int i = 0; i < arraySize; i++) {
            *commandArguments = realloc(*commandArguments, strlen(argv[i+3])+2);
            if (*commandArguments == NULL){
                printf("Memory reallocation failed");
                return -1;
            }

            if (i == 0){
                strcpy(*commandArguments, argv[i+3]);
                strcat(*commandArguments,"!");
            }
             else{
                strcat(*commandArguments, argv[i+3]);
                strcat(*commandArguments, " ");
             }
                
        }
        strcat(*commandArguments, "\0");
    }
    else{
        if (argc > 4 || argc < 4){      // server gets 3 parameters fixed
             printf("Wrong number of parameters");
             return -2;
         }
    }
    return 0;
}