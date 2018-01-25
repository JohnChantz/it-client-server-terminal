#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>

int sockfd, n;

void error(const char *msg) {
    perror(msg);
    exit(0);
}

void signal_handler(int signum) {   //when Ctrl+C signal is detected in client, inform server that a client disconnected
    printf("\nTermination signal detected, client terminating!\n");
    char *buffer = "quit";
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");
    close(sockfd);
    exit(signum);
}

int main(int argc, char *argv[]) {
    int portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[1024];

    signal(SIGINT, signal_handler); //set signal_handler() for SIGINT

    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }
    while (1) {
        fprintf(stdout, "%s:>", argv[1]);   //print client's prompt
        bzero(buffer, 1024);
        fgets(buffer, 1023, stdin);
        int ret = strcmp(buffer, "quit\n"); //check if client wants to disconnect, typed "quit"
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
            error("ERROR writing to socket");
        if (ret == 0) {     //terminate client, if input equals "quit"
            printf("Client disconecting!\n");
            break;
        }
        bzero(buffer, 1024);
        n = read(sockfd, buffer, 1023);
        if (n < 0)
            error("ERROR reading from socket");
        printf("%s\n", buffer);
    }
    close(sockfd);
    exit(EXIT_SUCCESS);
}
