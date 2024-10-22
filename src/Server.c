#include "../include/checkArguments.h"
#include<sys/wait.h>
#include<sys/socket.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../include/queueList.h"
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#define BUFFERSIZE 1024

int sigChild = 0;
int concurrencyLevel = 1;
int activeProcesses = 0;
int terminate = 0;
static int jobID = 0;
static int maxBuffer = 0;   // max number of processes that can get in the buffer
QueueList queue; //buffer
pthread_mutex_t concurrencyMTx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bufferMTx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t notFullBufferCond = PTHREAD_COND_INITIALIZER;    // conditional variable that buffer has space for new job
pthread_cond_t notEmptyBufferCond = PTHREAD_COND_INITIALIZER;   // conditional variable that buffer has a job inside

// function for making the path to log files
void makeLogFilePath(int childPid, char** path, int flag){
    char pidToStr[20];
    snprintf(pidToStr, sizeof(pidToStr), "%d", childPid);
    if (flag == 0)
        *path = realloc(*path, (strlen(pidToStr) + strlen("./build/") + strlen(".output") + 1));
    if(*path == NULL){
        perror("error malloc");
        exit(1);
    }

    if (flag == 0)
        strcpy(*path, "./build/");

    strcat(*path, pidToStr);
    strcat(*path, ".output");
}

// compare function. we store a complicated structure in our buffer so we must declare a function that compares them 
int CompareIds(void*a, void* b){
    int* first = (int*)a; 
    Job* second = (Job*)b;
    printf("## %d\n", *first);
    printf("#$ %d\n", second->jobId);
    if (*first > second->jobId)
        return 1;
    else if (*first < second->jobId)
        return -1;
    else
        return 0;

}

void sigchld_handler(int s)
{
    signal(SIGCHLD, sigchld_handler);
}

void handleCommand(int socket){
    char buffer[BUFFERSIZE];
    // read from client
    int check = read(socket, buffer ,BUFFERSIZE);
    if (check < 0){
        perror("read");
        exit(EXIT_FAILURE);
    }

    char cmd[check+1];
    cmd[check] = '\0';
    strncpy(cmd, buffer, check);

    if(strncmp(cmd , "setConcurrency", 14) == 0){
        pthread_mutex_lock(&concurrencyMTx);    // lock concurrency before accessing it
        concurrencyLevel = atoi(cmd + 15);  // setConcurrency (15 chars) 7 (7 is the new concurrency)
        pthread_mutex_unlock(&concurrencyMTx);

        char clientResponse[BUFFERSIZE];
        snprintf(clientResponse, BUFFERSIZE, "CONCURRENCY SET AT %d", concurrencyLevel);
        check = send(socket, clientResponse, BUFFERSIZE, 0);    // send reply to client
        if (check < 0){
            perror("send");
            exit(EXIT_FAILURE);
        }
        close(socket);
    }
    else if(strncmp(cmd, "poll", 4) == 0){
        pthread_mutex_lock(&bufferMTx);
        char response[BUFFERSIZE * maxBuffer]; // we need a big buffer for all the jobs in the buffer

        // iterate the buffer 
        for(ListNode n = getListFirst(queue); n != NULL; n = getListNext(queue, n)){
            char temp[BUFFERSIZE];
            Job* j = (Job*)getNodeValue(queue, n);
            snprintf(temp, "<%d, %s>\n", j->jobId, j->jobDescr);
            strcat(response, temp);
        }
        pthread_mutex_unlock(&bufferMTx);
        check = send(socket, response, BUFFERSIZE, 0);  //send reply
        if (check < 0){
            perror("send");
            exit(EXIT_FAILURE);
        }
        close(socket);
    }
    else if(strncmp(cmd, "exit", 4) == 0){
        terminate = 1;  //set flag for exit
        pthread_cond_broadcast(&notEmptyBufferCond); // wake all worker threads that are waiting for buffer to fill
        char clientResponse[BUFFERSIZE];
        snprintf(clientResponse, BUFFERSIZE, "SERVER TERMINATED BEFORE EXECUTION");
        check = send(socket, clientResponse, BUFFERSIZE, 0);
        if (check < 0){
            perror("send");
            exit(EXIT_FAILURE);
        }
        close(socket);
    }
    else if(strncmp(cmd, "stop", 4) == 0){
        pthread_mutex_lock(&bufferMTx);
        int id = atoi(cmd + 5);     // jopbid will be just after "stop " (5 characters)
        check = -1;
        for (ListNode node = getListFirst(queue); node != NULL; node = getListNext(queue, node)){
            Job *j = (Job*)getNodeValue(queue, node);
            if(j->jobId == id){
                check = removeValue(queue, &id, CompareIds);
                printf("check: %d", check);
            }
        }
        pthread_mutex_unlock(&bufferMTx);
        char clientResponse[BUFFERSIZE];
        if(!check)
            snprintf(clientResponse, BUFFERSIZE, "JOB %d REMOVED", id);
        else
            snprintf(clientResponse, BUFFERSIZE, "JOB %d NOT FOUND", id);
        check = send(socket, clientResponse, BUFFERSIZE, 0);
        if (check < 0){
            perror("send");
            exit(EXIT_FAILURE);
        }
        close(socket);
    }
    else if(strncmp(cmd, "issueJob", 8) == 0){
        pthread_mutex_lock(&bufferMTx);
        while(getListSize(queue) >= maxBuffer)          // wait for buffer to empty a slot
            pthread_cond_wait(&notFullBufferCond, &bufferMTx);
        
        jobID++;    // make the job and put it there
        Job* job = malloc(sizeof(job));
        job->jobId = jobID;
        strncpy(job->jobDescr, cmd + 9, strlen(cmd)-9); //read after "issueJob"
        job->clientSocket = socket;
        push(queue, job);
        pthread_cond_signal(&notEmptyBufferCond);   // signal that buffer has job inside
        pthread_mutex_unlock(&bufferMTx);
        char clientResponse[BUFFERSIZE];
        snprintf(clientResponse, BUFFERSIZE, "JOB <%d, %s> SUBMITTED", job->jobId, job->jobDescr);
        check = send(socket, clientResponse, BUFFERSIZE, 0);
        if (check < 0){
            perror("send");
            exit(EXIT_FAILURE);
        }
    }
}

void* controllerThread(void* arguments){
    int* socket = malloc(sizeof(int));
    socket = (int*)arguments;
    handleCommand(*socket);
    return NULL;
}

void* workerThread(void* arguments) {
    while (!terminate) {  // Check for termination
        pthread_mutex_lock(&bufferMTx);

        while (getListSize(queue) == 0) { // Check if there is anything in the buffer to execute
            pthread_cond_wait(&notEmptyBufferCond, &bufferMTx);
            if (terminate) { 
                pthread_mutex_unlock(&bufferMTx);
                return NULL;
            }
        }

        pthread_mutex_lock(&concurrencyMTx);
        if (activeProcesses < concurrencyLevel) { // Check how many jobs are currently running
            activeProcesses++;
            pthread_mutex_unlock(&concurrencyMTx);

            ListNode l = getListFirst(queue);
            if (l == NULL) {
                perror("buffer corrupted");
                exit(EXIT_FAILURE);
            }

            Job* j = (Job*)getNodeValue(queue, l);
            pop(queue); // Take the job out of the buffer
            pthread_cond_signal(&notFullBufferCond); // Signal that a spot has emptied
            pthread_mutex_unlock(&bufferMTx);

            pid_t pid = fork();
            if (pid == -1) {
                perror("Failed to create child process");
                exit(1);
            }

            if (pid == 0) { // Child process
                char path[256];
                snprintf(path, sizeof(path), "%d.output", getpid());
                FILE* f = fopen(path, "w+");    // Create a file for writing the output
                if (f == NULL) {
                    perror("Failed to open file");
                    exit(EXIT_FAILURE);
                }

                dup2(fileno(f), STDOUT_FILENO); // Redirect the output to the file
                fclose(f);

                // Prepare arguments for exec
                char* command = strdup(j->jobDescr);
                char* token = strtok(command, " ");
                char* argv[256];
                int argc = 0;

                while (token != NULL && argc < 255) {
                    argv[argc++] = token;
                    token = strtok(NULL, " ");
                }
                argv[argc] = NULL;

                execvp(argv[0], argv);
                perror("execvp failed");
                exit(EXIT_FAILURE);

            } else { // Parent process
                int status;
                waitpid(pid, &status, 0); // Wait for child

                char path[256];
                snprintf(path, sizeof(path), "%d.output", pid);
                FILE* fd = fopen(path, "r");    // Read file
                if (fd == NULL) {
                    perror("Failed to open file");
                    exit(EXIT_FAILURE);
                }

                fseek(fd, 0, SEEK_END); // See the length of the file
                long sz = ftell(fd);    // Return the current position = number of chars
                rewind(fd);             // Return position to start 

                char* response = malloc(sz + 1);
                if (response == NULL) {
                    perror("malloc failed");
                    fclose(fd);
                    exit(EXIT_FAILURE);
                }

                fread(response, 1, sz, fd);
                response[sz] = '\0';
                fclose(fd);
                remove(path); // Remove the file

                char clientResponse[sz + 100];
                snprintf(clientResponse, sizeof(clientResponse), "-------%d OUTPUT START-------\n%s-------OUTPUT END-------\n", j->jobId, response);
                free(response);

                int size = strlen(clientResponse);
                int check = send(j->clientSocket, &size, sizeof(int), 0);
                if (check < 0) {
                    perror("send size");
                    exit(EXIT_FAILURE);
                }
                check = send(j->clientSocket, clientResponse, size, 0);
                if (check < 0) {
                    perror("send clientResponse");
                    exit(EXIT_FAILURE);
                }

                close(j->clientSocket);

                pthread_mutex_lock(&concurrencyMTx);
                activeProcesses--; // Decrease the active processes
                pthread_mutex_unlock(&concurrencyMTx);
            }
        } else {
            pthread_mutex_unlock(&concurrencyMTx);
            pthread_mutex_unlock(&bufferMTx);
        }
    }
    return NULL;
}


int main(int argc, char* argv[]){
    int sock, client_count = 0;
    int new_sock;

    signal(SIGCHLD, sigchld_handler);

     // read arguments from input
    int portNum = atoi(argv[1]);
    int bufferSize = atoi(argv[2]);
    int threadPoolSize = atoi(argv[3]);
    maxBuffer = bufferSize;
    pthread_t workerThreads[threadPoolSize];

    int opt = 1;
    char* commandArguments;

    queue = createList();

    struct sockaddr_in addr;
    socklen_t length = sizeof(addr);

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }
    puts("Socket created");

    if (setsockopt(sock, SOL_SOCKET,
                   SO_REUSEADDR , &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Prepare the sockaddr_in structure
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(portNum);

    // Bind
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    puts("Bind done");

    // Listen
    listen(sock, 3);
    puts("Waiting for incoming connections...");

    for(int i = 0; i < threadPoolSize; i++){
        if (pthread_create(&workerThreads[i], NULL, workerThread, NULL) != 0) {
            perror("Failed to create thread");
        }
    }

    int lenAddr = sizeof(addr);
    while (1){
        if(terminate){
            exit(0);
            break;
        }   //check for exit

        new_sock = accept(sock, (struct sockaddr *)&addr, &lenAddr);    //accept new connections
        if(new_sock >= 0){
            pthread_t controller;
            if (pthread_create(&controller, NULL, controllerThread, (void*)&new_sock) != 0) {
                perror("Failed to create thread");
            }
        }
        else {
            perror("Accept failed");
            continue;
        }
        
    }
    close(new_sock);
    return 0;
}

