/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
 * Copyright (C) 2004-2006 Ogochan.
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

#if	1
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

#ifdef  USE_SSL
#ifdef  USE_PKCS11
static const char *PKCS11ErrorString(CK_RV rv);
static void *PKCS11LoadModule(const char* pkcs11, CK_FUNCTION_LIST_PTR_PTR f);
static Bool PKCS11UnloadModule(void *dl_handle, CK_FUNCTION_LIST_PTR f);
static Bool PKCS11FindPrivateKey(CK_SESSION_HANDLE session,
                CK_FUNCTION_LIST_PTR f,
                char *keyid,
                int keyid_size);
static Bool PKCS11GetCertificate(CK_SLOT_ID slot,
                CK_FUNCTION_LIST_PTR f, 
                char *pin,
                char **certder,
                int *certder_size,
                char **keyid,
                int *keyid_size);
static CK_ULONG PKCS11GetSlotList(CK_FUNCTION_LIST_PTR f, CK_SLOT_ID_PTR list);
static ENGINE *InitEnginePKCS11( const char *pkcs11, const char *pin);
static char *GetPinString(char *buf, size_t sz);
static Bool LoadEnginePKCS11(SSL_CTX *ctx, ENGINE **e, const char *pkcs11, const char *slotstr);
#endif
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
	pthread_mutex_destroy(&fp->lock);
	pthread_cond_destroy(&fp->isdata);
	xfree(fp);
}

extern	void
CloseNet(
	NETFILE	*fp)
{
ENTER_FUNC;
	if		(  fp  !=  NULL  ) {
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
			Warning("read %s\n",strerror(errno));
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
	pthread_cond_init(&fp->isdata,NULL);
	pthread_mutex_init(&fp->lock,NULL);
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
        Warning("pipe: %s\n", strerror(errno));
        return -1;
    }       
    if ((pid = fork()) < 0){
        close(p[0]);
        close(p[1]);
        Warning("fork: %s\n", strerror(errno));
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
        Error("exec(%s): %s\n", askpass_command, strerror(errno));
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
        Warning("waitpid: %s\n", strerror(errno));
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
			Warning("read %s\n",ERR_error_string(err, NULL));
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
			Warning("write %s\n",ERR_error_string(err, NULL));
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
        Warning("BIO_new failure: %s\n", GetSSLErrorString());
        goto err;
    }
    if (!X509_NAME_print_ex(out, subject, 0, flags)){
        Warning("X509_NAME_print_ex failure: %s\n",GetSSLErrorString());
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
        Warning("xmalloc failure: %s\n",strerror(errno));
        return NULL;
    }
    if (!X509_NAME_oneline(subject, ret, 1000)){
        Warning("X509_NAME_oneline failure: %s\n",GetSSLErrorString());
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
        Warning("SSL_connect failure: %s\n", GetSSLErrorString());
        return FALSE;
    }
    if ((cert = SSL_get_peer_certificate(fp->net.ssl)) != NULL){
        fp->peer_cert = cert;
        id_ok = CheckHostnameInCertificate(cert, hostname);
        if (id_ok != TRUE){
            Warning("hostname don't match %s\n", hostname);
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
        Warning("SSL_accept failure: %s\n", GetSSLErrorString());
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

    if ((fp = NewNet()) == NULL) return NULL;
    fp->read = SSL_Read;
    fp->write = SSL_Write;
    fp->close = SSL_Close;
    fp->peer_cert = NULL;
    fp->fd = fd;

    if ((fp->net.ssl = SSL_new(ctx)) == NULL){
        Warning("SSL_new failure: %s\n", GetSSLErrorString());
        FreeNet(fp);
        return NULL;
    }
    if (!SSL_set_fd(fp->net.ssl, fd)){
        Warning("SSL_set_fd failure: %s\n", GetSSLErrorString());
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
            Warning("%s: %s", message, GetSSLErrorString());
            break;
        }
		ERR_clear_error();		
        if ((pass = GetPasswordString(passbuf, sizeof(passbuf))) == NULL){
            Warning("cannot read password\n");
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
            Warning("%s: %s", message, GetSSLErrorString());
            return FALSE;
        }
        if (!SSL_CTX_use_PrivateKey(ctx, key)){
            message = "SSL_CTX_use_PrivateKey failure: %s\n";
            Warning("%s: %s",message, GetSSLErrorString());
            return FALSE;
        }
        if (!SSL_CTX_check_private_key(ctx)){
            Warning("SSL_CTX_check_private_key failure: %s\n",
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
        Warning("SSL_CTX_new failue: %s\n", GetSSLErrorString());
        return NULL;
    }

    if ((askpass_command = getenv(ASKPASS_ENV)) != NULL){
        SSL_CTX_set_default_passwd_cb(ctx, passphrase_callback);
        SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)askpass_command);
    }

    if (!SSL_CTX_set_cipher_list(ctx, ciphers)){
        Warning("SSL_CTX_set_cipher_list(%s) failue: %s\n",
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
            Warning("SSL_CTX_set_default_verify_paths error: %s\n",
                    GetSSLErrorString());
        }
    }
    else if (!SSL_CTX_load_verify_locations(ctx, cafile, capath)){
        Warning("SSL_CTX_load_verify_locations(%s, %s) error: %s\n",
                cafile, capath, GetSSLErrorString());
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (cert != NULL){
        if (LoadPKCS12(ctx, cert)) return ctx;
        if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) <= 0){
            Warning("SSL_CTX_use_certificate_file(%s) failure: %s\n",
                    cert, GetSSLErrorString());
            SSL_CTX_free(ctx);
            return NULL;
        }
        if (key == NULL) key = cert;
        for (;;){ 
            if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0){
                int err_reason;
                err_reason = ERR_GET_REASON(ERR_peek_error());
                Warning("SSL_CTX_use_PrivateKey_file(%s) failure: %s\n",
                        key, GetSSLErrorString());
                if (err_reason == PEM_R_BAD_DECRYPT ||
                    err_reason == EVP_R_BAD_DECRYPT) continue;
                SSL_CTX_free(ctx);
                return NULL;
            }
            break;
        }
        if (!SSL_CTX_check_private_key(ctx)){
            Warning("SSL_CTX_check_private_key failure: %s\n",
                    GetSSLErrorString());
            SSL_CTX_free(ctx);
            return NULL;
        }
    }

    return ctx;
}

/*
 * SECURITY_DEVICE
 */
#ifdef USE_PKCS11

/*
 * PKCS11 library
 */
#define PKCS11_ERR_MSG(x) case x: return #x; break;

static const char*
PKCS11ErrorString(CK_RV rv)
{
    switch (rv) {
        PKCS11_ERR_MSG(CKR_OK)
        PKCS11_ERR_MSG(CKR_CANCEL)
        PKCS11_ERR_MSG(CKR_HOST_MEMORY)
        PKCS11_ERR_MSG(CKR_SLOT_ID_INVALID)
        PKCS11_ERR_MSG(CKR_GENERAL_ERROR)
        PKCS11_ERR_MSG(CKR_FUNCTION_FAILED)
        PKCS11_ERR_MSG(CKR_ARGUMENTS_BAD)
        PKCS11_ERR_MSG(CKR_NO_EVENT)
        PKCS11_ERR_MSG(CKR_NEED_TO_CREATE_THREADS)
        PKCS11_ERR_MSG(CKR_CANT_LOCK)
        PKCS11_ERR_MSG(CKR_ATTRIBUTE_READ_ONLY)
        PKCS11_ERR_MSG(CKR_ATTRIBUTE_SENSITIVE)
        PKCS11_ERR_MSG(CKR_ATTRIBUTE_TYPE_INVALID)
        PKCS11_ERR_MSG(CKR_ATTRIBUTE_VALUE_INVALID)
        PKCS11_ERR_MSG(CKR_DATA_INVALID)
        PKCS11_ERR_MSG(CKR_DATA_LEN_RANGE)
        PKCS11_ERR_MSG(CKR_DEVICE_ERROR)
        PKCS11_ERR_MSG(CKR_DEVICE_MEMORY)
        PKCS11_ERR_MSG(CKR_DEVICE_REMOVED)
        PKCS11_ERR_MSG(CKR_ENCRYPTED_DATA_INVALID)
        PKCS11_ERR_MSG(CKR_ENCRYPTED_DATA_LEN_RANGE)
        PKCS11_ERR_MSG(CKR_FUNCTION_CANCELED)
        PKCS11_ERR_MSG(CKR_FUNCTION_NOT_PARALLEL)
        PKCS11_ERR_MSG(CKR_FUNCTION_NOT_SUPPORTED)
        PKCS11_ERR_MSG(CKR_KEY_HANDLE_INVALID)
        PKCS11_ERR_MSG(CKR_KEY_SIZE_RANGE)
        PKCS11_ERR_MSG(CKR_KEY_TYPE_INCONSISTENT)
        PKCS11_ERR_MSG(CKR_KEY_NOT_NEEDED)
        PKCS11_ERR_MSG(CKR_KEY_CHANGED)
        PKCS11_ERR_MSG(CKR_KEY_NEEDED)
        PKCS11_ERR_MSG(CKR_KEY_INDIGESTIBLE)
        PKCS11_ERR_MSG(CKR_KEY_FUNCTION_NOT_PERMITTED)
        PKCS11_ERR_MSG(CKR_KEY_NOT_WRAPPABLE)
        PKCS11_ERR_MSG(CKR_KEY_UNEXTRACTABLE)
        PKCS11_ERR_MSG(CKR_MECHANISM_INVALID)                 
        PKCS11_ERR_MSG(CKR_MECHANISM_PARAM_INVALID)
        PKCS11_ERR_MSG(CKR_OBJECT_HANDLE_INVALID)
        PKCS11_ERR_MSG(CKR_OPERATION_ACTIVE)
        PKCS11_ERR_MSG(CKR_OPERATION_NOT_INITIALIZED)
        PKCS11_ERR_MSG(CKR_PIN_INCORRECT)
        PKCS11_ERR_MSG(CKR_PIN_INVALID)
        PKCS11_ERR_MSG(CKR_PIN_LEN_RANGE)
        PKCS11_ERR_MSG(CKR_PIN_EXPIRED)
        PKCS11_ERR_MSG(CKR_PIN_LOCKED)
        PKCS11_ERR_MSG(CKR_SESSION_CLOSED)
        PKCS11_ERR_MSG(CKR_SESSION_COUNT)
        PKCS11_ERR_MSG(CKR_SESSION_HANDLE_INVALID)
        PKCS11_ERR_MSG(CKR_SESSION_PARALLEL_NOT_SUPPORTED)
        PKCS11_ERR_MSG(CKR_SESSION_READ_ONLY)
        PKCS11_ERR_MSG(CKR_SESSION_EXISTS)
        PKCS11_ERR_MSG(CKR_SESSION_READ_ONLY_EXISTS)
        PKCS11_ERR_MSG(CKR_SESSION_READ_WRITE_SO_EXISTS)
        PKCS11_ERR_MSG(CKR_SIGNATURE_INVALID)
        PKCS11_ERR_MSG(CKR_SIGNATURE_LEN_RANGE)
        PKCS11_ERR_MSG(CKR_TEMPLATE_INCOMPLETE)
        PKCS11_ERR_MSG(CKR_TEMPLATE_INCONSISTENT)
        PKCS11_ERR_MSG(CKR_TOKEN_NOT_PRESENT)
        PKCS11_ERR_MSG(CKR_TOKEN_NOT_RECOGNIZED)
        PKCS11_ERR_MSG(CKR_TOKEN_WRITE_PROTECTED)
        PKCS11_ERR_MSG(CKR_UNWRAPPING_KEY_HANDLE_INVALID)
        PKCS11_ERR_MSG(CKR_UNWRAPPING_KEY_SIZE_RANGE)
        PKCS11_ERR_MSG(CKR_UNWRAPPING_KEY_TYPE_INCONSISTENT)
        PKCS11_ERR_MSG(CKR_USER_ALREADY_LOGGED_IN)
        PKCS11_ERR_MSG(CKR_USER_NOT_LOGGED_IN)
        PKCS11_ERR_MSG(CKR_USER_PIN_NOT_INITIALIZED)
        PKCS11_ERR_MSG(CKR_USER_TYPE_INVALID)
        PKCS11_ERR_MSG(CKR_USER_ANOTHER_ALREADY_LOGGED_IN)
        PKCS11_ERR_MSG(CKR_USER_TOO_MANY_TYPES)
        PKCS11_ERR_MSG(CKR_WRAPPED_KEY_INVALID)
        PKCS11_ERR_MSG(CKR_WRAPPED_KEY_LEN_RANGE)
        PKCS11_ERR_MSG(CKR_WRAPPING_KEY_HANDLE_INVALID)
        PKCS11_ERR_MSG(CKR_WRAPPING_KEY_SIZE_RANGE)
        PKCS11_ERR_MSG(CKR_WRAPPING_KEY_TYPE_INCONSISTENT)
        PKCS11_ERR_MSG(CKR_RANDOM_SEED_NOT_SUPPORTED)
        PKCS11_ERR_MSG(CKR_RANDOM_NO_RNG)
        PKCS11_ERR_MSG(CKR_DOMAIN_PARAMS_INVALID)
        PKCS11_ERR_MSG(CKR_BUFFER_TOO_SMALL)
        PKCS11_ERR_MSG(CKR_SAVED_STATE_INVALID)
        PKCS11_ERR_MSG(CKR_INFORMATION_SENSITIVE)
        PKCS11_ERR_MSG(CKR_STATE_UNSAVEABLE)
        PKCS11_ERR_MSG(CKR_CRYPTOKI_NOT_INITIALIZED)
        PKCS11_ERR_MSG(CKR_CRYPTOKI_ALREADY_INITIALIZED)
        PKCS11_ERR_MSG(CKR_MUTEX_BAD)
        PKCS11_ERR_MSG(CKR_MUTEX_NOT_LOCKED)
        PKCS11_ERR_MSG(CKR_VENDOR_DEFINED)
        default:
          return "not PKCS#11 ERROR";
    }
    return "";
}

static void *
PKCS11LoadModule(const char* pkcs11, CK_FUNCTION_LIST_PTR_PTR f)
{
    int rv;
    void *dl_handle;
    CK_RV (*c_get_function_list)(CK_FUNCTION_LIST_PTR_PTR) = NULL;
    CK_FUNCTION_LIST_PTR ff;
    char *error;
    dl_handle = dlopen(pkcs11, RTLD_LAZY);
    if (!dl_handle){
        Warning("cannot open PKCS#11 library : %s\n", dlerror());
        return NULL;
    }
    c_get_function_list = 
        (CK_RV (*)(CK_FUNCTION_LIST_PTR_PTR))dlsym(dl_handle,"C_GetFunctionList");

    if ((error = dlerror()) != NULL ){
        Warning("cannot get C_GetFunctionList address : %s\n", error);
        dlclose(dl_handle);
        return NULL;
    }
    if (c_get_function_list){
        rv = c_get_function_list(f);
        if (rv != CKR_OK){
            Warning("C_GetFunctionList : %s\n", PKCS11ErrorString(rv));
            dlclose(dl_handle);
            return NULL;
        }
    }

    ff = *(f);
    rv = ff->C_Initialize(NULL);
    if (rv != CKR_OK){
        Warning("C_Initialize : %s\n", PKCS11ErrorString(rv));
        dlclose(dl_handle);
        return NULL;
    }
    
    return dl_handle;
}

static Bool
PKCS11UnloadModule(void *dl_handle, CK_FUNCTION_LIST_PTR f)
{
    int rv;
    rv = f->C_Finalize(NULL);
    if (rv != CKR_OK){
        Warning("C_Finalize : %s\n", PKCS11ErrorString(rv));
    }
    dlclose(dl_handle);
    dl_handle = NULL;
    return TRUE;
}

static Bool
PKCS11FindPrivateKey(CK_SESSION_HANDLE session, 
    CK_FUNCTION_LIST_PTR f,
    char *keyid,
    int keyid_size)
{
    CK_RV rv;
    CK_OBJECT_CLASS key_object_class = CKO_PRIVATE_KEY;
    CK_OBJECT_HANDLE key_handle[PKCS11_MAX_OBJECT_NUM];
    CK_ATTRIBUTE key_attr[2];
    CK_ULONG key_count;

    /* find key id by cert's modulus */
    key_attr[0].type = CKA_CLASS;
    key_attr[0].pValue = &key_object_class;
    key_attr[0].ulValueLen = sizeof(CK_OBJECT_CLASS);
    key_attr[1].type = CKA_ID;
    key_attr[1].pValue = keyid;
    key_attr[1].ulValueLen = keyid_size;
    rv = f->C_FindObjectsInit(session, key_attr, 2);
    if ( rv != CKR_OK ){
        Warning("C_FindObjectsInit : %s\n", PKCS11ErrorString(rv));
        return FALSE;
    }
    rv = f->C_FindObjects(session, key_handle, 1, &key_count); 
    if ( rv != CKR_OK ){
        Warning("C_FindObjects : %s\n", PKCS11ErrorString(rv));
        return FALSE;
    }
    rv = f->C_FindObjectsFinal(session);
    if ( rv != CKR_OK ){
        Warning("C_FindObjects : %s\n", PKCS11ErrorString(rv));
        return FALSE;
    }
    if ( key_count <= 0){
        Warning("cannot find private key ID : %s\n", keyid);
        return FALSE;
    }
    return TRUE;
}

static Bool
PKCS11GetCertificate(CK_SLOT_ID slot, 
    CK_FUNCTION_LIST_PTR f, 
    char *pin,
    char **certder,
    int *certder_size,
    char **keyid,
    int *keyid_size)
{
    int i;
    CK_RV rv;
    CK_SESSION_HANDLE session;

    /* cert object */
    CK_OBJECT_CLASS cert_object_class = CKO_CERTIFICATE;
    CK_ATTRIBUTE cert_attr[2];
    CK_OBJECT_HANDLE cert_handle[PKCS11_MAX_OBJECT_NUM];
    CK_ULONG cert_count;

    rv = f->C_OpenSession(slot, CKF_SERIAL_SESSION, NULL, NULL, &session);
    if (rv != CKR_OK){
        Warning("C_OpenSession : %s\n", PKCS11ErrorString(rv));
        return FALSE;
    }
    rv = f->C_Login(session, CKU_USER, pin, strlen(pin));
    if (rv != CKR_OK){
        Warning("C_Login : %s\n", PKCS11ErrorString(rv));
        rv = f->C_CloseSession(session);
        return FALSE;
    }

    /* find cert */
    cert_attr[0].type = CKA_CLASS;
    cert_attr[0].pValue = &cert_object_class;
    cert_attr[0].ulValueLen = sizeof(CK_OBJECT_CLASS);
    rv = f->C_FindObjectsInit(session, cert_attr, 1);
    if ( rv != CKR_OK ){
        Warning("C_FindObjectsInit : %s\n", PKCS11ErrorString(rv));
        goto PKCS11GetCertificateFailure;
    }
    rv = f->C_FindObjects(session, cert_handle, PKCS11_MAX_OBJECT_NUM, &cert_count); 
    if ( rv != CKR_OK ){
        Warning("C_FindObjects : %s\n", PKCS11ErrorString(rv));
        goto PKCS11GetCertificateFailure;
    }
    rv = f->C_FindObjectsFinal(session);
    if ( rv != CKR_OK ){
        Warning("C_FindObjectsFinal : %s\n", PKCS11ErrorString(rv));
        goto PKCS11GetCertificateFailure;
    }


    for (i = 0; i < cert_count; i++){
        /* get certificate */
        *certder = NULL;
        cert_attr[0].type = CKA_VALUE;
        cert_attr[0].pValue = NULL;
        cert_attr[0].ulValueLen = 0;
        cert_attr[1].type = CKA_ID;
        cert_attr[1].pValue = NULL;
        cert_attr[1].ulValueLen = 0;
        rv = f->C_GetAttributeValue(session, cert_handle[i], cert_attr, 2);
        if ( rv != CKR_OK ){
            Warning("C_GetAttributeValue : %s\n", PKCS11ErrorString(rv));
            goto PKCS11GetCertificateFailure;
        }
        *certder_size = cert_attr[0].ulValueLen;
        *keyid_size = cert_attr[1].ulValueLen;
        if ((*certder = xmalloc(*certder_size)) == NULL){
            Warning("xmalloc failure\n");
            goto PKCS11GetCertificateFailure;
        }
        if ((*keyid = xmalloc(*keyid_size)) == NULL){
            Warning("xmalloc failure\n");
            goto PKCS11GetCertificateFailure;
        }
        cert_attr[0].pValue = *certder;
        cert_attr[1].pValue = *keyid;
        rv = f->C_GetAttributeValue(session, cert_handle[i], cert_attr, 2);
        if ( rv != CKR_OK ){
            Warning("C_FindObjectsFinal : %s\n", PKCS11ErrorString(rv));
            goto PKCS11GetCertificateFailure;
        }
        if (PKCS11FindPrivateKey(session, f, *keyid, *keyid_size)){
            return TRUE;
        }
        
        free(*certder);
    }

PKCS11GetCertificateFailure:
    rv = f->C_Logout(session);
    rv = f->C_CloseSession(session);
    return FALSE;
}  

static CK_ULONG
PKCS11GetSlotList(CK_FUNCTION_LIST_PTR f, CK_SLOT_ID_PTR list)
{
    CK_RV rv;
    CK_ULONG count;

    rv = f->C_GetSlotList(TRUE, NULL, &count);
    if ( rv != CKR_OK ){
        Warning("C_GetSlotList : %s\n", PKCS11ErrorString(rv));
        return 0;
    }
    if (count > PKCS11_MAX_SLOT_NUM) count = PKCS11_MAX_SLOT_NUM;
    rv = f->C_GetSlotList(TRUE, list, &count);
    if ( rv != CKR_OK ){
        Warning("C_GetSlotList : %s\n", PKCS11ErrorString(rv));
        return 0;
    }
    return count;
}

/*
 * OpenSSL ENGINE + engine_pkcs11.so
 */

static ENGINE *
InitEnginePKCS11( const char *pkcs11, const char *pin)
{
    ENGINE *e;
    const char *message;
    ENGINE_load_dynamic();
    e = ENGINE_by_id("dynamic");
    if (!e){
        message = "Engine_by_id failure\n";
        Warning("%s: %s", message, GetSSLErrorString());
        return NULL;
    }

    if(!ENGINE_ctrl_cmd_string(e, "SO_PATH", ENGINE_PKCS11_PATH, 0)||
       !ENGINE_ctrl_cmd_string(e, "ID", "pkcs11", 0) ||
       !ENGINE_ctrl_cmd_string(e, "LIST_ADD", "1", 0) ||
       !ENGINE_ctrl_cmd_string(e, "LOAD", NULL, 0) ||
       !ENGINE_ctrl_cmd_string(e, "MODULE_PATH", pkcs11, 0) ||
       !ENGINE_ctrl_cmd_string(e, "PIN", pin, 0) ){
        message = "Engine_ctrl_cmd_string failure\n";
        Warning("%s: %s", message, GetSSLErrorString());
        ENGINE_free(e);
        return NULL;
    }

    if(!ENGINE_init(e)){
        message = "Engine_init failure\n";
        Warning("%s: %s", message, GetSSLErrorString());
        ENGINE_free(e);
        return NULL;
    }

    return e; 
}

#define ASKPIN_ENV "PANDA_ASKPASS"
#define ASKPIN_PROMPT "Please input security device PIN:"

static char *
GetPinString(char *buf, size_t sz)
{
    const char *cmd;

    if ((cmd = getenv(ASKPIN_ENV)) != NULL){
        if (askpass(cmd, ASKPIN_PROMPT, buf, sz) >= 0){
            return buf;
        }
    }
    else if (isatty(STDIN_FILENO)){
        if (EVP_read_pw_string(buf, sz, ASKPIN_PROMPT, 0) == 0){
            return buf;
        }
    }

    return NULL;
}

static Bool
LoadEnginePKCS11(SSL_CTX *ctx, ENGINE **e, const char *pkcs11, const char *slotstr)
{
    char pinbuf[PKCS11_BUF_SIZE];
    char *pin = NULL;
    const char *message;
    EVP_PKEY *key = NULL;
    X509 *cert = NULL;
    int i;

    void *dl_handle = NULL;
    Bool flag = FALSE;
    char *certder;
    unsigned char *derptr;
    int certder_size;
    char *keyidbuf;
    int keyidbuf_size;
    char keyid[PKCS11_BUF_SIZE];
    char buf[4];
    CK_SLOT_ID slot;
    CK_SLOT_ID slot_list[PKCS11_MAX_SLOT_NUM];
    CK_ULONG slot_count = 0;
    CK_FUNCTION_LIST_PTR p11funcs = NULL;

    if (!(dl_handle = PKCS11LoadModule((const char*)pkcs11, &p11funcs))) return FALSE;
    if ((pin = GetPinString(pinbuf, sizeof(pinbuf))) == NULL){
        Warning("cannot read pin\n");
        return FALSE;
    }
    
    /* get certificate and keyid by PKCS#11 */
    if (slotstr != NULL){
        slot = atol(slotstr);
        flag = PKCS11GetCertificate(slot,
            p11funcs,
            pin,
            &certder,
            &certder_size,
            &keyidbuf,
            &keyidbuf_size);
    }
    else {
        /* try all available slot */
        slot_count = PKCS11GetSlotList(p11funcs, slot_list);
        for (i = 0; i < slot_count; i++){
            flag = PKCS11GetCertificate(slot_list[i],
                    p11funcs,
                    pin,
                    &certder,
                    &certder_size,
                    &keyidbuf,
                    &keyidbuf_size);
            if (flag) break;
        }
    }
    PKCS11UnloadModule(dl_handle, p11funcs);
    if (!flag) return FALSE;
    
    /* change engine_pkcs11 id format 'id_XXXX' XXXX is keyids hexdump */
    if (keyidbuf_size > PKCS11_BUF_SIZE - strlen("id_")){
        Warning("keyidbuf size over");
        free(keyidbuf);
        return FALSE;
    }
    sprintf(keyid, "id_");
    for (i = 0; i < keyidbuf_size; i++){
        sprintf(buf, "%02x", keyidbuf[i]);
        strcat(keyid, buf);
    }
    free(keyidbuf);

    derptr = (unsigned char*)certder;
    cert = d2i_X509(NULL , &derptr ,certder_size);
    if (cert == NULL) {
        message = "d2i_X509 failure: %s\n";
        Warning("%s: %s", message, GetSSLErrorString());
        free(certder);
        return FALSE;
    }
    
    /* setup OpenSSL ENGINE */
    if (!(*e = InitEnginePKCS11(pkcs11, pin))){
        return FALSE;
    } 
    if(!(key = ENGINE_load_private_key(*e, keyid, NULL, NULL))) {
            message = "SSL_CTX_use_certificate failure: %s\n";
            Warning("%s: %s", message, GetSSLErrorString());
            return FALSE;
    }

    /* set key and cert to SSL_CTX */
    if (cert && key){
        if (!SSL_CTX_use_certificate(ctx, cert)){
            message = "SSL_CTX_use_certificate failure: %s\n";
            Warning("%s: %s", message, GetSSLErrorString());
            return FALSE;
        }
        if (!SSL_CTX_use_PrivateKey(ctx, key)){
            message = "SSL_CTX_use_PrivateKey failure: %s\n";
            Warning("%s: %s",message, GetSSLErrorString());
            return FALSE;
        }
        if (!SSL_CTX_check_private_key(ctx)){
            Warning("SSL_CTX_check_private_key failure: %s\n",
                    GetSSLErrorString());
            return FALSE;
        }
    }
    
    memset(pinbuf, 0, sizeof(pinbuf));
    return TRUE;
}

extern	SSL_CTX	*
MakeSSL_CTX_PKCS11(
    ENGINE  **e,
	char	*pkcs11,
	char	*slotstr,
	char	*cafile,
	char	*capath,
	char	*ciphers)
{
    SSL_CTX *ctx;
    int     mode = SSL_VERIFY_NONE;
    const char *askpass_command;

    if ((ctx = SSL_CTX_new(SSLv23_method())) == NULL){
        Warning("SSL_CTX_new failue: %s\n", GetSSLErrorString());
        return NULL;
    }

    if ((askpass_command = getenv(ASKPASS_ENV)) != NULL){
        SSL_CTX_set_default_passwd_cb(ctx, passphrase_callback);
        SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)askpass_command);
    }

    if (!SSL_CTX_set_cipher_list(ctx, ciphers)){
        Warning("SSL_CTX_set_cipher_list(%s) failue: %s\n",
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
            Warning("SSL_CTX_set_default_verify_paths error: %s\n",
                    GetSSLErrorString());
        }
    }
    else if (!SSL_CTX_load_verify_locations(ctx, cafile, capath)){
        Warning("SSL_CTX_load_verify_locations(%s, %s) error: %s\n",
                cafile, capath, GetSSLErrorString());
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (pkcs11 != NULL)
        if (LoadEnginePKCS11(ctx, e, pkcs11, slotstr)) return ctx;
    return NULL;
}
#endif /* USE_PKCS11 */

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
