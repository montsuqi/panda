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

/*
#define	DEBUG
#define	TRACE
*/

#define	MT_NET

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/socket.h>
#include	<sys/wait.h>
#include	<time.h>

#include	"socket.h"
#include	"net.h"
#include	<libmondai.h>
#include	"port.h"
#include	"debug.h"
#include	"gettext.h"

#ifdef	MT_NET
#define	LockNet(fp)		{											\
	dbgmsg("lock");													\
	pthread_mutex_lock(&(fp)->lock);								\
}
#define	UnLockNet(fp)	{				\
	dbgmsg("unlock");					\
	pthread_mutex_unlock(&(fp)->lock);	\
}
#else
#define	LockNet(fp)
#define	UnLockNet(fp)
#endif

#ifdef  USE_SSL
#define	SSL_MESSAGE_SIZE			1024
static	char	ssl_error_message[SSL_MESSAGE_SIZE];
static	char	ssl_warning_message[SSL_MESSAGE_SIZE];
static	int		(* AskPassFunction_)(char *buf, size_t buflen, const char *prompt) = NULL;
#ifdef  USE_PKCS11
static	ENGINE		*InitEnginePKCS11( const char *pkcs11, const char *pin);
static	Bool		LoadEnginePKCS11(SSL_CTX *ctx, ENGINE **e, const char *pkcs11, const char *slotstr);
#endif	//	USE_PKCS11
#endif	//	USE_SSL

static	Bool
_Flush(
	NETFILE	*fp)
{
	unsigned char	*p = fp->buff;
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

	if		(	fp  !=  NULL  ) {
		LockNet(fp);
		if	(	!fp->fOK	) {
			UnLockNet(fp);
			return -1;
		}
		ret = size;
		if		(	(  fp->buff  !=  NULL  )
				&&	(  fp->size  >  size  ) ) {
			while	(  size + fp->ptr  >  fp->size  ) {
				left = fp->size - fp->ptr;
				memcpy((fp->buff + fp->ptr),p,left);
				fp->ptr = fp->size;
				if		(  !_Flush(fp)  )	{
					ret = -1;
					break;
				}
				p += left;
				size -= left;
			}
			if		(  ret  !=  -1  ) {
				memcpy((fp->buff + fp->ptr),p,size);
				fp->ptr += size;
				fp->fSent = TRUE;
			}
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
	return	(ret);
}

extern	int
RecvAtOnce(
	NETFILE	*fp,
	char	*buff,
	size_t	size)
{
	return	fp->read(fp,buff,size);
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

	if		(	fp  !=  NULL  ) {
		LockNet(fp);
		if	(	!fp->fOK	) {
			UnLockNet(fp);
			return -1;
		}
		if		(  fp->fSent  ) {
			_Flush(fp);
		}
		ret = size;
		while	(  size  >  0  ) {
			if		(  ( count = RecvAtOnce(fp,p,size) )  >  0  ) {
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
	int		ret;

	if		(  Recv(fp,&ch,1)  >=  0  ) {
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
#endif
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
		fp->fOK = TRUE;
	}
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
			Warning(_d("read error:\n %s\n"),strerror(errno));
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
			Warning(_d("write error:\n %s\n"),strerror(errno));
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
	memclear(fp->buff, SIZE_BUFF);
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

static void
SSL_Error(char *format, ...)
{
	char buff[SSL_MESSAGE_SIZE];
	va_list va;

	va_start(va, format);
	vsnprintf(buff, sizeof(buff), format, va);
	va_end(va);
	strncat(ssl_error_message, buff, SSL_MESSAGE_SIZE - 1);
}

extern char*
GetSSLErrorMessage(void)
{
	return ssl_error_message;
}

static void
SSL_Warning(char *format, ...)
{
	char buff[SSL_MESSAGE_SIZE];
	va_list va;

	va_start(va, format);
	vsnprintf(buff, sizeof(buff), format, va);
	va_end(va);
	strncat(ssl_warning_message, buff, SSL_MESSAGE_SIZE - 1);
}

extern char*
GetSSLWarningMessage(void)
{
	return ssl_warning_message;
}

#define ASKPASS_PROMPT		  _("Please input key passphrase:")
#define ASKPASS_PROMPT_RETRY	_("Verify ERROR!\nPlease input key passphrase:")

extern void
SetAskPassFunction(
	int (*func)(char *buf, size_t sz, const char *prompt))
{
	AskPassFunction_ = func;
}

static int
AskPassFunction(char *buf, size_t sz, const char *prompt)
{
	if ( AskPassFunction_ != NULL ) {
		return AskPassFunction_(buf, sz, prompt);
	} else if (isatty(STDIN_FILENO)){
		if (EVP_read_pw_string(buf, sz, prompt, 0) == 0) {
			return strlen(buf);
		}
	}
	return -1;
}

static char *
GetPasswordString(char *buf, size_t sz, const char *prompt)
{
	if ( AskPassFunction(buf, sz, prompt) != -1) {
		return buf;
	}
	return NULL;
}

static int
passphrase_callback(char *buf, int buflen, int flag, void *userdata)
{
	return AskPassFunction(buf, (size_t)buflen, (const char*)userdata);
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
			SSL_Error(_d("SSL read error:\n %s\n"),ERR_error_string(err, NULL));
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
			SSL_Error(_d("SSL write error:\n %s\n"),ERR_error_string(err, NULL));
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

#if 0
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
		SSL_Error(_d("BIO_new failure:\n %s\n"), GetSSLErrorString());
		goto err;
	}
	if (!X509_NAME_print_ex(out, subject, 0, flags)){
		SSL_Error(_d("X509_NAME_print_ex failure:\n %s\n"),GetSSLErrorString());
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
#endif

static char*
GetSubjectFromCertificate_X509_NAME_online(X509 *cert)
{
	X509_NAME *subject;
	char *ret = NULL;

	if ((subject = X509_get_subject_name(cert)) == NULL){
		return NULL;
	}
	if ((ret = xmalloc(1000)) == NULL){
		return NULL;
	}
	if (!X509_NAME_oneline(subject, ret, 1000)){
		SSL_Error(_d("X509_NAME_oneline failure:\n %s\n"),GetSSLErrorString());
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
	const X509V3_EXT_METHOD *meth;
	unsigned char *data;
	int len;
	int ok = FALSE;
	int i;

	data = ext->value->data;
	if ((meth = X509V3_EXT_get(ext)) == NULL) return FALSE;
	data = ext->value->data;
	len = ext->value->length;
	values = meth->i2v(meth, meth->d2i(NULL, (const unsigned char **)&data, len), NULL);
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
		SSL_Error(_d("SSL_connect failure:\n %s\n"), GetSSLErrorString());
		return FALSE;
	}
	if ((cert = SSL_get_peer_certificate(fp->net.ssl)) != NULL){
		fp->peer_cert = cert;
		id_ok = CheckHostnameInCertificate(cert, hostname);
		if (id_ok != TRUE){
			SSL_Error(_d("The hostname does not match %s.\n"), hostname);
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
		SSL_Error(_d("SSL_accept failure:\n %s\n"), GetSSLErrorString());
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
		SSL_Error(_d("SSL_new failure:\n %s\n"), GetSSLErrorString());
		FreeNet(fp);
		return NULL;
	}
	if (!SSL_set_fd(fp->net.ssl, fd)){
		SSL_Error(_d("SSL_set_fd failure:\n %s\n"), GetSSLErrorString());
		SSL_free(fp->net.ssl);
		FreeNet(fp);
		return NULL;
	}

	return fp;
}


static	time_t
asn1_time_to_time(
	ASN1_TIME *tm)
{
	struct tm rtm;
	char work[3];
	time_t rt;
	extern time_t timezone;

	memset(work, '\0', sizeof(work));
	memset(&rtm, 0, sizeof(struct tm));

	memcpy(work, tm->data + 10, 2);
	rtm.tm_sec = atoi(work);
	memcpy(work, tm->data + 8, 2);
	rtm.tm_min = atoi(work);
	memcpy(work, tm->data + 6, 2);
	rtm.tm_hour = atoi(work);
	memcpy(work, tm->data + 4, 2);
	rtm.tm_mday = atoi(work);
	memcpy(work, tm->data + 2, 2);
	rtm.tm_mon = atoi(work) - 1;
	memcpy(work, tm->data, 2);
	rtm.tm_year = atoi(work);
	if (rtm.tm_year < 70)
	rtm.tm_year += 100;

	timezone = 0;
	rt = mktime(&rtm);
	tzset();

	return rt;
}

#define WARNING_BEFORE_EXPIRATION_PERIOD (31 * 24 * 3600) // 31 days
static 	void
CheckValidPeriod(
	X509 *cert)
{
	char buf[256];
	long valid;
	valid = (long)difftime(asn1_time_to_time(X509_get_notAfter(cert)), time(NULL));

	if(valid < WARNING_BEFORE_EXPIRATION_PERIOD){
		X509_NAME_oneline(X509_get_subject_name(cert),buf,sizeof buf);
		SSL_Warning(_d("The certificate(%s) will be expired in %d days.\nPlease update the certificate.\n"), buf, valid / (24 * 3600));
	}
}

static	int
_VerifyCallBack(
	int		ok,
	X509_STORE_CTX	*ctx)
{
	char buf[256];
	X509 *err_cert;
	BIO	*bio_err;
	char *ptr = NULL;
	char printable[256] = {'\0'};
	int length;

	bio_err = BIO_new(BIO_s_mem());
	err_cert = X509_STORE_CTX_get_current_cert(ctx);
	X509_STORE_CTX_get_error(ctx);

	X509_NAME_oneline(X509_get_subject_name(err_cert),buf,sizeof buf);
	switch (ctx->error) {
	  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
	  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
		X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert),buf,sizeof buf);
		SSL_Error(_d(" Unable to get issuer cert.\n issuer= %s\n"), buf);
		break;
	  case X509_V_ERR_CERT_NOT_YET_VALID:
	  case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
		SSL_Error(_d(" Error in cert not_before field.\n notBefore="));
		ASN1_TIME_print(bio_err,X509_get_notBefore(ctx->current_cert));
				length = BIO_get_mem_data(bio_err, &ptr);
				memcpy(printable, ptr, length - 1);
		SSL_Error("%s\n", printable);
		break;
	  case X509_V_ERR_CERT_HAS_EXPIRED:
	  case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
		SSL_Error(_d(" Error in cert not_after field.\n notAfter="));
		ASN1_TIME_print(bio_err,X509_get_notAfter(ctx->current_cert));
				length = BIO_get_mem_data(bio_err, &ptr);
				memcpy(printable, ptr, length - 1);
		SSL_Error("%s\n", printable);
		break;
	}
	BIO_free(bio_err);
	return(ok);
}

static	int
LocalVerifyCallBack(
	int		ok,
	X509_STORE_CTX	*ctx)
{
	char buf[256];
	X509 *err_cert;
	int err, depth;

	err_cert = X509_STORE_CTX_get_current_cert(ctx);
	err = X509_STORE_CTX_get_error(ctx);
	depth = X509_STORE_CTX_get_error_depth(ctx);

	X509_NAME_oneline(X509_get_subject_name(err_cert),buf,sizeof buf);
	if	(!ok) {
		SSL_Error(_d("Verification error of local entity certificate:\n depth=%d\n %s:%s\n"),
				  depth, X509_verify_cert_error_string(err),buf);
	}
	_VerifyCallBack(ok, ctx);
	return(ok);
}

static	int
RemoteVerifyCallBack(
	int		ok,
	X509_STORE_CTX	*ctx)
{
	char buf[256];
	X509 *err_cert;
	int err, depth;

	err_cert = X509_STORE_CTX_get_current_cert(ctx);
	err = X509_STORE_CTX_get_error(ctx);
	depth = X509_STORE_CTX_get_error_depth(ctx);

	X509_NAME_oneline(X509_get_subject_name(err_cert),buf,sizeof buf);
	if	(!ok) {
		SSL_Error(_d("Verification error of remote entity certificate:\n depth=%d\n %s:%s\n"),
				  depth, X509_verify_cert_error_string(err),buf);
	}
	_VerifyCallBack(ok, ctx);
	return(ok);
}


static int
SSL_CTX_use_certificate_with_check(
	SSL_CTX *ctx,
	X509 *x509)
{
	int ret;
	X509_STORE_CTX *sctx;
	ret = SSL_CTX_use_certificate(ctx, x509);
	if(!ret)return ret;
	X509_STORE_add_cert(ctx->cert_store, x509);
	sctx = X509_STORE_CTX_new();
	X509_STORE_CTX_init(sctx, ctx->cert_store, x509, NULL);
	X509_STORE_CTX_set_verify_cb(sctx, LocalVerifyCallBack);
	X509_verify_cert(sctx);
	X509_STORE_CTX_free(sctx);
	CheckValidPeriod(x509);
	return ret;
}

static int
SSL_CTX_use_certificate_file_with_check(
	SSL_CTX *ctx,
	char *file,
	int type)
{
	FILE *fp;
	X509 *x509;
	X509_STORE_CTX *sctx;
	int ret;
	ret = SSL_CTX_use_certificate_file(ctx, file, type);
	if(!ret) return ret;
	if(!(fp = fopen(file, "r"))) {
		return -1;
	}
	x509 = PEM_read_X509(fp, NULL, NULL, NULL);
	if(!x509){
		rewind(fp);
		x509 = d2i_X509_fp(fp, NULL);
	}
	fclose(fp);
	if(!x509) return -1;
	X509_STORE_add_cert(ctx->cert_store, x509);
	sctx = X509_STORE_CTX_new();
	X509_STORE_CTX_init(sctx, ctx->cert_store, x509, NULL);
	X509_STORE_CTX_set_verify_cb(sctx, LocalVerifyCallBack);
	X509_verify_cert(sctx);
	X509_STORE_CTX_free(sctx);
	CheckValidPeriod(x509);
	return ret;
}

static Bool
IsPKCS12(const char *file)
{
	Bool ret = TRUE;
	EVP_PKEY *key = NULL;
	X509 *cert = NULL;
	BIO *input;
	PKCS12 *p12;
	int err_reason;

	if ((input = BIO_new_file(file, "r")) == NULL){
		if (d2i_PKCS12_bio(input, &p12) == NULL) return FALSE;
	}
	p12 = d2i_PKCS12_bio(input, NULL);
	BIO_free(input);
	if (p12 == NULL) return FALSE;

	err_reason = PKCS12_parse(p12, "", &key, &cert, NULL);
	if (err_reason == PKCS12_R_MAC_VERIFY_FAILURE){
		ret = FALSE;
	}
	if (cert){ X509_free(cert); cert = NULL; }
	if (key){ EVP_PKEY_free(key); key = NULL; }
	ERR_clear_error();
	PKCS12_free(p12);

	return ret;
}

static Bool
LoadPKCS12(SSL_CTX *ctx, const char *file)
{
	char passbuf[256];
	char *pass = NULL;
	PKCS12 *p12;
	EVP_PKEY *key = NULL;
	X509 *cert = NULL;
	BIO *input;
	int err_reason;
	int count = 0;
	const char *prompt = ASKPASS_PROMPT;

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
		Message("PKCS12_parse failure: %s", GetSSLErrorString());
		break;
	}
	ERR_clear_error();
	if (count >= 1) prompt = ASKPASS_PROMPT_RETRY;
	if ((pass = GetPasswordString(passbuf, sizeof(passbuf), prompt)) == NULL){
		Message("PASSWORD input was canceled\n");
		break;
	}
	count++;
	}
	//OPENSSL_cleanse(passbuf, sizeof(passbuf));
	memset(passbuf, 0, sizeof(passbuf));
	PKCS12_free(p12);

	/* set key and cert to SSL_CTX */
	if (cert && key){
		if (!SSL_CTX_use_certificate_with_check(ctx, cert)){
			SSL_Error(_d("SSL_CTX_use_certificate failure:\n %s"), GetSSLErrorString());
			return FALSE;
		}
		if (!SSL_CTX_use_PrivateKey(ctx, key)){
			SSL_Error(_d("SSL_CTX_use_PrivateKey failure:\n %s"), GetSSLErrorString());
			return FALSE;
		}
		if (!SSL_CTX_check_private_key(ctx)){
			SSL_Error(_d("SSL_CTX_check_private_key failure:\n %s\n"),
				GetSSLErrorString());
			return FALSE;
		}
	}
	else{
		return FALSE;
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
	SSL_CTX *ctx = NULL;
	int	 mode = SSL_VERIFY_NONE;

	if ((ctx = SSL_CTX_new(SSLv23_method())) == NULL){
		SSL_Error(_d("SSL_CTX_new failure:\n %s\n"), GetSSLErrorString());
		return NULL;
	}

	SSL_CTX_set_default_passwd_cb(ctx, passphrase_callback);
	SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)ASKPASS_PROMPT);

	if (!SSL_CTX_set_cipher_list(ctx, ciphers)){
		SSL_Error(_d("SSL_CTX_set_cipher_list(%s) failure:\n %s\n"),
				ciphers, GetSSLErrorString());
		SSL_CTX_free(ctx);
		return NULL;
	}

	mode = SSL_VERIFY_PEER;
	mode |= SSL_VERIFY_CLIENT_ONCE;
	SSL_CTX_set_verify(ctx, mode, RemoteVerifyCallBack);
	SSL_CTX_set_options(ctx, SSL_OP_ALL);

	if ((cafile == NULL) && (capath == NULL)){
		if (!SSL_CTX_set_default_verify_paths(ctx)){
			SSL_Error(_d("SSL_CTX_set_default_verify_paths error:\n %s\n"),
					GetSSLErrorString());
		}
	}
	else if (!SSL_CTX_load_verify_locations(ctx, cafile, capath)){
		if (cafile == NULL) cafile = capath;
		SSL_Error(_d("SSL_CTX_load_verify_locations(%s)\n"), cafile);
		SSL_CTX_free(ctx);
		return NULL;
	}

	if (cert != NULL){
		if (IsPKCS12(cert)){
			if (LoadPKCS12(ctx, cert)){
				return ctx;
			}
			else {
				SSL_CTX_free(ctx);
				return NULL;
			}
		}
		else {
			if (SSL_CTX_use_certificate_file_with_check(ctx, cert, SSL_FILETYPE_PEM) <= 0){
				SSL_Error(_d("SSL_CTX_use_certificate_file(%s) failure:\n %s\n"),
						cert, GetSSLErrorString());
				SSL_CTX_free(ctx);
				return NULL;
			}
			if (key == NULL) key = cert;
			for (;;){
				if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0){
					int err_reason;
					err_reason = ERR_GET_REASON(ERR_peek_error());
					SSL_Error(_d("SSL_CTX_use_PrivateKey_file(%s) failure:\n %s\n"),
							key, GetSSLErrorString());
					if (err_reason == PEM_R_BAD_DECRYPT ||
						err_reason == EVP_R_BAD_DECRYPT) continue;
					SSL_CTX_free(ctx);
					return NULL;
				}
				break;
			}
			if (!SSL_CTX_check_private_key(ctx)){
				SSL_Error(_d("SSL_CTX_check_private_key failure:\n %s\n"),
						GetSSLErrorString());
				SSL_CTX_free(ctx);
				return NULL;
			}
		}
	} else {
		SSL_Error(_d("Please specify certificate file"));
		return NULL;
	}

	return ctx;
}

/*
 * SECURITY_DEVICE
 */
#ifdef USE_PKCS11

/*
 * OpenSSL ENGINE + engine_pkcs11.so
 */

static ENGINE *
InitEnginePKCS11( const char *pkcs11, const char *pin)
{
	ENGINE *e;
	ENGINE_load_dynamic();
	e = ENGINE_by_id("dynamic");
	if (!e){
		SSL_Error(_d("Engine_by_id:\n %s"), GetSSLErrorString());
		return NULL;
	}

	if(!ENGINE_ctrl_cmd_string(e, "SO_PATH", ENGINE_PKCS11_PATH, 0)||
	   !ENGINE_ctrl_cmd_string(e, "ID", "pkcs11", 0) ||
	   !ENGINE_ctrl_cmd_string(e, "LIST_ADD", "1", 0) ||
	   !ENGINE_ctrl_cmd_string(e, "LOAD", NULL, 0) ||
	   !ENGINE_ctrl_cmd_string(e, "MODULE_PATH", pkcs11, 0) ||
	   !ENGINE_ctrl_cmd_string(e, "PIN", pin, 0) ){
		SSL_Error(_d("Engine_ctrl_cmd_string failure:\n %s"), GetSSLErrorString());
		ENGINE_free(e);
		return NULL;
	}

	if(!ENGINE_init(e)){
		SSL_Error(_d("Engine_init failure:\n %s"), GetSSLErrorString());
		ENGINE_free(e);
		return NULL;
	}

	return e;
}

#define PKCS11_ASKPIN_PROMPT _("Please input security device PIN:")

static Bool
LoadEnginePKCS11(SSL_CTX *ctx, ENGINE **e, const char *p11lib, const char *slotstr)
{
	char certid[PKCS11_BUF_SIZE];
	char certidbuf[PKCS11_BUF_SIZE];
	char pinbuf[PKCS11_BUF_SIZE];
	char *pin = NULL;
	EVP_PKEY *key = NULL;
	X509 *x509 = NULL;

	int rc = 0;
	int i;
	unsigned int nslots,ncerts;
	int nslot = 0;

	PKCS11_CTX *p11ctx;
	PKCS11_SLOT *slots, *slot;
	PKCS11_CERT *certs,*cert;

	pin = GetPasswordString(pinbuf, sizeof(pinbuf), PKCS11_ASKPIN_PROMPT);
	if (pin == NULL){
		Message("PIN input was canceled\n");
		return FALSE;
	}

	p11ctx = PKCS11_CTX_new();

	/* load pkcs #11 module */
	rc = PKCS11_CTX_load(p11ctx, p11lib);
	if (rc) {
		SSL_Error("loading pkcs11 engine failed: %s\n",
			ERR_reason_error_string(ERR_get_error()));
		return FALSE;
	}

	/* get information on all slots */
	rc = PKCS11_enumerate_slots(p11ctx, &slots, &nslots);
	if (rc < 0) {
		SSL_Error("no slots available\n");
		return FALSE;
	}

	/* get certificate and keyid by PKCS#11 */
	if (strcmp("",slotstr)){
		nslot = atoi(slotstr);
		if (nslot < nslots) {
			slot = (PKCS11_SLOT*)&slots[nslot];
			if (!slot || !slot->token) {
				SSL_Error("no token available\n");
				return FALSE;
			}
		} else {
			SSL_Error("no token available\n");
			return FALSE;
		}
	}
	else {
		/* get first slot with a token */
		slot = PKCS11_find_token(p11ctx, slots, nslots);
		if (!slot || !slot->token) {
			SSL_Error("no token available\n");
			return FALSE;
		}
		for(i=0;i<nslots;i++) {
			if (&slots[i] == slot) {
				nslot = i;
			}
		}
	}

	printf("Slot manufacturer......: %s\n", slot->manufacturer);
	printf("Slot description.......: %s\n", slot->description);
	printf("Slot token label.......: %s\n", slot->token->label);
	printf("Slot token manufacturer: %s\n", slot->token->manufacturer);
	printf("Slot token model.......: %s\n", slot->token->model);
	printf("Slot token serialnr....: %s\n", slot->token->serialnr);

	/* perform pkcs #11 login */
	rc = PKCS11_login(slot, 0, pin);
	if (rc != 0) {
		SSL_Error("PKCS11_login failed\n");
		return FALSE;
	}

	/* get all certs */
	rc = PKCS11_enumerate_certs(slot->token, &certs, &ncerts);
	if (rc) {
		SSL_Error("PKCS11_enumerate_certs failed\n");
		return FALSE;
	}
	if (ncerts <= 0) {
		SSL_Error("no certificates found\n");
		return FALSE;
	}

	/* use the first cert */
	cert=(PKCS11_CERT*)&certs[0];


	sprintf(certid,"slot_%d-id_",nslot);
	for(i=0;i<cert->id_len;i++) {
		sprintf(certidbuf,"%02x",(unsigned int)(cert->id[i]));
		strcat(certid,certidbuf);
	}
	printf("id:[%s] label:%s [%p]\n",certid,cert->label,cert->x509);
	x509 = X509_dup(cert->x509);

	PKCS11_logout(slot);
	PKCS11_release_all_slots(p11ctx, slots, nslots);
	PKCS11_CTX_unload(p11ctx);
	PKCS11_CTX_free(p11ctx);

	/* setup OpenSSL ENGINE */
	if (!(*e = InitEnginePKCS11(p11lib, pin))){
		return FALSE;
	}
	if(!(key = ENGINE_load_private_key(*e, certid, NULL, NULL))) {
		SSL_Error(_d("ENGINE_load_private_key failure:\n %s\n"), GetSSLErrorString());
		return FALSE;
	}

	/* set key and cert to SSL_CTX */
	if (key){
		if (!SSL_CTX_use_certificate_with_check(ctx, x509)){
			SSL_Error(_d("SSL_CTX_use_certificate failure:\n %s"), GetSSLErrorString());
			return FALSE;
		}
		if (!SSL_CTX_use_PrivateKey(ctx, key)){
			SSL_Error(_d("SSL_CTX_use_PrivateKey failure:\n %s"), GetSSLErrorString());
			return FALSE;
		}
		if (!SSL_CTX_check_private_key(ctx)){
			SSL_Error(_d("SSL_CTX_check_private_key failure:\n %s\n"),
					GetSSLErrorString());
			return FALSE;
		}
	}
	memset(pin, 0, sizeof(pinbuf));
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
	int	 mode = SSL_VERIFY_NONE;

	if ((ctx = SSL_CTX_new(SSLv23_method())) == NULL){
		SSL_Error(_d("SSL_CTX_new failure:\n %s\n"), GetSSLErrorString());
		return NULL;
	}

	if (!SSL_CTX_set_cipher_list(ctx, ciphers)){
		SSL_Error(_d("SSL_CTX_set_cipher_list(%s) failure:\n %s\n"),
				ciphers, GetSSLErrorString());
		SSL_CTX_free(ctx);
		return NULL;
	}

	mode = SSL_VERIFY_PEER;
	mode |= SSL_VERIFY_CLIENT_ONCE;
	SSL_CTX_set_verify(ctx, mode, RemoteVerifyCallBack);
	SSL_CTX_set_options(ctx, SSL_OP_ALL);

	if ((cafile == NULL) && (capath == NULL)){
		if (!SSL_CTX_set_default_verify_paths(ctx)){
			SSL_Error(_d("SSL_CTX_set_default_verify_paths error:\n %s\n"),
					GetSSLErrorString());
		}
	}
	else if (!SSL_CTX_load_verify_locations(ctx, cafile, capath)){
		if (cafile == NULL) cafile = capath;
		SSL_Error(_d("SSL_CTX_load_verify_locations(%s)\n"), cafile);
		SSL_CTX_free(ctx);
		return NULL;
	}

	if (LoadEnginePKCS11(ctx, e, pkcs11, slotstr)) return ctx;
	return NULL;
}
#endif /* USE_PKCS11 */

#endif	/*	USE_SSL	*/

extern	void
InitNET(void)
{
	bindtextdomain(PACKAGE, LOCALEDIR);
#ifdef	USE_SSL
	ssl_error_message[0] = '\0';
	ssl_warning_message[0] = '\0';
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

extern	int
InitServerMultiPort(
	Port	*port,
	int	back,
	int	*soc)
{
	int soc_len;

	if ( port->type == PORT_UNIX ) {
		soc[0] = InitServerPort(port,back);
		soc_len = 1;
	} else {
#ifdef	USE_IPv6
		soc_len = BindIP_Multi_Listen(IP_PORT(port), back, soc);
#else
		soc[0] = InitServerPort(port,back);
		soc_len = 1;
#endif
	}
	return soc_len;
}

extern	int
AcceptLoop(
	int *soc,
	int soc_len)
{
	int	i
		,	fd
		,   width;
	fd_set	mask;

	fd = 0;
	FD_ZERO(&mask);
	for (i = 0; i < soc_len; i++){
		FD_SET(soc[i], &mask);
	}
	width = soc[soc_len - 1] + 1;

	if (select(width, &mask,NULL,NULL,NULL) < 0) {
		if (errno == EINTR) {
			return -1;
		}
		Error("select: %s", strerror(errno));
	} else {
		for (i = 0; i < soc_len; i++){
			if ( FD_ISSET(soc[i], &mask ) ) {
				if ( ( fd = accept(soc[i],0,0) ) < 0 ) {
					if ( errno == EINTR ) {
						return -1;
					}
					Error("accept: %s", strerror(errno));
				}
				break;
			}
		}
	}
	return fd;
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
