#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/* Reads a file and passes its content line by line to the kernel */
int main (int argc, char **argv) {
  
  int procFileFd;
  char * buffer;
  int a,b;
  a=21;
  b=213;
  buffer=(char *)(malloc(2*sizeof(int)+sizeof(char)));
  buffer[0]='A';
  memcpy(buffer+sizeof(char),&a,sizeof(int));
  memcpy(buffer+sizeof(char)+sizeof(int),&b,sizeof(int));
  printf("%d\n",*((int *)(buffer+sizeof(char))));
	if (argc != 2) {
	fprintf (stderr, "usage: %s <proc filename> \n", argv[0]);
	exit (1);
	}

	procFileFd = open (argv[1], O_WRONLY || O_APPEND);
	if (procFileFd == -1) {
	fprintf (stderr, "Opening failed!\n");
	exit (1);
	}
	write (procFileFd, buffer,sizeof(char)+2*sizeof(int));

  
  close (procFileFd); /* make sure data is properly written */

  
  exit (0);

}
  

