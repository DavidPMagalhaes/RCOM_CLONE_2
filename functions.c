#include "functions.h"
int parseCommandString(char** URLpath, char** serverName, char** serverIP, char** name, char** password){
  char** soup = memmov(URLpath, URLpath+6, strlen(URLpath));
  //parsing hostname
  char** bar = "/";
  char** hostname = memchr(soup, bar, strlen(soup));
  //parsing user
  char** token = strtok(soup, ":");
  if (strcmp(token, soup) == 0) {
    name="anonymous";
    password="";
  }else{
    char** aux = memchr(soup, ":", strlen(soup));
    name = memmov(soup, soup, strlen(soup)-strlen(aux));
    //parsing password
    char** aux2 = memchr(aux, "@", strlen(aux));
    char** password = memmov(aux, aux, strlen(aux)-strlen(aux2));
    password = memmov(password, password+1, strlen(password));
    //remove-se o 1o pq é :
  }
  //parsing serverIP and serverName

}
