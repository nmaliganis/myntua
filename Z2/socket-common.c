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
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <crypto/cryptodev.h>
#include "socket-common.h"

ssize_t insist_read(int fd, void *buf, size_t cnt)
{
        ssize_t ret;
        size_t orig_cnt = cnt;

        while (cnt > 0) {
                ret = read(fd, buf, cnt);
                if (ret < 0)
                        return ret;
                buf += ret;
                cnt -= ret;
        }

        return orig_cnt;
}

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
int decrypt(int cfd, unsigned char *input_buf,
	unsigned char *data_iv, struct session_op sess ,
	unsigned char *data_decrypted){
	
	struct crypt_op cryp;

	memset(&cryp, 0, sizeof(cryp));
	
	if (ioctl(cfd, CIOCGSESSION, &sess)) {
		perror("ioctl(CIOCGSESSION)");
		return 1;
	}
	cryp.ses = sess.ses;
	cryp.len = DATA_SIZE;
	cryp.src = input_buf;
	cryp.dst = data_decrypted;
	cryp.iv = data_iv;
	cryp.op = COP_DECRYPT;

	if (ioctl(cfd, CIOCCRYPT, &cryp)) {
		perror("ioctl(CIOCCRYPT)");
		return 1;
	}

	if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
		perror("ioctl(CIOCFSESSION)");
		return 1;
	}
	return 0;
}

int encrypt(int cfd, int sfd, unsigned char *input_buf,
	unsigned char *data_iv, struct session_op sess, size_t cnt){
	
	struct crypt_op cryp;
	struct {
		unsigned char 	in[DATA_SIZE],
				encrypted[DATA_SIZE],
				decrypted[DATA_SIZE];
	} data;

	memset(&cryp, 0, sizeof(cryp));

	if (ioctl(cfd, CIOCGSESSION, &sess)) {
		perror("ioctl(CIOCGSESSION)");
		return 1;
	}
	cryp.ses = sess.ses;
	cryp.len = DATA_SIZE;
	cryp.src = input_buf;
	cryp.dst = data.encrypted;
	cryp.iv = data_iv;
	cryp.op = COP_ENCRYPT;
			
	if (ioctl(cfd, CIOCCRYPT, &cryp)) {
		perror("ioctl(CIOCCRYPT)");
		return 1;
	}
	//strlen((char *)data.encrypted)
	if (insist_write(sfd, data.encrypted, 256) != 256) {
		perror("write");
		exit(1);
	}

	if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
		perror("ioctl(CIOCFSESSION)");
		return 1;
	}

	return 0;
}