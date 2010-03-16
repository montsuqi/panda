/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2008 Ogochan & JMA (Japan Medical Association).
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef	_INC_NET_H
#define	_INC_NET_H
#include	<stdio.h>
#include	<pthread.h>
#ifdef	USE_SSL
#include	<openssl/crypto.h>
#include	<openssl/x509.h>
#include	<openssl/x509v3.h>
#include	<openssl/pem.h>
#include	<openssl/ssl.h>
#include	<openssl/err.h>
#include	<openssl/pkcs12.h>
#ifdef USE_PKCS11
#include	<opensc/rsaref/unix.h>
#include	<opensc/rsaref/pkcs11.h>
#include	<openssl/engine.h>
#include	<dlfcn.h>
#define		PKCS11_MAX_SLOT_NUM			16
#define		PKCS11_MAX_OBJECT_NUM		16
#define		PKCS11_BUF_SIZE				256
#define		PKCS11_OBJECT_SIZE			4096
#endif	/* USE_PKCS11 */
#endif	/* USE_SSL */

#include	"libmondai.h"
#include	"socket.h"

typedef	struct _NETFILE	{
	int		fd;
	union {
		FILE	*fp;
#ifdef	USE_SSL
		SSL		*ssl;
#endif
	}	net;
#ifdef	USE_SSL
	X509	*peer_cert;
#endif
	Bool	fOK
	,		fSent;
	int		err;
	size_t	size
	,		ptr;
	unsigned char	*buff;
#ifdef	MT_NET
	pthread_mutex_t	lock;
#endif
	ssize_t	(*read)(struct _NETFILE *fp, void *buf, size_t count);
	ssize_t	(*write)(struct _NETFILE *fp, void *buf, size_t count);
	void	(*close)(struct _NETFILE *fp);
}	NETFILE;

extern	Bool		Flush(NETFILE *fp);
extern	int			Send(NETFILE *fp, void *buff, size_t size);
extern	int			Recv(NETFILE *fp, void *buff, size_t size);
extern	int			RecvAtOnce(NETFILE *fp, char *buff, size_t size);
extern	int			nputc(int c, NETFILE *fp);
extern	int			ngetc(NETFILE *fp);

extern	void		FreeNet(NETFILE *fp);
extern	void		CloseNet(NETFILE *fp);
extern	NETFILE		*NewNet(void);
extern	NETFILE		*SocketToNet(int s);
extern	NETFILE		*FileToNet(int f);
extern	void		NetSetFD(NETFILE *fp, int fd);
extern	void		InitNET(void);
#define	NetGetFD(fp)	((fp)->fd)
#ifdef	USE_SSL
extern	void 		SetAskPassFunction(
						int (*func)(
							char *buf, 
							size_t buflen, 
							const char *prompt));
extern  char		*GetSSLErrorMessage(void);
extern  char		*GetSSLWarningMessage(void);
extern	NETFILE		*MakeSSL_Net(SSL_CTX *ctx, int fd);
extern	SSL_CTX		*MakeSSL_CTX(char *key, char *cert, char *cafile, char *capath, char *ciphers);
extern	char		*GetSubjectFromCertificate(X509 *cert);
extern	char 		*GetCommonNameFromCertificate(X509 *cert);
extern	Bool		CheckHostnameInCertificate(X509 *cert, const char *host);
extern	Bool		StartSSLClientSession(NETFILE *fp, const char *host);
extern	Bool		StartSSLServerSession(NETFILE *fp);
#define	NETFILE_SSL(fp)		((fp)->net.ssl)
#ifdef  USE_PKCS11
extern	SSL_CTX		*MakeSSL_CTX_PKCS11(ENGINE **e,
										char *pkcs11, 
										char *slot, 
										char *cafile, 
										char *capath, 
										char *ciphers);
#endif	/* USE_PKCS11 */
#endif	/* USE_SSL */
extern	NETFILE		*OpenPort(char *url, char *defport);
extern	int			InitServerPort(Port *port, int back);

extern	Bool		CheckNetFile(NETFILE *fp);
#define	SetErrorNetFile(fp)		(fp)->fOK = FALSE

#define	ON_IO_ERROR(fp,label)	if (!CheckNetFile(fp)) goto label

#endif
