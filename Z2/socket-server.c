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
#include <crypto/cryptodev.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>
#include "socket-common.h"

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
	int sd, newsd, i;
	ssize_t n;
	socklen_t len;
	struct sockaddr_in sa;
	struct session_op sess;
	unsigned char data_in[DATA_SIZE];
	unsigned char data_out[DATA_SIZE];
	unsigned char data_iv[BLOCK_SIZE];
	unsigned char data_key[KEY_SIZE];

	memset(&sess, 0, sizeof(sess));

	int crypto_fd = open("/dev/crypto", O_RDWR);
	if (crypto_fd < 0) {
		perror("open(/dev/crypto)");
	}

	for (i = 0; i < BLOCK_SIZE; i++){
		data_iv[i] = '1';
		data_key[i] = i+'0';
	}

	sess.cipher = CRYPTO_AES_CBC;
	sess.keylen = KEY_SIZE;
	sess.key = data_key;

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

				/* read stdin */
				n = read(0,buf,BUFSIZ);
				if (n < 0) {
					perror("read");
					exit(1);
				}
				
				//refresh the prompt and force it to appear
				fprintf(stdout,"\r\033[34;1mserver \033[0m");
				fflush(stdout);

				//if it doesn't start with an alphanumeric keep looping
				//this is to prevent sending empty messages
				if (!isalpha(buf[0]))
					continue;

				//while n is positive chechk if it's more than DATA_SIZE. if it is
				//then encrypt a DATA_SIZE chunk of the input and keep looping.
				//On the last iteration, encrypt the remaining chunk and set n to 0
				//in order to break
				while ( n > 0){

					if (n > DATA_SIZE){
						n -= DATA_SIZE;
						memcpy(data_in,buf,DATA_SIZE);
						memmove(buf,buf+DATA_SIZE,DATA_SIZE);
					} else {
						n = 0;
						memset(data_in, 0, sizeof(data_in));
						memcpy(data_in,buf,256);

					}

					if (encrypt(crypto_fd, socket_fd, data_in, data_iv, sess, DATA_SIZE) < 0){
						perror("encrypt");
						exit(1);
					}
				}

				/*  if the message was sent, print our own version of the message 
				 *  with a local timestamp */
				
				get_time(&tv,timestamp,sizeof(timestamp));
				fprintf(stdout,"\r\033[1A");
				fprintf(stdout,"%s \033[34;1mserver \033[0m%s",timestamp,buf);
				fprintf(stdout,"\r\033[34;1mserver \033[0m");

				memset(buf, 0, BUFSIZ * sizeof(char));
			}

			if(FD_ISSET(socket_fd, &rdfs)){

				/*read the message and see if the peer is still there*/

				n = read(newsd, buf, BUFSIZ);
				if (n < 0) {
					perror("read");
					exit(1);
				} else if (n == 0){
					perror("remote peer went away");
					break;
				}


				// clear the ouput to be sure (if the chunk sent is less than DATA_SIZE
				// then remaining garbage would persist) and copy the data to decrypt it
				memset(data_out, 0, sizeof(data_out));
				memcpy(data_in, buf, DATA_SIZE);

				if (decrypt(crypto_fd, data_in, data_iv, sess, data_out) < 0){
					perror("decrypt");
					exit(1);
				}

			
				//get the timestamp for the received message, print it and restore the prompt
				// \033D = scroll terminal down one line
				// \033[1A = move the cursor up one line

				get_time(&tv,timestamp,sizeof(timestamp));

				fprintf(stdout,"\033D\033[1A\r");
				fprintf(stdout,"%s \033[33;1mclient \033[0m%s",timestamp,data_out);
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
