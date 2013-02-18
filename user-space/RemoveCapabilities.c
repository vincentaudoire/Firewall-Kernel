#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define proc_file_adress "/proc/kernelSwap"


int main (int argc, char **argv) {
  
  int procFileFd;
  char * buffer;
  int a,b;
  buffer=(char *)(malloc(2*sizeof(int)+sizeof(char)));
  buffer[0]='D';  
  if (argc != 3) {
	fprintf (stderr, "usage: %s capability port \n", argv[0]);
        exit (1);
   }
   a=atoi(argv[1]);
   b=atoi(argv[2]);
  memcpy(buffer+sizeof(char),&a,sizeof(int));
  memcpy(buffer+sizeof(char)+sizeof(int),&b,sizeof(int));
  printf("%d %d  \n",a,b);
	procFileFd = open (proc_file_adress, O_WRONLY || O_APPEND);
	if (procFileFd == -1) {
	fprintf (stderr, "Opening failed!\n");
	exit (1);
	}
	write (procFileFd, buffer,sizeof(char)+2*sizeof(int));

  close (procFileFd);
  
  exit (0);

}
  

