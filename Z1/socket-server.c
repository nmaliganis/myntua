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

//helper function to get a timestamp for a message
void get_time(struct timeval *tv, char *timestamp, int size){
	gettimeofday(tv,NULL);
	strftime(timestamp, size,"[%d/%m/%Y %H:%M:%S]",	localtime(&tv->tv_sec));
}

int main(void)
{
	fd_set rdfs;
	struct timeval tv;
	char timestamp[64];
	int retval;
	char buf[BUFSIZ];
	char addrstr[INET_ADDRSTRLEN];
	int sd, newsd;
	ssize_t n;
	socklen_t len;
	struct sockaddr_in sa;

	/* Make sure a broken connection doesn't kill us */
	signal(SIGPIPE, SIG_IGN);

	/* Create TCP/IP socket, used as main chat channel */
	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	fprintf(stderr, "Created TCP socket\n");

	/* Bind to a well-known port */
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(TCP_PORT);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("bind");
		exit(1);
	}
	fprintf(stderr, "Bound TCP socket to port %d\n", TCP_PORT);

	/* Listen for incoming connections */
	if (listen(sd, TCP_BACKLOG) < 0) {
		perror("listen");
		exit(1);
	}

	/* Loop forever, accept()ing connections */
	for (;;) {
		fprintf(stderr, "Waiting for an incoming connection...\n");

		/* Accept an incoming connection */
		len = sizeof(struct sockaddr_in);
		if ((newsd = accept(sd, (struct sockaddr *)&sa, &len)) < 0) {
			perror("accept");
			exit(1);
		}
		if (!inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr))) {
			perror("could not format IP address");
			exit(1);
		}
		fprintf(stderr, "Incoming connection from %s:%d\n",

		addrstr, ntohs(sa.sin_port));

		/* We break out of the loop when the remote peer goes away */
		int socket_fd = newsd;
		
		fprintf(stdout,"\r\033[34;1mserver\033[0m ");
		fflush(stdout);
		for (;;) {

			FD_ZERO(&rdfs);
			FD_SET(0, &rdfs);
			FD_SET(socket_fd, &rdfs);

			retval = select(socket_fd +1, &rdfs, NULL, NULL, NULL);
			if(retval == -1){
				perror("select");
				exit(1);
			}


			if(FD_ISSET(0, &rdfs)){

				// read stdin
				n = read(0,buf,BUFSIZ);
				if (n < 0) {
					perror("read");
					exit(1);
				}

				// refresh the prompt
				fprintf(stdout,"\r\033[34;1mserver \033[0m");


				// send the message only if it starts with an alphanumeric character
				if (isalpha(buf[0]) && insist_write(newsd, buf, strlen(buf)) != strlen(buf)) {
					perror("write");
					exit(1);
				}

				//  if the message was sent, print our own version of the message and 
				// with a local timestamp
				if (isalpha(buf[0])){
					get_time(&tv,timestamp,sizeof(timestamp));
					fprintf(stdout,"\r\033[1A");
					fprintf(stdout,"%s \033[34;1mserver \033[0m%s",timestamp,buf);
					fprintf(stdout,"\r\033[34;1mserver \033[0m");
				}

				memset(buf, 0, BUFSIZ * sizeof(char));
			}

			if(FD_ISSET(socket_fd, &rdfs)){

				// read the message and see if the peer is still there

				n = read(newsd, buf, BUFSIZ);
				if (n < 0) {
					perror("read");
					exit(1);
				} else if (n == 0){
					perror("remote peer went away");
					break;
				}
			
				//get the timestamp for the received message, print it and restore the prompt
				// \033D = scroll terminal down one line
				// \033[1A = move the cursor up one line

				get_time(&tv,timestamp,sizeof(timestamp));

				fprintf(stdout,"\033D\033[1A\r");
				fprintf(stdout,"%s \033[33;1mclient \033[0m%s",timestamp,buf);
				fprintf(stdout,"\r\033[34;1mserver \033[0m");

				//clear buffer
				memset(buf, 0 , BUFSIZ * sizeof(char));

			}

			//force output
			fflush(stdout);
		}
		/* Make sure we don't leak open files */
		if (close(newsd) < 0)
			perror("close");
	}

	/* This will never happen */
	return 1;
}
