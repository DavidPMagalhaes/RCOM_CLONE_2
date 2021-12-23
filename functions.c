#include <stdio.h>
#include <string.h>
#include "functions.h"
int parseCommandString(char* commandArg, char* serverName, char* serverIP, char* name, char* password, char* file_name){
  char* soup;
  memmove(soup, commandArg+6, strlen(commandArg));
  //parsing hostname
  serverName = memchr(soup, '/', strlen(soup));
  //parsing user
  char* token = strtok(soup, ":");
  if (strcmp(token, soup) == 0) {
    name="anonymous";
    password="";
  }else{
    char* aux = memchr(soup, ':', strlen(soup));
    memmove(name, soup, strlen(soup)-strlen(aux));
    //parsing password
    char* aux2 = strchr(aux, '@');
    memmove(password, aux, strlen(aux)-strlen(aux2));
    memmove(password, password+1, strlen(password));
    //remove-se o 1o pq é :
  }
  //parsing serverIP and serverName
  char* last = strrchr(token, '/');
  if (last != NULL) {
    memmove(serverIP, last, strlen(last));
    memmove(file_name, last+1, strlen(last));
  }else {
    serverIP="";
    memmove(file_name, token, strlen(token));
  }
  printf("Parsed command string.\n");
  return 0;
}
