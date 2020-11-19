#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <ctype.h>
#include <time.h> // for better random

#define MAXLEN 256

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

// read user input, make sure it's valid, otherwise prompt user again
void getInput(char *buf) {
    int isValid;
    while (1) {
        isValid = 1;
        bzero(buf, MAXLEN);
        printf("Letter to guess: ");
        fgets(buf, MAXLEN, stdin);
        if (buf[1] != '\0' && buf[1] != '\n') isValid = 0; // check length
        if (isalpha(buf[0]) == 0) isValid = 0; // check if is alphabet
        if (isValid == 1) break;
        else printf("Error! Please guess one letter.\n"); 
    }
    buf[1] = tolower(buf[0]); // set package format
    buf[0] = '1';
}

int main(int argc, char *argv[])
{
    srand(time(0));

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    
    /* 
    Server Header format: Msg Flag (==0) | Word Length | Num Incorrect | Data 
                          Msg Flag (!=0, indicating data length) | Data 
    */
    char inputbuf[MAXLEN], recvbuf[MAXLEN];

    // prompt the user
    printf("Ready to start game? (y/n): ");
    bzero(inputbuf, MAXLEN);
    fgets(inputbuf, sizeof(inputbuf), stdin);
    printf("\n");

    // Check y/n and terminate if 'n'
    if (inputbuf[0] != 'y') {
        printf("User declined the game\n");
        close(sockfd);
        return 0;
    } 

    // send "0" package to start the game
    n = write(sockfd, "0", 1);
    if (n < 0) error("ERROR writing to socket");
    
    // receive first package
    bzero(recvbuf, MAXLEN);
    n = read(sockfd, recvbuf, sizeof(recvbuf));
    if (n < 0) error("ERROR reading from socket");
    //printf("Package received: %s\n", recvbuf);

    // if the server is overloaded
    if (recvbuf[0] == 's' && recvbuf[1] == 'e') {
        printf("%s", recvbuf);
        close(sockfd);
        return 0;
    }

    int flag = recvbuf[0] - '0', length = recvbuf[1] - '0'; //, incorrect = recvbuf[2] - '0';
    while (1) {
        if (flag != 0) { // game control package
            printf("%s", recvbuf + 1);
            break;
        }

        printf("%.*s\n", length, recvbuf + 3);
        printf("Incorrect Guesses: %s\n\n", recvbuf + 3 + length);

        // prompt for input, package it, then send it
        getInput(inputbuf);
        n = write(sockfd, inputbuf, 2);
        if (n < 0) error("ERROR writing to socket");

        // receive from server
        bzero(recvbuf, MAXLEN);
        n = read(sockfd, recvbuf, sizeof(recvbuf));
        if (n < 0) error("ERROR reading from socket");
        flag = recvbuf[0] - '0'; length = recvbuf[1] - '0'; // incorrect = recvbuf[2] - '0';
    }

    close(sockfd);
    return 0;
}