#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>

int sockfd;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void signal_handler(int signum) {
    printf("\nTermination signal detected, server terminating!\n");
    close(sockfd);
    exit(signum);
}

int tok_command(char **args, char *buffer) {
    int flag = 0;
    int i = 0;
    char *token;
    token = strtok(buffer, " ");
    while (token != NULL) {
        args[i] = token;
        if (strcmp(token, "|") == 0) {
            flag = 1;
        }else if(strcmp(token, ">") == 0){
            flag = 2;
        }
        token = strtok(NULL, " ");
        i++;
    }
    args[i] = NULL;
    return flag;
}

void split_commands(char **args, char **cmd1, char **cmd2) {
    int i = 0;
    int cmd1_position = 0, cmd2_position = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "|") == 0) {
            cmd1_position = i - 1;
            cmd2_position = i + 1;
            break;
        }
        i++;
    }
    for (i = 0; i <= cmd1_position; i++) {
        cmd1[i] = args[i];
    }
    cmd1[i] = NULL;
    i = 0;
    while (args[cmd2_position + i] != NULL) {
        cmd2[i] = args[cmd2_position + i];
        i++;
    }
    cmd2[i] = NULL;
}

int find_redirection(char **args,char**cmd){
    int i=0;
    int redirection_position=0;
    int redirection;
    while(args[i] != NULL){
        if(strcmp(args[i],">")==0){
            redirection_position = i;
            break;
        }
        i++;
    }
    for(i=0;i<redirection_position;i++){
        cmd[i]=args[i];
    }
    cmd[i]=NULL;
    redirection = open(args[redirection_position+1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    return redirection;
}

void runpipe(int pfd[], char **cmd1, char **cmd2, int newsockfd) {
    int pid;
    switch (pid = fork()) {
        case -1:
            perror("fork");
        case 0: /* child */
            dup2(pfd[0], 0);
            close(pfd[1]);/* the child does not need this end of the pipe */
            dup2(newsockfd, STDOUT_FILENO);
            dup2(newsockfd, STDERR_FILENO);
            close(newsockfd);
            execvp(cmd2[0], cmd2);
            perror(cmd2[0]);
            exit(EXIT_FAILURE);
        default: /* parent */
            dup2(pfd[1], 1);
            close(pfd[0]);/* the parent does not need this end of the pipe */
            execvp(cmd1[0], cmd1);
            perror(cmd1[0]);
            exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    int newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    char str[INET_ADDRSTRLEN];
    char *args[10];

    signal(SIGINT, signal_handler);

    if (argc < 2) {
        fprintf(stderr, "No port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    printf("Server listening on port %d\n", portno);
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            error("ERROR on accept");
        }
        if (inet_ntop(AF_INET, &cli_addr.sin_addr, str, INET_ADDRSTRLEN) == NULL) {
            fprintf(stderr, "Could not convert byte to address\n");
            exit(1);
        }
        fprintf(stdout, "New client connected!\n");
        fprintf(stdout, "The client address is :%s\n", str);
        int pid = fork();
        if (pid < 0) {
            error("ERROR in new process creation: FAILED to create new client process!");
        }
        if (pid == 0) {
            while (1) {
                int flag, pid, status;
                bzero(buffer, 256);
                n = read(newsockfd, buffer, 255);
                if (n < 0) error("ERROR reading from socket");
                char *ret = strchr(buffer, '\n');
                if (ret != NULL) {
                    *ret = 0;
                }
                if (strcmp(buffer, "quit") == 0) {
                    fprintf(stdout, "Client disconnected!\n");
                    break;
                } else if (buffer[0] != '\0') {
                    printf("Executing command: %s\n", buffer);
                    flag = tok_command(args, buffer);
                }
                if (flag == 1) {
                    char *cmd1[10];
                    char *cmd2[10];
                    split_commands(args, cmd1, cmd2);
                    int fd[2];
                    pipe(fd);
                    switch (pid = fork()) {
                        case -1:
                            perror("fork");
                            exit(1);
                        case 0: /* child */
                            runpipe(fd, cmd1, cmd2, newsockfd);
                        default: /* parent */
                            while (wait(&status) != pid);  /*Wait for child*/
                            printf("Child terminated with exit code %d\n", status >> 8);
                    }
                }else if(flag == 2){
                    char *cmd[10];
                    int redirection = find_redirection(args,cmd);
                    if(redirection < 0){
                        strcpy(buffer,"Cant open file for redirection!");
                        n = write(sockfd, buffer, strlen(buffer));
                        if (n < 0)
                            error("ERROR writing to socket");
                    }
                    if ((pid = fork()) < 0) {
                        perror("fork");
                        exit(1);
                    }
                    if (pid != 0) {
                        while (wait(&status) != pid);  /*Wait for child*/
                        printf("Child terminated with exit code %d\n", status >> 8);
                        strcpy(buffer,"Output of the execution was send to file!");
                        n = write(newsockfd, buffer, strlen(buffer));
                        if (n < 0)
                            error("ERROR writing to socket");
                    } else {
                        dup2(redirection,STDOUT_FILENO);
                        close(redirection);
                        execvp(cmd[0],cmd);
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    if ((pid = fork()) < 0) {
                        perror("fork");
                        exit(1);
                    }
                    if (pid != 0) {
                        while (wait(&status) != pid);  /*Wait for child*/
                        printf("Child terminated with exit code %d\n", status >> 8);
                    } else {
                        dup2(newsockfd, STDOUT_FILENO);
                        dup2(newsockfd, STDERR_FILENO);
                        close(newsockfd);
                        execvp(args[0], args);
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    exit(EXIT_SUCCESS);
}