/*
 * Copyright(c) 2015 Tim Ruehsen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This file is part of libvtls.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <vtls.h>
#include <netdb.h>

static int _get_connected_socket(const char *host, int port);

static void debugmsg(void *ctx, const char *fmt, va_list args)
{
	char buf[2048];
	struct timeval tv;
	struct tm *tp, tbuf;

	gettimeofday(&tv, NULL); // obsoleted by POSIX.1-2008, maybe use clock_gettime() ? needs -lrt
	tp = localtime_r((const time_t *)&tv.tv_sec, &tbuf); // cast avoids warning on OpenBSD

	vsnprintf(buf, sizeof(buf), fmt, args);

	printf("%02d:%02d:%02d.%03ld %s",
		tp->tm_hour, tp->tm_min, tp->tm_sec, tv.tv_usec / 1000,
		buf);
}

static void errormsg(void *ctx, const char *fmt, va_list args)
{
	debugmsg(ctx, fmt, args);
}


int main(int argc, const char *const *argv)
{
	vtls_session_t *sess = NULL;
	vtls_config_t *default_config;
	int rc, sockfd, status;
	ssize_t nbytes;
	char buf[2048];
	const char *hostname = "www.google.com";

	sockfd = _get_connected_socket(hostname, 443);

	/*
	 * Plain text connection has been established.
	 * Before we establish the TLS layer, we could send/recv plain text here.
	 */

	/* optional example of how to set default config values */
	if (vtls_config_init(&default_config,
		VTLS_CFG_TLS_VERSION, CURL_SSLVERSION_TLSv1_0,
		VTLS_CFG_VERIFY_PEER, 1,
		VTLS_CFG_VERIFY_HOST, 1	,
		VTLS_CFG_VERIFY_STATUS, 0,
		VTLS_CFG_CA_PATH, "/etc/ssl/certs",
		VTLS_CFG_CA_FILE, NULL,
		VTLS_CFG_CRL_FILE, NULL,
		VTLS_CFG_ISSUER_FILE, NULL,
		VTLS_CFG_RANDOM_FILE, NULL,
		VTLS_CFG_EGD_SOCKET, NULL,
		VTLS_CFG_CIPHER_LIST, NULL,
		VTLS_CFG_LOCK_CALLBACK, NULL,
		VTLS_CFG_ERRORMSG_CALLBACK, errormsg, NULL, /* third arg is the user context */
		VTLS_CFG_DEBUGMSG_CALLBACK, debugmsg, NULL, /* third arg is the user context */
		VTLS_CFG_CONNECT_TIMEOUT, 30*1000,
		VTLS_CFG_READ_TIMEOUT, 30*1000,
		VTLS_CFG_WRITE_TIMEOUT, 30*1000,
		NULL))
	{
		fprintf(stderr, "Failed to init default config\n");
		return 1;
	}

	/* call vtls_init(NULL) to use library defaults */
	if (vtls_init(default_config)) {
		fprintf(stderr, "Failed to init vtls\n");
		return 1;
	}

	if ((rc = vtls_session_init(&sess, NULL))) {
		fprintf(stderr, "Failed to init vtls session (%d)\n", rc);
		return 1;
	}

	if ((rc = vtls_connect(sess, sockfd, hostname))) {
		fprintf(stderr, "Failed to connect (%d)\n", rc);
		return 1;
	}
	printf("handshake done\n");
	
#define HTTP_REQUEST \
"GET / HTTP/1.1\r\n"\
"Host: www.google.com\r\n"\
"Accept: */*\r\n"\
"\r\n"

	if ((nbytes = vtls_write(sess, HTTP_REQUEST, sizeof(HTTP_REQUEST) - 1, &status)) < 0) {
		fprintf(stderr, "Failed to write (%d)\n", rc);
		return 1;
	}
	printf("data written (%zd bytes)\n", nbytes);

	while ((nbytes = vtls_read(sess, buf, sizeof(buf), &status)) >= 0) {
		fwrite(buf, 1, nbytes, stdout);
	}

	vtls_close(sess);

	vtls_config_deinit(default_config);
	vtls_session_deinit(sess);
	vtls_deinit();

	/*
	 * TLS connection has been shut down, but the TCP/IP connection is still valid.
	 * We could again send/recv plain text here.
	 */

	close(sockfd);

	return 0;
}

#ifndef SOCK_NONBLOCK
static void _set_async(int fd)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL)) < 0)
		fprintf(stderr, "Failed to get socket flags\n");

	if (fcntl(fd, F_SETFL, flags | O_NDELAY) < 0)
		fprintf(stderr, "Failed to set socket to non-blocking\n");
}
#endif

static int _get_async_socket(void)
{
	int sockfd;

#ifdef SOCK_NONBLOCK
	if ((sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) != -1) {
#else
	int on = 1;

	if ((sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) != -1) {
		int on = 1;

		_set_async(sockfd);
#endif
/*
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on)) == -1)
			fprintf(stderr, "Failed to set socket option REUSEADDR\n");

		on = 1;
		if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(on)) == -1)
			fprintf(stderr, "Failed to set socket option NODELAY\n");
*/
	}

	return sockfd;
}

static int _get_connected_socket(const char *host, int port)
{
	struct addrinfo *addrinfo, hints;
	int rc, sockfd;
	char s_port[16];

	memset(&hints, 0 ,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;

	snprintf(s_port, sizeof(s_port), "%d", port);
	if ((rc = getaddrinfo(host, s_port, &hints, &addrinfo))) {
		fprintf(stderr, "Failed to resolve %s\n", host);
		return rc;
	}

	if ((sockfd = _get_async_socket()) < 0) {
		fprintf(stderr, "Failed to get socket\n");
		freeaddrinfo(addrinfo);
		return rc;
	}

	if ((rc = connect(sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen))
		&& errno != EAGAIN
#ifdef EINPROGRESS
		&& errno != EINPROGRESS
#endif
		)
	{
		fprintf(stderr, "Failed to get socket\n");
		freeaddrinfo(addrinfo);
		return rc;
	}


	freeaddrinfo(addrinfo);
	return sockfd;
}
