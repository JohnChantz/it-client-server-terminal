#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void error(const char * msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char * argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    char str[INET_ADDRSTRLEN];
    char * args[2];

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
    newsockfd = accept(sockfd, (struct sockaddr * ) & cli_addr, & clilen);
    if (newsockfd < 0)
        error("ERROR on accept");
    if (inet_ntop(AF_INET, & cli_addr.sin_addr, str, INET_ADDRSTRLEN) == NULL) {
        fprintf(stderr, "Could not convert byte to address\n");
        exit(1);
    }
    fprintf(stdout, "The client address is :%s\n", str);
    while (1) {
        bzero(buffer, 256);
        n = read(newsockfd, buffer, 255);
        if (n < 0) error("ERROR reading from socket");
        // printf("Here is the message: %s\n", buffer);
        args[0] = buffer;
        args[1] = NULL;
        char * ret = strchr(buffer, '\n');
        if (ret != NULL) { * ret = 0;
        }
        int pid, status;
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(1);
        }
        if (pid != 0) {
            // printf("Server/parent with pid=%d waiting...\n", getpid());
            if (wait( & status) == -1) {
                perror("wait");
            }
        }else{
            dup2(newsockfd, STDOUT_FILENO);
	        dup2(newsockfd,STDERR_FILENO);
	        close(newsockfd);
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        // n = write(newsockfd, "message received", 17);
        // if (n < 0) error("ERROR writing to socket");
    }
    close(newsockfd);
    close(sockfd);
    return 0;
}