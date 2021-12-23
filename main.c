#include "functions.h"

#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    char *name, *password, *serverName, *serverIP, *file;
    if (argc != 2)
    {
        printHelp();
        exit(-1);
    }

    parseCommandString(argv[1], serverName, serverIP, name, password, file);

    if (strcmp(serverIP, "") == 0)
    {
        getIPfromDNS(&serverName, &serverIP);
    }
    free(serverName);

    downloadFile(serverIP, name, password, file);

    free(name);
    free(password);
    free(serverIP);
    free(file);
    return 0;
}
