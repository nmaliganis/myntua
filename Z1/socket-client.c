#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include "socket-common.h"


/* Insist until all of the data has been written */
ssize_t insist_write(int fd, const void *buf, size_t cnt)
{
	ssize_t ret;
	size_t orig_cnt = cnt;

	while (cnt > 0) {
	        ret = write(fd, buf, cnt);
	        if (ret < 0)
	                return ret;
	        buf += ret;
	        cnt -= ret;
	}

	return orig_cnt;
}

//helper function to get a timestamp
void get_time(struct timeval *tv, char *timestamp, int size){
	gettimeofday(tv,NULL);
	strftime(timestamp, size,"[%d/%m/%Y %H:%M:%S]",	localtime(&tv->tv_sec));
}

int main(int argc, char *argv[])
{

	fd_set rdfs;
	struct timeval tv;
	int retval;
	int sd, port;
	ssize_t n;
	char timestamp[64];
	char buf[BUFSIZ];
	char *hostname;
	struct hostent *hp;
	struct sockaddr_in sa;


	if (argc != 3) {
		fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
		exit(1);
	}
	hostname = argv[1];
	port = atoi(argv[2]); 
	/* Needs better error checking */

	/* Create TCP/IP socket, used as main chat channel */
	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	fprintf(stderr, "Created TCP socket\n");

	/* Look up remote hostname on DNS */
	if ( !(hp = gethostbyname(hostname))) {
		printf("DNS lookup failed for host %s\n", hostname);
		exit(1);
	}

	/* Connect to remote TCP port */
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	memcpy(&sa.sin_addr.s_addr, hp->h_addr, sizeof(struct in_addr));
	fprintf(stderr, "Connecting to remote host... "); fflush(stderr);
	if (connect(sd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("connect");
		exit(1);
	}
	fprintf(stderr, "Connected.\n");

	int socket_fd = sd;

	//force stdout to print the line
	fprintf(stdout,"\r\033[33;1mclient \033[0m");
	fflush(stdout);

	for (;;) {
		//creal the fd set and initialize it with stdin and socket_fd
		FD_ZERO(&rdfs);
		FD_SET(0, &rdfs);
		FD_SET(socket_fd, &rdfs);

		//select an active fd
		retval = select(socket_fd + 1, &rdfs, NULL, NULL, NULL);
		if(retval == -1){
			perror("select");
			exit(1);
		}


		if(FD_ISSET(0, &rdfs)){

			//read stdin
			n = read(0, buf, BUFSIZ);
			if (n < 0) {
				perror("read");
				exit(1);
			}
			
			//refresh the prompt
			fprintf(stdout,"\r\033[33;1mclient \033[0m");
			
			//send the message away only if it starts with an alphanumeric character
			if (isalpha(buf[0]) && insist_write(sd, buf, strlen(buf)) != strlen(buf)) {
				perror("write");
				exit(1);
			}

			//if the message was sent, print our own version of the message and timestamp
			if (isalpha(buf[0])){
				get_time(&tv,timestamp,sizeof(timestamp));
				fprintf(stdout,"\r\033[1A");
				fprintf(stdout,"%s \033[33;1mclient \033[0m%s",timestamp,buf);
				fprintf(stdout,"\r\033[33;1mclient \033[0m");
			}

			//clear the buffer to avoid issues
			memset(buf, 0 , BUFSIZ*sizeof(char));
		}

		if(FD_ISSET(socket_fd, &rdfs)){

			//read from the fd and see if the peer is still there
			n = read(sd, buf, BUFSIZ);
			if (n < 0) {
				perror("read");
				exit(1);
			} else if (n==0) {
				perror("remote peer went away");
				break;
			}
			
			//get the timestamp for the received message, print it and restore the prompt
			// \033D = scroll terminal down one line
			// \033[1A = move the cursor up one line

			get_time(&tv,timestamp,sizeof(timestamp));

			fprintf(stdout,"\033D\033[1A\r");
			fprintf(stdout,"%s \033[34;1mserver \033[0m%s",timestamp,buf);
			fprintf(stdout,"\r\033[33;1mclient \033[0m");

			//clear buffer
			memset(buf, 0 , BUFSIZ * sizeof(char));

		}
		
		//force output
		fflush(stdout);
	}

	//reachable only if the server exits on us first

	if (shutdown(socket_fd,SHUT_WR) < 0){
		perror("shutdown");
		exit(1);
	}

	if (close(socket_fd) < 0){
		perror("close");
		exit(1);
	}

	fprintf(stderr, "\nDone.\n");
	return 0;
}
