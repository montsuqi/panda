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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include    <sys/socket.h>

#include	"types.h"
#include	"socket.h"
#include	"net.h"
#include	<libmondai.h>
#include	"port.h"
#include	"debug.h"

static	void
Flush(
	NETFILE	*fp)
{
	char	*p = fp->buff;
	ssize_t	count;

	while	(  fp->ptr  >  0  ) {
		if		(  ( count = fp->write(fp,p,fp->ptr) )  >  0  ) {
			fp->ptr -= count;
			p += count;
		} else {
			break;
		}
	}
}

extern	int
Send(
	NETFILE	*fp,
	void	*buff,
	size_t	size)
{
	char	*p = buff;
	ssize_t	count
		,	left;
	int		ret;

	if		(  fp->fOK  ) {
		if		(	(  fp->buff  !=  NULL  )
				&&	(  fp->size  >  size  ) ) {
			if		(  size + fp->ptr  >  fp->size  ) {
				left = fp->size - fp->ptr;
				memcpy((fp->buff + fp->ptr),buff,left);
				fp->ptr = fp->size;
				Flush(fp);
				buff += left;
				size -= left;
			}
			memcpy((fp->buff + fp->ptr),buff,size);
			fp->ptr += size;
			fp->fSent = TRUE;
		} else {
			Flush(fp);
			ret = size;
			while	(  size  >  0  ) {
				if		(  ( count = fp->write(fp,p,size) )  >  0  ) {
					size -= count;
					p += count;
				} else {
					ret = -1;
					break;
				}
			}
			fp->fSent = FALSE;
		}
	} else {
		ret = 0;
	}
	return	(ret);
}

extern	int
Recv(
	NETFILE	*fp,
	void	*buff,
	size_t	size)
{
	char	*p;
	ssize_t	count;
	int		ret;

	if		(  fp->fOK  ) {
		if		(  fp->fSent  ) {
			Flush(fp);
		}
		ret = size;
		p = buff;
		while	(  size  >  0  ) {
			if		(  ( count = fp->read(fp,p,size) )  >  0  ) {
				size -= count;
				p += count;
			} else {
				ret = -1;
				break;
			}
		}
	} else {
		ret = 0;
	}
	fp->fSent = FALSE;
	return	(ret);
}

extern	int
nputc(
	int		c,
	NETFILE	*fp)
{
	unsigned	char	ch;

	ch = c;
	return	(Send(fp,&ch,1));
}

extern	int
ngetc(
	NETFILE	*fp)
{
	unsigned char	ch;
	size_t	s;
	int		ret;

	if		(  ( s = Recv(fp,&ch,1) )  >=  0  ) {
		ret = ch;
	} else {
		ret = -1;
	}
	return	(ret);
}

extern	void
FreeNet(
	NETFILE	*fp)
{
	if		(  fp->buff  !=  NULL  ) {
		xfree(fp->buff);
	}
	xfree(fp);
}

extern	void
CloseNet(
	NETFILE	*fp)
{
	Flush(fp);
	fp->close(fp);
	FreeNet(fp);
}

extern	void
NetSetFD(
	NETFILE	*fp,
	int		fd)
{
	if		(  fp  !=  NULL  ) {
		fp->fd = fd;
	}
	fp->fOK = TRUE;
}

/*
 *	socket
 */
static	ssize_t
FD_Read(
	NETFILE	*fp,
	void	*buff,
	size_t	size)
{
	ssize_t	ret;

	if		(  ( ret = read(fp->fd,buff,size) )  <=  0  ) {
		if		(  ret  <  0  ) {
			fprintf(stderr,"read %s\n",strerror(errno));
		}
		fp->fOK = FALSE;
		fp->err = errno;
	}
	return	(ret);
}

static	ssize_t
FD_Write(
	NETFILE	*fp,
	void	*buff,
	size_t	size)
{
	ssize_t	ret;

	if		(  ( ret = write(fp->fd,buff,size) )  <=  0  ) {
		if		(  ret  <  0  ) {
			fprintf(stderr,"write %s\n",strerror(errno));
		}
		fp->fOK = FALSE;
		fp->err = errno;
	}
	return	(ret);
}

static	void
FD_Close(
	NETFILE	*fp)
{
	close(fp->fd);
}

extern	NETFILE	*
NewNet(void)
{
	NETFILE	*fp;

	fp = New(NETFILE);
	fp->fOK = TRUE;
	fp->err = 0;
	fp->read = NULL;
	fp->write = NULL;
	fp->close = NULL;
	fp->fSent = FALSE;
	fp->size = 0;
	fp->ptr = 0;
	fp->buff = NULL;
	return	(fp);
}

extern	NETFILE	*
SocketToNet(
	int		fd)
{
	NETFILE	*fp;

	fp = NewNet();
	SetNodelay(fd);
	fp->fd = fd;
	fp->read = FD_Read;
	fp->write = FD_Write;
	fp->close = FD_Close;
#if	1
	fp->buff = xmalloc(SIZE_BUFF);
	fp->size = SIZE_BUFF;
	fp->ptr = 0;
#endif
	return	(fp);
}

/*
 *	file
 */

extern	NETFILE	*
FileToNet(
	int		fd)
{
	NETFILE	*fp;

	fp = NewNet();
	fp->fd = fd;
	fp->read = FD_Read;
	fp->write = FD_Write;
	fp->close = FD_Close;
	return	(fp);
}

/*
 *	SSL
 */
#ifdef	USE_SSL
static	ssize_t
SSL_Read(
	NETFILE	*fp,
	void	*buff,
	size_t	size)
{
	ssize_t	ret;
	unsigned long	err;

	if		(  ( ret = SSL_read(fp->net.ssl,buff,size) )  <=  0  ) {
		if		(  ret  <  0  ) {
			err = ERR_get_error();
			fprintf(stderr,"read %s\n",ERR_error_string(err, NULL));
		} else {
			err = 0;
		}
		fp->fOK = FALSE;
		fp->err = (int)err;
	}
	return	(ret);
}
static	ssize_t
SSL_Write(
	NETFILE	*fp,
	void	*buff,
	size_t	size)
{
	ssize_t	ret;
	unsigned long	err;

	if		(  ( ret = SSL_write(fp->net.ssl,buff,size) )  <=  0  ) {
		if		(  ret  <  0  ) {
			err = ERR_get_error();
			fprintf(stderr,"write %s\n",ERR_error_string(err, NULL));
		} else {
			err = 0;
		}
		fp->fOK = FALSE;
		fp->err = (int)err;
	}
	return	(ret);
}
static	void
SSL_Close(
	NETFILE	*fp)
{
	SSL_shutdown(fp->net.ssl);
	close(SSL_get_fd(fp->net.ssl));
	SSL_free(fp->net.ssl); 
}
extern	NETFILE	*
MakeSSL_Net(
	SSL_CTX	*ctx,
	int		fd)
{
	NETFILE	*fp;
	SSL		*ssl;

	if		(  ( ssl = SSL_new(ctx)  )  !=  NULL  ) {
		SSL_set_fd(ssl,fd);
		fp = NewNet();
		fp->fd = fd;
		fp->net.ssl = ssl;
		fp->read = SSL_Read;
		fp->write = SSL_Write;
		fp->close = SSL_Close;
	} else {
		fp = NULL;
	}
	return	(fp);
}

static	int
VerifyCallBack(
	int		ok,
	X509_STORE_CTX	*ctx)
{
	char buf[256];
	X509 *err_cert;
	int err,depth;
	BIO	*bio_err;

	bio_err = BIO_new_fp(stderr,BIO_NOCLOSE);
	err_cert = X509_STORE_CTX_get_current_cert(ctx);
	err = X509_STORE_CTX_get_error(ctx);
	depth = X509_STORE_CTX_get_error_depth(ctx);

	X509_NAME_oneline(X509_get_subject_name(err_cert),buf,sizeof buf);
	if	(!ok) {
#ifdef	DEBUG
		BIO_printf(bio_err,"depth=%d\n",depth);
#endif
		BIO_printf(bio_err,"verify error:%s:%s\n",
				   X509_verify_cert_error_string(err),buf);
	}
	switch (ctx->error) {
	  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
		X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert),buf,sizeof buf);
		BIO_printf(bio_err,"issuer= %s\n",buf);
		break;
	  case X509_V_ERR_CERT_NOT_YET_VALID:
	  case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
		BIO_printf(bio_err,"notBefore=");
		ASN1_TIME_print(bio_err,X509_get_notBefore(ctx->current_cert));
		BIO_printf(bio_err,"\n");
		break;
	  case X509_V_ERR_CERT_HAS_EXPIRED:
	  case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
		BIO_printf(bio_err,"notAfter=");
		ASN1_TIME_print(bio_err,X509_get_notAfter(ctx->current_cert));
		BIO_printf(bio_err,"\n");
		break;
	}
	BIO_free(bio_err);
	return(ok);
}

extern	SSL_CTX	*
MakeCTX(
	char	*key,
	char	*cert,
	char	*cafile,
	char	*capath,
	Bool	fVeri)
{
	SSL_CTX	*ctx;
	int		mode;

	if		(  key  ==  NULL  ) {
		key = cert;
	}
	if		(  ( ctx = SSL_CTX_new(SSLv23_method()) )  !=  NULL  ) {
		if		(  fVeri  ) {
			mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
		} else {
			mode = SSL_VERIFY_NONE;
		}
		SSL_CTX_set_verify(ctx,mode,VerifyCallBack);
		if		(	(  cafile  !=  NULL  )
				||	(  capath  !=  NULL  ) )	{
			if		(  !SSL_CTX_load_verify_locations(ctx,cafile,capath)  ) {
				fprintf(stderr,"verify location error\n");
				SSL_CTX_free(ctx);
				ctx = NULL;
			}
		} else {
			fprintf(stderr,"not verify\n");
		}
		if		(  cert  !=  NULL  ) {
			if		(  SSL_CTX_use_certificate_file(ctx,cert,SSL_FILETYPE_PEM)  <=  0  ) {
				fprintf(stderr,"unable to get certificate from '%s'\n",cert);
				SSL_CTX_free(ctx);
				ctx = NULL;
			}
			if		(  SSL_CTX_use_PrivateKey_file(ctx,key,SSL_FILETYPE_PEM)  <=  0  ) {
				fprintf(stderr,"unable to get private key from '%s'\n",key);
				SSL_CTX_free(ctx);
				ctx = NULL;
			}
			if		(  !SSL_CTX_check_private_key(ctx)  ) {
				fprintf(stderr,"Private key does not match the certificate public key\n");
				SSL_CTX_free(ctx);
				ctx = NULL;
			}
		} else {
			fprintf(stderr,"certificate is not defined.\n");
		}
	} else {
		fprintf(stderr,"SSL error.\n");
	}
	return	(ctx);
}
#endif	/*	USE_SSL	*/

extern	void
InitNET(void)
{
#ifdef	USE_SSL
	SSL_load_error_strings(); 
	SSL_library_init(); 
#endif
}

extern	NETFILE	*
OpenPort(
	char	*url,
	char	*defport)
{
	int		fd;
	Port	*port;
	NETFILE	*fp;

	port = ParPort(url,defport);
	fd = ConnectSocket(port,SOCK_STREAM);
	DestroyPort(port);
	fp = SocketToNet(fd);
	return	(fp);
}

extern	int
InitServerPort(
	Port	*port,	
	int		back)
{
	int		fd;

ENTER_FUNC;
	fd = BindSocket(port,SOCK_STREAM);
	if		(  listen(fd,back)  <  0  )	{
		shutdown(fd, 2);
		Error("INET Domain listen");
	}
LEAVE_FUNC;
	return	(fd);
}

extern	Bool
CheckNetFile(
	NETFILE	*fp)
{
	Bool	ret;

	if		(  fp->fOK  ) {
		ret = TRUE;
	} else {
		dbgmsg("bad net file");
		ret = FALSE;
	}
	return	(ret);
}
