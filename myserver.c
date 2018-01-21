#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

void error(const char * msg) {
    perror(msg);
    exit(1);
}

void signal_handler(int signum) {
    printf("Termination signal detected, server terminating!\n");
    exit(signum);
}

void tok_command(char **args,char *buffer){
    int i=0;
    char *token;
    token = strtok(buffer," ");
    while(token != NULL){
        args[i]=token;
        token = strtok(NULL," ");
        i++;
    }
    args[i]=NULL;
}

int main(int argc, char * argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    char str[INET_ADDRSTRLEN];
    char *args[10];

    signal(SIGINT,signal_handler);

    if (argc < 2) {
        fprintf(stderr, "No port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char * ) & serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    printf("Server listening on port %d\n", portno);
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    int pid;
    while(1) {
        newsockfd = accept(sockfd, (struct sockaddr * ) & cli_addr, & clilen);
        if (newsockfd < 0) {
            error("ERROR on accept");
        }
        if (inet_ntop(AF_INET, & cli_addr.sin_addr, str, INET_ADDRSTRLEN) == NULL) {
            fprintf(stderr, "Could not convert byte to address\n");
            exit(1);
        }
        fprintf(stdout, "New client connected!\n");
        fprintf(stdout, "The client address is :%s\n", str);
        pid = fork();
        if(pid<0) {
            error("ERROR in new process creation");
        }
        if(pid==0) {
            while (1) {
                bzero(buffer, 256);
                n = read(newsockfd, buffer, 255);
                if (n < 0) error("ERROR reading from socket");
                char * ret = strchr(buffer, '\n');
                if (ret != NULL) {
                    *ret = 0;
                }
                if(strcmp(buffer,"quit")==0){
                    fprintf(stdout,"Client disconnected!\n");
                    break;
                }else if(buffer[0]!='\0'){
                    printf("Executing command: %s\n", buffer);
                    tok_command(args,buffer);                    
                }
                int pid, status;
                if ((pid = fork()) == -1) {
                    perror("fork");
                    exit(1);
                }
                if (pid != 0) {
                    if (wait( & status) == -1) {
                        perror("wait");
                    }
                } else {
                    dup2(newsockfd, STDOUT_FILENO);
                    dup2(newsockfd,STDERR_FILENO);
                    close(newsockfd);
                    execvp(args[0], args);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    exit(EXIT_SUCCESS);
}