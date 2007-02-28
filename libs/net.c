/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2007 Ogochan.
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

/*
#define	DEBUG
#define	TRACE
*/

/*
#define	MT_NET
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
#include	<sys/wait.h>

#include	"types.h"
#include	"socket.h"
#include	"net.h"
#include	<libmondai.h>
#include	"port.h"
#include	"debug.h"

#ifdef	MT_NET
#define	LockNet(fp)		{				\
	dbgmsg("lock");						\
	pthread_mutex_lock(&(fp)->lock);	\
}
#define	UnLockNet(fp)	{				\
	dbgmsg("unlock");					\
	pthread_mutex_unlock(&(fp)->lock);	\
	pthread_cond_signal(&(fp)->isdata);	\
}
#else
#define	LockNet(fp)
#define	UnLockNet(fp)
#endif

static	Bool
_Flush(
	NETFILE	*fp)
{
	byte	*p = fp->buff;
	ssize_t	count;

ENTER_FUNC;
	while	(	(  fp->fOK  )
			&&	(  fp->ptr  >  0  ) ) {
		if		(  ( count = fp->write(fp,p,fp->ptr) )  >  0  ) {
			fp->ptr -= count;
			p += count;
		} else {
			fp->fOK = FALSE;
			break;
		}
	}
LEAVE_FUNC;
	return	(fp->fOK);
}

extern	Bool
Flush(
	NETFILE	*fp)
{
	LockNet(fp);
	_Flush(fp);
	UnLockNet(fp);
	return	(fp->fOK);
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

	if		(	(  fp  !=  NULL  )
			&&	(  fp->fOK       ) ) {
		LockNet(fp);
		ret = size;
		if		(	(  fp->buff  !=  NULL  )
				&&	(  fp->size  >  size  ) ) {
			while	(  size + fp->ptr  >  fp->size  ) {
				left = fp->size - fp->ptr;
				memcpy((fp->buff + fp->ptr),p,left);
				fp->ptr = fp->size;
				if		(  !_Flush(fp)  )	{
					ret = -1;
					goto	quit;
				}
				p += left;
				size -= left;
			}
			memcpy((fp->buff + fp->ptr),p,size);
			fp->ptr += size;
			fp->fSent = TRUE;
		} else {
			_Flush(fp);
			while	(  size  >  0  ) {
				if		(  ( count = fp->write(fp,p,size) )  >  0  ) {
					size -= count;
					p += count;
				} else {
					ret = -1;
					fp->fOK = FALSE;
					break;
				}
			}
			fp->fSent = FALSE;
		}
		UnLockNet(fp);
	} else {
		ret = -1;
	}
 quit:
	return	(ret);
}

extern	int
Recv(
	NETFILE	*fp,
	void	*buff,
	size_t	size)
{
	char	*p = buff;
	ssize_t	count;
	int		ret;

	if		(	(  fp  !=  NULL  )
			&&	(  fp->fOK       ) ) {
		LockNet(fp);
		if		(  fp->fSent  ) {
			_Flush(fp);
		}
		ret = size;
		while	(  size  >  0  ) {
			if		(  ( count = fp->read(fp,p,size) )  >  0  ) {
				size -= count;
				p += count;
			} else {
				ret = -1;
				break;
			}
		}
		UnLockNet(fp);
	} else {
		memclear(buff,size);
		ret = -1;
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
	int		ret;

	if		(  Recv(fp,&ch,1)  ==  1  ) {
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
#ifdef	MT_NET
	pthread_mutex_destroy(&fp->lock);
	pthread_cond_destroy(&fp->isdata);
#endif
	xfree(fp);
}

extern	void
CloseNet(
	NETFILE	*fp)
{
ENTER_FUNC;
	if		(  fp  !=  NULL  ) {
		LockNet(fp);
		if		(  fp->fOK  ) {
			_Flush(fp);
		}
		fp->close(fp);
		FreeNet(fp);
	}
LEAVE_FUNC;
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
#ifdef	MT_NET
	pthread_cond_init(&fp->isdata,NULL);
	pthread_mutex_init(&fp->lock,NULL);
#endif
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
static int
askpass(const char *askpass_command, const char *prompt, char *buf, int buflen)
{
    int ret, len, status;
    int p[2];
    pid_t pid;

    if (pipe(p) < 0) {
        fprintf(stderr, "pipe: %s\n", strerror(errno));
        return -1;
    }       
    if ((pid = fork()) < 0){
        close(p[0]);
        close(p[1]);
        fprintf(stderr, "fork: %s\n", strerror(errno));
        return -1;
    }
    else if (pid == 0){
        seteuid(getuid());
        setuid(getuid());
        close(p[0]);
        if (dup2(p[1], STDOUT_FILENO) < 0){
            fprintf(stderr, "dup2: %s\n", strerror(errno));
            exit(1);
        }
        if (p[1] != STDOUT_FILENO) close(p[1]);
        execlp(askpass_command, askpass_command, prompt, (char *) 0);
        fprintf(stderr, "exec(%s): %s\n", askpass_command, strerror(errno));
        exit(1);
    }
    close(p[1]);
    len = ret = 0;
    do {
        ret = read(p[0], buf + len, buflen - 1 - len);
        if (ret == -1 && errno == EINTR) continue;
        if (ret <= 0) break;
        len += ret;
    } while (buflen - 1 - len > 0);
    buf[len] = '\0';
    buf[strcspn(buf, "\r\n")] = '\0';

    close(p[0]);
    while (waitpid(pid, &status, 0) < 0){
        fprintf(stderr, "waitpid: %s\n", strerror(errno));
        if (errno != EINTR) break;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        memset(buf, 0, sizeof(buf));
        return -1;
    }

    return strlen(buf);
}

#define ASKPASS_ENV "PANDA_ASKPASS"
#define ASKPASS_PROMPT "Please input key passphrase:"

static int
passphrase_callback(char *buf, int buflen, int flag, void *userdata)
{
    const char *askpass_command = (const char *)userdata;
    return askpass(askpass_command, ASKPASS_PROMPT, buf, buflen);
}

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
    if (fp->peer_cert)
        X509_free(fp->peer_cert);
    SSL_shutdown(fp->net.ssl);
    SSL_free(fp->net.ssl); 
    close(fp->fd);
}

static const char*
GetSSLErrorString()
{
    const char *msg;
    int e;

    e = ERR_get_error();
    msg = ERR_error_string(e, NULL);
    ERR_clear_error();

    return msg;
}

static char*
GetSubjectFromCertificate_X509_NAME_print_ex(X509 *cert)
{
    X509_NAME *subject;
    char *ret = NULL;
    BIO *out = NULL;
    BUF_MEM *buf;
    unsigned long flags = XN_FLAG_ONELINE;

    if ((subject = X509_get_subject_name(cert)) == NULL){
        return NULL;
    }

    if ((out = BIO_new(BIO_s_mem())) == NULL){
        fprintf(stderr, "BIO_new failure: %s\n", GetSSLErrorString());
        goto err;
    }
    if (!X509_NAME_print_ex(out, subject, 0, flags)){
        fprintf(stderr,"X509_NAME_print_ex failure: %s\n",GetSSLErrorString());
        goto err;
    }
    BIO_write(out, "\0", 1);
    BIO_get_mem_ptr(out, &buf);
    if ((ret = xmalloc(buf->length)) == NULL){
        goto err;
    }
    memcpy(ret, buf->data, buf->length);

err:
    BIO_free(out);

    return ret;
}

static char*
GetSubjectFromCertificate_X509_NAME_online(X509 *cert)
{
    X509_NAME *subject;
    char *ret = NULL;

    if ((subject = X509_get_subject_name(cert)) == NULL){
        return NULL;
    }
    if ((ret = xmalloc(1000)) == NULL){
        fprintf(stderr,"xmalloc failure: %s\n",strerror(errno));
        return NULL;
    }
    if (!X509_NAME_oneline(subject, ret, 1000)){
        fprintf(stderr,"X509_NAME_oneline failure: %s\n",GetSSLErrorString());
        xfree(ret);
        ret = NULL;
    }

    return ret;
}

extern char*
GetSubjectFromCertificate(X509 *cert)
{
    //return GetSubjectFromCertificate_X509_NAME_print_ex(cert);
    return GetSubjectFromCertificate_X509_NAME_online(cert);
}

static int
GetCommonNameFromCertificate__(X509 *cert, char *name, size_t len)
{
    X509_NAME *subject;
    X509_NAME_ENTRY *entry;
    char oid[16];
    int count, i;

    if ((subject = X509_get_subject_name(cert)) == NULL)
        return -1;
    count = X509_NAME_entry_count(subject);
    for (i = 0; i < count; i++){
        if ((entry = X509_NAME_get_entry(subject, i)) == NULL) break;
        if (OBJ_obj2nid(entry->object) != NID_commonName) continue;
        if (name != NULL && len > 0)
            snprintf(name, len, "%s", entry->value->data);
        return entry->value->length;
    }

    return -1;
}

char *
GetCommonNameFromCertificate(X509 *cert)
{
    char *name;
    int len;

    if (cert == NULL)
        return NULL;
    if ((len = GetCommonNameFromCertificate__(cert, NULL, 0)) < 0)
        return NULL;
    if ((name = xmalloc(len+1)) == NULL)
        return NULL;
    if ((len = GetCommonNameFromCertificate__(cert, name, len+1)) < 0){
        xfree(name);
        return NULL;
    }

    return name;
}

static int
CheckSubjectAltName(X509_EXTENSION *ext, const char *hostname)
{
    STACK_OF(CONF_VALUE) *values;
    CONF_VALUE *value;
    X509V3_EXT_METHOD *meth;
    unsigned char *data;
    int ok = FALSE;
    int i;

    data = ext->value->data;
    if ((meth = X509V3_EXT_get(ext)) == NULL) return FALSE;
    values = meth->i2v(meth, meth->d2i(NULL, &data, ext->value->length), NULL);
    if (values == NULL) return FALSE;
    for (i = 0; i < sk_CONF_VALUE_num(values); i++){
        value = sk_CONF_VALUE_value(values, i);
        if (strcmp(value->name, "DNS") != 0) continue;
        if (strcasecmp(value->value, hostname) != 0) continue;
		ok = TRUE;
        break;
    }
    sk_CONF_VALUE_free(values);

    return ok;
}

Bool
CheckHostnameInCertificate(X509 *cert, const char *hostname)
{
    X509_EXTENSION *ext;
    ASN1_OBJECT *extobj;
    int ext_count, i, oid;
    Bool should_check_cn = TRUE;
	Bool ok = FALSE;
    char *cn;

    if (cert == NULL) return FALSE;
    if (strcasecmp(hostname, "localhost") == 0) return TRUE;
    if (strcasecmp(hostname, "127.0.0.1") == 0) return TRUE;
    if (strcasecmp(hostname, "::1") == 0) return TRUE;

    /* check dNSName in subjectAltName extension */
    if ((ext_count = X509_get_ext_count(cert)) > 0){
        for (i = 0; i < ext_count; i++){
            if ((ext = X509_get_ext(cert, i)) == NULL) break;
        if ((extobj = X509_EXTENSION_get_object(ext)) == NULL) break;
            if ((oid = OBJ_obj2nid(extobj)) == NID_subject_alt_name){
                ok = CheckSubjectAltName(ext, hostname);
            }
        }
    }
    /* check commonName */
    if (ok != TRUE && should_check_cn == TRUE){
        if ((cn = GetCommonNameFromCertificate(cert)) != NULL){
            if (strcasecmp(cn, hostname) == 0) ok = TRUE;
            xfree(cn);
        }
    }

    return ok;
}

Bool
StartSSLClientSession(NETFILE *fp, const char *hostname)
{
    X509 *cert;
    Bool id_ok = FALSE;

    if (SSL_connect(fp->net.ssl) <= 0){
        fprintf(stderr, "SSL_connect failure: %s\n", GetSSLErrorString());
        return FALSE;
    }
    if ((cert = SSL_get_peer_certificate(fp->net.ssl)) != NULL){
        fp->peer_cert = cert;
        id_ok = CheckHostnameInCertificate(cert, hostname);
        if (id_ok != TRUE){
            fprintf(stderr, "hostname don't match %s\n", hostname);
            if (SSL_get_verify_mode(fp->net.ssl) & SSL_VERIFY_PEER){
                return FALSE;
            }
        }
    }

    return TRUE;
}

Bool
StartSSLServerSession(NETFILE *fp)
{
    X509 *cert;

    if (SSL_accept(fp->net.ssl) <= 0){
        fprintf(stderr, "SSL_accept failure: %s\n", GetSSLErrorString());
        return FALSE;
    }
    if ((cert = SSL_get_peer_certificate(fp->net.ssl)) != NULL){
        fp->peer_cert = cert;
    }

    return TRUE;
}

extern	NETFILE	*
MakeSSL_Net(
	SSL_CTX	*ctx,
	int		fd)
{
    NETFILE	*fp;
    SSL		*ssl;

    if ((fp = NewNet()) == NULL) return NULL;
    fp->read = SSL_Read;
    fp->write = SSL_Write;
    fp->close = SSL_Close;
    fp->peer_cert = NULL;
    fp->fd = fd;

    if ((fp->net.ssl = SSL_new(ctx)) == NULL){
        fprintf(stderr, "SSL_new failure: %s\n", GetSSLErrorString());
        FreeNet(fp);
        return NULL;
    }
    if (!SSL_set_fd(fp->net.ssl, fd)){
        fprintf(stderr, "SSL_set_fd failure: %s\n", GetSSLErrorString());
        SSL_free(fp->net.ssl);
        FreeNet(fp);
        return NULL;
    }

    return fp;
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
		BIO_printf(bio_err,"depth=%d\n",depth);
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

static char *
GetPasswordString(char *buf, size_t sz)
{
    const char *cmd;

    if ((cmd = getenv(ASKPASS_ENV)) != NULL){
        if (askpass(cmd, ASKPASS_PROMPT, buf, sz) >= 0){
            return buf;
        }
    }
    else if (isatty(STDIN_FILENO)){
        if (EVP_read_pw_string(buf, sz, ASKPASS_PROMPT, 0) == 0){
            return buf;
        }
    }

    return NULL;
}


static Bool
LoadPKCS12(SSL_CTX *ctx, const char *file)
{
    char passbuf[256];
    char *pass = NULL;
    const char *message;
    const char *cmd;
    PKCS12 *p12;
    EVP_PKEY *key = NULL;
    X509 *cert = NULL;
    BIO *input;
    int err_reason;

    /* read PKCS #12 from specified file */
    if ((input = BIO_new_file(file, "r")) == NULL){
        if (d2i_PKCS12_bio(input, &p12) == NULL) return FALSE;
    }
    p12 = d2i_PKCS12_bio(input, NULL);
    BIO_free(input);
    if (p12 == NULL) return FALSE;

    /* get key and cert from  PKCS #12 */
    for (;;){
        if (PKCS12_parse(p12, pass, &key, &cert, NULL))
            break;
        err_reason = ERR_GET_REASON(ERR_peek_error());
        if (cert){ X509_free(cert); cert = NULL; }
        if (key){ EVP_PKEY_free(key); key = NULL; }
        if (err_reason != PKCS12_R_MAC_VERIFY_FAILURE){
            message = "PKCS12_parse failure: %s\n";
            fprintf(stderr, message, GetSSLErrorString());
            break;
        }
		ERR_clear_error();
        if ((pass = GetPasswordString(passbuf, sizeof(passbuf))) == NULL){
            fprintf(stderr, "cannot read password\n");
            break;
        }
    }
    //OPENSSL_cleanse(passbuf, sizeof(passbuf));
    memset(passbuf, 0, sizeof(passbuf));
    PKCS12_free(p12);

    /* set key and cert to SSL_CTX */
    if (cert && key){
        if (!SSL_CTX_use_certificate(ctx, cert)){
            message = "SSL_CTX_use_certificate failure: %s\n";
            fprintf(stderr, message, GetSSLErrorString());
            return FALSE;
        }
        if (!SSL_CTX_use_PrivateKey(ctx, key)){
            message = "SSL_CTX_use_PrivateKey failure: %s\n";
            fprintf(stderr, message, GetSSLErrorString());
            return FALSE;
        }
        if (!SSL_CTX_check_private_key(ctx)){
            fprintf(stderr, "SSL_CTX_check_private_key failure: %s\n",
                    GetSSLErrorString());
            return FALSE;
        }
    }

    return TRUE;
}

extern	SSL_CTX	*
MakeSSL_CTX(
	char	*key,
	char	*cert,
	char	*cafile,
	char	*capath,
	char	*ciphers)
{
    SSL_CTX *ctx;
    int     mode = SSL_VERIFY_NONE;
    const char *askpass_command;

    if ((ctx = SSL_CTX_new(SSLv23_method())) == NULL){
        fprintf(stderr,"SSL_CTX_new failue: %s\n", GetSSLErrorString());
        return NULL;
    }

    if ((askpass_command = getenv(ASKPASS_ENV)) != NULL){
        SSL_CTX_set_default_passwd_cb(ctx, passphrase_callback);
        SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)askpass_command);
    }

    if (!SSL_CTX_set_cipher_list(ctx, ciphers)){
        fprintf(stderr,"SSL_CTX_set_cipher_list(%s) failue: %s\n",
                ciphers, GetSSLErrorString());
        SSL_CTX_free(ctx);
        return NULL;
    }

    mode = SSL_VERIFY_PEER;
    mode |= SSL_VERIFY_CLIENT_ONCE;
    SSL_CTX_set_verify(ctx, mode, VerifyCallBack);
    SSL_CTX_set_options(ctx, SSL_OP_ALL);

    if ((cafile == NULL) && (capath == NULL)){
        if (!SSL_CTX_set_default_verify_paths(ctx)){
            fprintf(stderr, "SSL_CTX_set_default_verify_paths error: %s\n",
                    GetSSLErrorString());
        }
    }
    else if (!SSL_CTX_load_verify_locations(ctx, cafile, capath)){
        fprintf(stderr,"SSL_CTX_load_verify_locations(%s, %s) error: %s\n",
                cafile, capath, GetSSLErrorString());
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (cert != NULL){
        if (LoadPKCS12(ctx, cert)) return ctx;
        if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) <= 0){
            fprintf(stderr,"SSL_CTX_use_certificate_file(%s) failure: %s\n",
                    cert, GetSSLErrorString());
            SSL_CTX_free(ctx);
            return NULL;
        }
        if (key == NULL) key = cert;
        for (;;){ 
            if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0){
                int err_reason;
                err_reason = ERR_GET_REASON(ERR_peek_error());
                fprintf(stderr,"SSL_CTX_use_PrivateKey_file(%s) failure: %s\n",
                        key, GetSSLErrorString());
                if (err_reason == PEM_R_BAD_DECRYPT ||
                    err_reason == EVP_R_BAD_DECRYPT) continue;
                SSL_CTX_free(ctx);
                return NULL;
            }
            break;
        }
        if (!SSL_CTX_check_private_key(ctx)){
            fprintf(stderr, "SSL_CTX_check_private_key failure: %s\n",
                    GetSSLErrorString());
            SSL_CTX_free(ctx);
            return NULL;
        }
    }

    return ctx;
}
#endif	/*	USE_SSL	*/

extern	void
InitNET(void)
{
#ifdef	USE_SSL
	OpenSSL_add_ssl_algorithms();
	OpenSSL_add_all_algorithms();	
	ERR_load_crypto_strings();
	SSL_load_error_strings();	
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
	if		(  ( fd = ConnectSocket(port,SOCK_STREAM) )  >  0  ) {
		fp = SocketToNet(fd);
	} else {
		fp = NULL;
	}
	DestroyPort(port);
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

	if		(  fp && fp->fOK  ) {
		ret = TRUE;
	} else {
		dbgmsg("bad net file");
		ret = FALSE;
	}
	return	(ret);
}
