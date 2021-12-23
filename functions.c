#include "functions.h"

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void printHelp()
{
    printf("Usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
}

int parseCommandString(char *URLpath, char **serverName, char **serverIP, char **name, char **password, char **file)
{
}

void downloadFile(char *serverIP, char *name, char *password, char *file)
{
    int res;
    int controlSocket = establishControlConnection(serverIP);
    loginControlConnection(controlSocket, name, password);
    int dataSocket = establishDataConnection(serverIP, controlSocket);
    size_t fileLen = askForFile(controlSocket, file);
    char buf[fileLen];
    waitForFileTransfer(controlSocket);
    readFile(dataSocket, buf);
    saveFile(buf);

    if (close(controlSocket) < 0)
    {
        perror("close()");
        exit(-1);
    }
    if (close(dataSocket) < 0)
    {
        perror("close()");
        exit(-1);
    }
}

int establishConnection(char *serverIP, int port)
{
    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(serverIP); /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);                /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        exit(-1);
    }
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect()");
        exit(-1);
    }

    return sockfd;
}

int establishControlConnection(char *serverIP)
{
    return establishConnection(serverIP, SERVER_PORT);
}

void loginControlConnection(int controlSocket, char *name, char *password)
{
    int res;
    char command[BUFFER_SIZE];
    char *usernameAnswer = "331 Please specify the password.";
    char *passwordAnswer = "230 Login successful.";
    snprintf(command, BUFFER_SIZE, "name %s", name);
    socketWrite(controlSocket, command);
    res = socketReadAndVerify(controlSocket, usernameAnswer, strlen(usernameAnswer) - 1);
    if (res)
    {
        fprintf(stderr, "Error providing the user");
        exit(-1);
    }
    snprintf(command, BUFFER_SIZE, "pass %s", password);
    socketWrite(controlSocket, command);
    res = socketReadAndVerify(controlSocket, passwordAnswer, strlen(passwordAnswer) - 1);
    if (res)
    {
        fprintf(stderr, "Error providing the password");
        exit(-1);
    }
}

int establishDataConnection(char *serverIP, int controlSocket)
{
    char answer[BUFFER_SIZE];
    socketWrite(controlSocket, "PASV");
    socketRead(controlSocket, answer, BUFFER_SIZE);
    int port = getPASVPort(answer);
    int dataSocket = establishConnection(serverIP, port);
    return dataSocket;
}

int getPASVPort(char *answer)
{
    char s[4][20];
    int nb[6];
    if (strncmp(answer, "227", 3))
    {
        fprintf(stderr, "PASV didn't return 227");
        exit(-1);
    }
    sscanf(answer, "%s %s %s %s (%d,%d,%d,%d,%d,%d)",
           s[0], s[1], s[2], s[3],
           &nb[0], &nb[1], &nb[2], &nb[3], &nb[4], &nb[5]);

    return nb[4] * 256 + nb[5];
}

off_t askForFile(int socket, char *file)
{
    int cmdSize = strlen(file) + 5 + 1;
    char command[cmdSize];
    int strSize = strlen(file) + BUFFER_SIZE;
    char answer[strSize];
    char *expectedAnswerPrefix = "150 Opening BINARY mode data connection for ";
    int cmpSize = strlen(expectedAnswerPrefix) + strlen(file) + 1;
    char expectedAnswer[cmpSize];

    snprintf(command, cmdSize, "RETR %s", file);
    socketWrite(socket, command);
    socketRead(socket, answer, strSize);

    snprintf(expectedAnswer, cmpSize, "%s%s", expectedAnswerPrefix, file);
    if (strncmp(answer, expectedAnswer, cmpSize - 1))
    {
        fprintf(stderr, "Wrong RETR answer: %s", answer);
        exit(-1);
    }

    off_t fileSize = getFileSize(answer, strlen(expectedAnswer) + 1);
    return fileSize;
}

off_t getFileSize(char *answer, int size)
{
    off_t fileSize;
    sscanf(answer + size, "(%jd bytes).", &fileSize);
    return fileSize;
}

void waitForFileTransfer(int socket)
{
    char *transferComplete = "226 Tranfer complete.";
    int res = socketReadAndVerify(socket, transferComplete, strlen(transferComplete));
    if (res)
    {
        fprintf(stderr, "Couldn't receive file");
        exit(-1);
    }
}

void readFile(int socket, char *buf)
{
}

void saveFile(char *buf)
{
}

void socketWrite(int socket, char *toWrite)
{
    size_t bytes = write(socket, toWrite, strlen(toWrite));
    if (bytes > 0)
    {
        // printf("Bytes escritos %ld\n", bytes);
    }
    else
    {
        perror("write()");
        exit(-1);
    }
}

void socketRead(int socket, char *toRead, int toReadSize)
{
    ssize_t bytes = read(socket, toRead, toReadSize);

    if (bytes > 0)
    {
    }
    else
    {
        perror("read()");
        exit(-1);
    }
}

int socketReadAndVerify(int socket, char *toVerify, int bytes)
{
    char toRead[BUFFER_SIZE];
    socketRead(socket, toRead, BUFFER_SIZE);
    if (strncmp(toRead, toVerify, bytes))
    {
        return -1;
    }
    return 0;
}