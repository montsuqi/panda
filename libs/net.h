/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

#ifndef	_INC_NET_H
#define	_INC_NET_H

#ifdef	USE_SSL
#include	<openssl/ssl.h>
#endif

#include	"value.h"
typedef	struct _NETFILE	{
	union {
		int		fd;
#ifdef	USE_SSL
		SSL		*ssl;
#endif
	}	net;
	Bool	fOK;
	int		err;
	ssize_t	(*read)(struct _NETFILE *fp, void *buf, size_t count);
	ssize_t	(*write)(struct _NETFILE *fp, void *buf, size_t count);
	void	(*close)(struct _NETFILE *fp);
}	NETFILE;

extern	int		Send(NETFILE *fp, void *buff, size_t size);
extern	int		Recv(NETFILE *fp, void *buff, size_t size);
extern	void	CloseNet(NETFILE *fp);
extern	NETFILE	*SocketToNet(int s);
extern	void	InitNET(void);
#ifdef	USE_SSL
extern	NETFILE	*MakeSSL_Net(SSL_CTX *ctx, int fd);
extern	SSL_CTX	*MakeCTX(char *key, char *cert, char *cafile, char *capath, Bool fVeri);
#define	NETFILE_SSL(fp)		((fp)->net.ssl)
#endif
extern	NETFILE	*OpenPort(char *url, int port);
extern	int		InitServerPort(char *port, int back);

extern	Bool	_CheckNetFile(NETFILE *fp);

#define	ON_IO_ERROR(fp,label)	if (!_CheckNetFile(fp)) goto label

#endif
