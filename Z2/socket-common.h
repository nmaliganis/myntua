/*
 * socket-common.h
 *
 * Simple TCP/IP communication using sockets
 *
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 */

#ifndef _SOCKET_COMMON_H
#define _SOCKET_COMMON_H

/* Compile-time options */
#define TCP_PORT    35001
#define TCP_BACKLOG 5

#define DATA_SIZE       256
#define BLOCK_SIZE      16
#define KEY_SIZE	16  /* AES128 */
#define BUF_SIZ 256

#define HELLO_THERE "Hello there!"
ssize_t insist_read(int fd, void *buf, size_t cnt);
ssize_t insist_write(int fd, const void *buf, size_t cnt);

int decrypt(int cfd, unsigned char *input_buf,
	unsigned char *data_iv, struct session_op sess ,
	unsigned char *data_decrypted);
int encrypt(int cfd, int sfd, unsigned char *input_buf,
	unsigned char *data_iv, struct session_op sess, size_t cnt);

#endif /* _SOCKET_COMMON_H */