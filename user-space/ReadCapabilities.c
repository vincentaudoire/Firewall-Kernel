#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define  BUFFERSIZE 1024
#define  proc_file_path "/proc/kernelSwap"

struct capability{
int port;
int capability;
}; 

int main (int argc, char **argv) {
  
	int procFileFd;
	struct capability buffer[BUFFERSIZE];
	int count = 0;
	int currentlyRead = 0;
	int i;


	procFileFd = open (proc_file_path, O_RDONLY);
  
	if (procFileFd == -1) {
		fprintf (stderr, "Opening failed!\n");
		exit (1);
	}

	while (count < BUFFERSIZE * sizeof (struct capability)) {
		currentlyRead = read (procFileFd, buffer + count, BUFFERSIZE * sizeof (struct capability) - count);
		if (currentlyRead < 0) {
			fprintf (stderr, "Reading failed! \n");
			exit (1);
		}
		count = count + currentlyRead;
		if (currentlyRead == 0) { 
			/* EOF encountered */
			break;
		}
	}
	printf ("Read buffer of size %d\n", count);
	for (i = 0; i * sizeof (struct capability) < count; i++) {
		printf ("Capability %d :  ( %d , %d )\n",(i+1),buffer[i].capability,buffer[i].port);
	}
	close (procFileFd);
	exit (0);

}
