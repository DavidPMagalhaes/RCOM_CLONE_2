all: program getip clienttcp

program: main.c
	gcc -g -o main.exe main.c

getip: getip.c
	gcc -g -o getip.exe getip.c

clienttcp: clientTCP.c
	gcc -g -o clientTCP.exe clienttcp.c