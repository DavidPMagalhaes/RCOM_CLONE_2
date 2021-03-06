#include "functions.h"

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <ctype.h>

int state = 0;
void printHelp()
{
  printf("Usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
}

int parseCommandString(char *commandArg, char **serverName, char *serverIP, char **name, char **password, char **file_name)
{
  char *soup = (char *)malloc((strlen(commandArg) + 1) * sizeof(char));
  (*serverName) = (char *)malloc((strlen(commandArg) + 1) * sizeof(char));
  (*name) = (char *)malloc((strlen(commandArg) + 1) * sizeof(char));
  (*password) = (char *)malloc((strlen(commandArg) + 1) * sizeof(char));
  (*file_name) = (char *)malloc((strlen(commandArg) + 1) * sizeof(char));
  memmove(soup, commandArg + 6, strlen(commandArg));
  //parsing hostname
  char *temp = memchr(soup, '/', strlen(soup));
  //parsing user, password and serverName
  if (strchr(soup, ':'))
  {
    memmove(*serverName, soup, strlen(soup) - strlen(temp));
    *serverName = memchr(*serverName, '@', strlen(*serverName));
    memmove(*serverName, *serverName + 1, strlen(*serverName));
    printf("serverName: %s\n", *serverName);
    char *aux = memchr(soup, ':', strlen(soup));
    memmove(*name, soup, strlen(soup) - strlen(aux));
    //parsing password
    char *aux2 = strchr(aux, '@');
    memmove(*password, aux, strlen(aux) - strlen(aux2));
    memmove(*password, *password + 1, strlen(*password));
  }
  else
  {
    char *token = strstr(soup, ":");
    memmove(*serverName, soup, strlen(soup) - strlen(temp));
    strcpy(*name, "anonymous");
    strcpy(*password, "empty");
  }
  //parsing serverIP and file_name
  char *last = strrchr(temp, '/');
  last++;
  if (last != NULL)
  {
    memmove(serverIP, last, strlen(last));
    memmove(*file_name, temp + 1, strlen(temp));
  }
  else
  {
    strcpy(serverIP, "");
    memmove(*file_name, temp, strlen(temp));
  }
  return 0;
}

void getIPfromDNS(char *servername, char *serverIP)
{
  struct hostent *h;
  if ((h = gethostbyname(servername)) == NULL)
  {
    herror("gethostbyname()");
    exit(-1);
  }
  strcpy(serverIP, inet_ntoa(*((struct in_addr *)h->h_addr)));
}

void downloadFile(char *serverIP, char *name, char *password, char *file)
{
  int res;
  int controlSocket = establishControlConnection(serverIP);
  state++;
  loginControlConnection(controlSocket, name, password);
  state++;
  int dataSocket = establishDataConnection(serverIP, controlSocket);
  state++;
  off_t fileLen = askForFile(controlSocket, file);
  state++;
  char buf[fileLen];
  waitForFileTransfer(controlSocket);
  state++;

  readFile(dataSocket, buf, fileLen);
  state++;
  saveFile(file, buf, fileLen);

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
  char *connectionAnswer = "220 Welcome to netlab-FTP server";
  int socket = establishConnection(serverIP, SERVER_PORT);
  int res = socketReadAndVerify(socket, connectionAnswer, strlen(connectionAnswer) - 1);
  if (res)
  {
    fprintf(stderr, "Error establishing connection");
    exit(-1);
  }
  return socket;
}

void loginControlConnection(int controlSocket, char *name, char *password)
{
  int res;
  char command[BUFFER_SIZE];
  char *usernameAnswer = "331 Please specify the password.";
  char *passwordAnswer = "230 Login successful.";
  snprintf(command, BUFFER_SIZE, "user %s\n", name);
  socketWrite(controlSocket, command);
  int d = strlen(usernameAnswer) - 1;
  res = socketReadAndVerify(controlSocket, usernameAnswer, strlen(usernameAnswer) - 1);
  if (res)
  {
    fprintf(stderr, "Error providing the user");
    exit(-1);
  }
  snprintf(command, BUFFER_SIZE, "pass %s\n", password);
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
  socketWrite(controlSocket, "PASV\n");
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
  int cmdSize = strlen(file) + 5 + 1 + 1;
  char command[cmdSize];
  int strSize = strlen(file) + BUFFER_SIZE;
  char answer[strSize];
  char *expectedAnswerPrefix = "150 Opening BINARY mode data connection for ";
  int cmpSize = strlen(expectedAnswerPrefix) + strlen(file) + 1;
  char expectedAnswer[cmpSize];

  snprintf(command, cmdSize, "RETR %s\n", file);
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
  char *transferComplete = "226 Transfer complete.";
  int res = socketReadAndVerify(socket, transferComplete, strlen(transferComplete));
  if (res)
  {
    fprintf(stderr, "Couldn't receive file");
    exit(-1);
  }
}

void readFile(int socket, char *buf, off_t len)
{
  socketRead(socket, buf, len);
}

void saveFile(char *filename, char *buf, off_t len)
{
  char *fileNameNoPath = strrchr(filename, '/');
  if (fileNameNoPath == NULL)
  {
    fileNameNoPath = filename;
  }
  else
  {
    fileNameNoPath += 1;
  }
  int fd = open(fileNameNoPath, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  if (fd == -1)
  {
    perror("open()");
    exit(-1);
  }
  int res = write(fd, buf, len);
  if (res == -1)
  {
    perror("write()");
    exit(-1);
  }
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
  char *specialCases;
  char *lastLine;
  char *readTo = toRead;
  ssize_t leftInLine = 0;
  while (1)
  {
    ssize_t bytes = read(socket, readTo, toReadSize);

    if (bytes > 0)
    {
    }
    else
    {
      perror("read()");
      exit(-1);
    }
    if(state == 0 || state == 4 || state == 5){
      specialCases = strrchr(toRead, '\n');
      *(specialCases+1) = '\0';
    }
    printf("%s", readTo);

    // Mainly after connecting to the server
    // Not all of the message might have been sent
    // We only know the message hasn't ended if the last line does not have a dash '-' after an http status code

    // Get the last line and number of bytes in it
    readTo[bytes - 1] = '\0';
    lastLine = strrchr(readTo, '\n');
    readTo[bytes - 1] = '\n';
    if (lastLine == NULL)
    {
      // Read a total of one line
      leftInLine = strlen(readTo);
      lastLine = readTo;
    }
    else
    {
      lastLine++;
      leftInLine = bytes - (lastLine - readTo);
    }

    // Verify whether or not the line fits the conditions for being a message termination
    if (leftInLine >= 2 && isdigit(lastLine[0]) && isdigit(lastLine[1]) && isdigit(lastLine[2]))
    {
      if (leftInLine >= 3)
      {
        // There are more bytes after the code
        if (lastLine[3] == ' ')
        {
          return;
          // readTo = readTo + bytes;
          // continue;
        }
        else
        {
          return;
        }
      }
      else
      {
        return;
      }
    }
    else
    {
      return;
    }
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
