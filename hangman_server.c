/* A simple server in the internet domain using TCP
   The port number is passed as an argument */

// http://www.linuxhowtos.org/data/6/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <pthread.h>

#define MAXLEN 256
#define MAXTHREADS 3

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

const char *filename = "hangman_words.txt";
char words[15][9]; // global to save lists of words
int numWord = 0;

static int numThreads = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

const int attemptsLimit = 6;

// reads the words from file to memory
void readWord() {
    numWord = 0;
    FILE *input = fopen(filename, "r");
    while (fscanf(input, "%s", words[numWord]) == 1) {
        //printf("%s\n", words[numWord]);
        ++numWord;
    }
}

/* 
    Server Header format: Msg Flag (==0) | Word Length | Num Incorrect | Data 
                          Msg Flag (!=0, indicating data length) | Data 
*/
void *func(void *arg) {
    pthread_mutex_lock(&mutex); // LOCK
    ++numThreads; // Multi-thread control
    int sockfd = *((int*)arg);
    char word[9], sendbuf[MAXLEN], recvbuf[MAXLEN];
    strcpy(word, words[(int)rand()%numWord]); // get a random word
    int length = strlen(word);
    printf("At descriptor: %d, the word is: %s\n", sockfd, word); 
    pthread_mutex_unlock(&mutex); // UNLOCK

    int n, i;
    bzero(recvbuf, MAXLEN);
    // receives the '0' package from client
    n = read(sockfd, recvbuf, sizeof(recvbuf));
    if (n < 0) error("ERROR reading from socket");
    // printf("Here is the initial message: %s\n", recvbuf);
    if (recvbuf[0] != '0') {
        printf("No game start signal, closing the connection.\n");
        pthread_mutex_lock(&mutex);
        --numThreads; // Multi-thread control
        pthread_mutex_unlock(&mutex);
        close(sockfd);
        return NULL;
    }

    // send back first package
    // flag, word length, num incorrect, data
    bzero(sendbuf, MAXLEN);
    sendbuf[0] = '0'; sendbuf[1] = length + '0'; sendbuf[2] = '0';
    for (i = 3; i < 3 + length; ++i) sendbuf[i] = '_';
    n = write(sockfd, sendbuf, sizeof(sendbuf));
    if (n < 0) error("ERROR writing to socket");

    int attempts = 0; 
    while (1) {
        // receive feedback
        bzero(recvbuf, MAXLEN);
        n = read(sockfd, recvbuf, sizeof(recvbuf));
        if (n < 0) error("ERROR reading from socket");

        // check the package format
        if (recvbuf[0] != '1') {
            printf("Wrongly formatted package from client.\n");
            close(sockfd);
            pthread_mutex_lock(&mutex);
            --numThreads; // Multi-thread control
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        // check if guess is correct
        char guess = recvbuf[1];
        int correct = 0, allCorrect = 1;
        for (i = 0; i < length; ++i) {
            if (word[i] == guess) {
                correct = 1;
                sendbuf[i+3] = guess;
            }
            if (sendbuf[i+3] == '_')
                allCorrect = 0;
        }
        // WINNING
        if (allCorrect == 1) { // if the client wins, send game control and close the connection
            bzero(sendbuf, MAXLEN);
            sendbuf[0] = '0' + 13 + length + 1 + 8 + 1 + 10 + 1;
            snprintf(sendbuf + 1, MAXLEN, "The word was %s\nYou Win!\nGame Over!\n", word);
            n = write(sockfd, sendbuf, sizeof(sendbuf));
            if (n < 0) error("ERROR writing to socket");
            close(sockfd);
            pthread_mutex_lock(&mutex);
            --numThreads; // Multi-thread control
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        if (correct == 0) { // incorrect guess
            sendbuf[3+length+attempts] = guess;
            sendbuf[2] = (++attempts) + '0';
        }
        // LOSING
        if (attempts >= attemptsLimit) {
            bzero(sendbuf, MAXLEN);
            sendbuf[0] = '0' + 13 + length + 1 + 9 + 1 + 10 + 1;
            snprintf(sendbuf + 1, MAXLEN, "The word was %s\nYou Lose!\nGame Over!\n", word);
            n = write(sockfd, sendbuf, sizeof(sendbuf));
            if (n < 0) error("ERROR writing to socket");
            close(sockfd);
            pthread_mutex_lock(&mutex);
            --numThreads; // Multi-thread control
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        n = write(sockfd, sendbuf, sizeof(sendbuf));
        if (n < 0) error("ERROR writing to socket");
    }
}

int main(int argc, char *argv[])
{
    int sockfd, connfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    // read the words to memory
    readWord();

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr,
            sizeof(serv_addr)) < 0) 
            error("ERROR on binding");
    
    listen(sockfd, 10);

    clilen = sizeof(cli_addr);
    while (1) {
        connfd = accept(sockfd, 
                (struct sockaddr *) &cli_addr, 
                &clilen);
        if (connfd < 0) error("ERROR on accept");
        if (numThreads >= MAXTHREADS) { // Close the connection if there is more than 3 clients
            // flush the fd?
            write(connfd, "server-overloaded\n", 18);
            close(connfd);
            continue;
        }
        pthread_t tid;
        pthread_mutex_lock(&mutex);
        if (pthread_create(&tid, NULL, func, (void*)&connfd) != 0) printf("Failed to create thread\n");
        if (pthread_detach(tid) != 0) printf("Failed to detach thread\n");
        printf("Thread created at file descriptor %d\n", connfd);
        pthread_mutex_unlock(&mutex);
    }

    close(sockfd);
    return 0; 
}
