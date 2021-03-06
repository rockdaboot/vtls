#ifndef _VTLS_BACKEND_H
#define _VTLS_BACKEND_H

#include <vtls.h>
#include <errno.h>

/* Set the API backend definition to GnuTLS */
#define CURL_SSL_BACKEND CURLSSLBACKEND_GNUTLS
#define Curl_nop_stmt do { } while(0);

/*
 * Macro SOCKERRNO / SET_SOCKERRNO() returns / sets the *socket-related* errno
 * (or equivalent) on this platform to hide platform details to code using it.
 */

#ifdef USE_WINSOCK
#define SOCKERRNO         ((int)WSAGetLastError())
#define SET_SOCKERRNO(x)  (WSASetLastError((int)(x)))
#else
#define SOCKERRNO         (errno)
#define SET_SOCKERRNO(x)  (errno = (x))
#endif

#define CURL_CSELECT_IN   0x01
#define CURL_CSELECT_OUT  0x02
#define CURL_CSELECT_ERR  0x04

struct _vtls_config_st {
	void (*lock_callback)(int); /* callback function for multithread library use */
	void (*errormsg_callback)(void *, const char *, ...); /* callback function for error messages */
	void (*debugmsg_callback)(void *, const char *, ...); /* callback function for debug messages */
	void *errormsg_ctx; /* context for error messages */
	void *debugmsg_ctx; /* context for debug messages */
	const char *CApath; /* certificate directory (doesn't work on windows) */
	const char *CAfile; /* certificate to verify peer against */
	const char *CRLfile; /* CRL to check certificate revocation */
	const char *CERTfile;
	const char *KEYfile;
	const char *issuercert; /* optional issuer certificate filename */
	const char *random_file; /* path to file containing "random" data */
	const char *egdsocket; /* path to file containing the EGD daemon socket */
	const char *cipher_list; /* list of ciphers to use */
	const char *username; /* TLS username (for, e.g., SRP) */
	const char *password; /* TLS password (for, e.g., SRP) */
	int connect_timeout; /* connection timeout in ms */
	int read_timeout; /* read timeout in ms */
	int write_timeout; /* write timeout in ms */
	enum CURL_TLSAUTH authtype; /* TLS authentication type (default SRP) */
	char version; /* what TLS version the client wants to use */
	char verifypeer; /* if peer verification is requested */
	char verifyhost; /* if hostname matching is requested */
	char verifystatus; /* if certificate status check is requested */
	char cert_type; /* filetype of CERTfile and KEYfile */
};

struct _vtls_session_st {
	vtls_config_t *config;
	const char *hostname; /* SNI hostname */
	void *backend_data;
	struct timeval connect_start;
	struct timeval read_start;
	struct timeval write_start;
	int sockfd;
	int use;
	int state;
	int connecting_state;
};

/* API of backend TLS engines */
/*
#define curlssl_close_all(x) ((void)x)
#define curlssl_set_engine(x,y) ((void)x, (void)y, CURLE_NOT_BUILT_IN)
#define curlssl_set_engine_default(x) ((void)x, CURLE_NOT_BUILT_IN)
#define curlssl_engines_list(x) ((void)x, (struct curl_slist *)NULL)
#define curlssl_check_cxn(x) ((void)x, -1)
#define curlssl_data_pending(x,y) ((void)x, (void)y, 0)
 */
int backend_get_engine(void);
int backend_init(vtls_config_t *config);
int backend_deinit(void);
int backend_session_init(vtls_session_t *sess);
void backend_session_deinit(vtls_session_t *sess);
ssize_t backend_read(vtls_session_t *sess, char *buf, size_t count, int *curlcode);
ssize_t backend_write(vtls_session_t *sess, const void *buf, size_t count, int *curlcode);
int backend_connect(vtls_session_t *sess);
void backend_close(vtls_session_t *sess);
int backend_shutdown(vtls_session_t *sess);
void backend_session_free(void *ptr);
size_t backend_version(char *buffer, size_t size);
int backend_md5sum(unsigned char *tmp, /* input */
						 size_t tmplen,
						 unsigned char *md5sum, /* output */
						 size_t md5len);
int backend_cert_status_request(void);

#endif /* _VTLS_BACKEND_H */
