all: program getip clienttcp

program: main.c functions.c functions.h
	gcc -g -o download main.c functions.c

getip: getip.c
	gcc -g -o getip.exe getip.c

clienttcp: clientTCP.c
	gcc -g -o clientTCP.exe clientTCP.c