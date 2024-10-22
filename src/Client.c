#include "../include/checkArguments.h"
#include<sys/wait.h>
#include<sys/socket.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define BUFFSIZE 1024

int main(int argc, char* argv[]){

    int sock;

    // read arguments from input
    char* serverName = malloc(strlen(argv[1]) + 1);
    if (serverName == NULL){
        printf("Memory allocation failed");
        return -1;
    }
    strcpy(serverName, argv[1]);
    
    int portNum = atoi(argv[2]);
    char* commandArguments = NULL;
    commandArguments = malloc(sizeof(char));
    // validate arguments
    int check = checkArguments(0, argc, argv, &commandArguments);
    if (check != 0)
        return check;

    // connect to server
    struct sockaddr_in addr;
    struct hostent *host;

    host  = gethostbyname(serverName);
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\n Socket creation error \n");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(portNum);
    addr.sin_addr = *((struct in_addr *)host->h_addr_list[0]);

    int true = 1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)); 

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
          perror("Failed to connect to socket\n");
          return -1;
    }

    int stop = 0, flag = 0, checkSending;
    char buffer[BUFFSIZE];
    while (!stop)
    {
        // pass the commands to the server and wait for responses
        if (strncmp(commandArguments, "setConcurrency", 14) == 0) {
            send(sock, commandArguments, BUFFSIZE, 0);
            int check = read(sock, buffer, BUFFSIZE);
            printf("%s\n", buffer);
            break;
        } 
        else if (strncmp(commandArguments, "poll", 4) == 0) {
            send(sock, commandArguments, BUFFSIZE, 0);
            int check = read(sock, buffer, BUFFSIZE);
            printf("%s\n", buffer);
            break;
        } 
        else if (strncmp(commandArguments, "exit", 4) == 0) {
            strncpy(buffer, commandArguments, strlen(commandArguments));
            send(sock, buffer, BUFFSIZE, 0);
            int check = read(sock, buffer, BUFFSIZE);
            printf("%s\n", buffer);
            break;
        } 
        else if (strncmp(commandArguments, "stop", 4) == 0) {
            strncpy(buffer, commandArguments, strlen(commandArguments));
            send(sock, buffer, BUFFSIZE, 0);
            int check = read(sock, buffer, BUFFSIZE);
            printf("%s\n", buffer);
            break;
        } 
        else if (strncmp(commandArguments, "issueJob", 8) == 0) {   // here we are waiting for more than 1 response
            if (flag == 1){
                int size;
                int check = read(sock, &size, sizeof(int));
                char response[size+1];
                check = read(sock, response, size);
                response[size] = '\0';
                printf("%s\n", response);
                break;
            } 
            else{
                send(sock, commandArguments, strlen(commandArguments), 0);
                int check = read(sock, buffer, BUFFSIZE);
                printf("%s\n", buffer);
                flag++;
            }    
            continue;
        } 
        else {
            printf("Invalid command. Try again.\n");
        }
    }

    close(sock);
    return 0;
}