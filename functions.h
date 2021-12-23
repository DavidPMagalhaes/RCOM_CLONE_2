#define SERVER_PORT 21
#define BUFFER_SIZE 1024

#include <unistd.h>

void printHelp();

int parseCommandString(char* commandArg, char* serverName, char* serverIP, char* name, char* password, char* file_name);

void getIPfromDNS(char **servername, char **serverIP);

void downloadFile(char *serverIP, char *name, char *password, char *file);

int establishConnection(char *serverIP, int port);

int establishControlConnection(char *serverIP);

void loginControlConnection(int controlSocket, char *name, char *password);

int establishDataConnection(char *serverIP, int port);

int getPASVPort(char *answer);

off_t askForFile(int socket, char *file);

off_t getFileSize(char *answer, int size);

void waitForFileTransfer(int socket);

void readFile(int socket, char *buf);

void saveFile(char *buf);

void socketWrite(int socket, char *toWrite);

void socketRead(int socket, char *toRead, int toReadSize);

int socketReadAndVerify(int socket, char *toVerify, int bytes);
