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

#ifdef HAVE_CRYPT_H
#include	<crypt.h>
#endif
#include	<sys/types.h>
#include	<sys/time.h>
#include	<unistd.h>
#include	<time.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<glib.h>
#include	<errno.h>
#include	<assert.h>

#include	"types.h"
#include	"libmondai.h"
#include	"auth.h"
#include	"message.h"
#include	"debug.h"

#undef	Error
#define	Error(msg)	printf("%s\n",(msg))

static	GHashTable		*PasswdTable;
static	PassWord		**PasswdPool;
static	int				cPass;

extern	const	char	*
AuthMakeSalt(void)
{
	struct	timeval	tv;
	static	char	result[40];

	result[0] = '\0';
	strcpy(result, "$1$");  /* magic for the new MD5 crypt() */

	/*
	 * Generate 8 chars of salt, the old crypt() will use only first 2.
	 */
	gettimeofday(&tv, (struct timezone *) 0);
	strcat(result, l64a(tv.tv_usec));
	strcat(result, l64a(tv.tv_sec + getpid() + clock()));

	if (strlen(result) > 3 + 8)  /* magic+salt */
		result[11] = '\0';

	return result;
}

extern	void
AuthClearEntry(void)
{
	int		i;

	if		(  PasswdTable  !=  NULL  ) {
		g_hash_table_destroy(PasswdTable);
		for	( i = 0 ; PasswdPool[i] != NULL ; i ++ ) {
			xfree(PasswdPool[i]);
		}
		xfree(PasswdPool);
	}
	PasswdTable = NewNameHash();
	cPass = 0;
	PasswdPool = (PassWord **)xmalloc(sizeof(PassWord *));
	PasswdPool[0] = NULL;
}

extern	void
AuthAddEntry(
	PassWord	*pw)
{
	PassWord	**pp;

	g_hash_table_insert(PasswdTable,pw->name,pw);
	pp = (PassWord **)xmalloc(sizeof(PassWord *) * ( cPass + 2 ));
	if		(  cPass  >  0  ) {
		memcpy(pp,PasswdPool,sizeof(PassWord *) * cPass);
	}
	xfree(PasswdPool);
	PasswdPool = pp;
	PasswdPool[cPass] = pw;
	PasswdPool[cPass+1] = NULL;
	cPass ++;
}

extern	void
AuthAddUser(
	char	*name,
	char	*pass,
	int		gid,
	int		uid,
	char	*other)
{
	PassWord	*pw;
	Bool	fNew;

dbgmsg(">AuthAddUser");
	if		(  ( pw = g_hash_table_lookup(PasswdTable,name) )  ==  NULL  ) {
		pw = New(PassWord);
		fNew = TRUE;
	} else {
		fNew = FALSE;
	}
	strncpy(pw->name,name,SIZE_USER);
	strcpy(pw->pass,pass);
	pw->gid = gid;
	pw->uid = uid;
	strncpy(pw->other,other,SIZE_OTHER);
	if		(  fNew  ) {
		AuthAddEntry(pw);
	}
dbgmsg("<AuthAddUser");
}

extern	void
AuthDelUser(
	char	*name)
{
	PassWord	*pw;

	if		(  ( pw = g_hash_table_lookup(PasswdTable,name) )  !=  NULL  ) {
		g_hash_table_remove(PasswdTable,name);
		*pw->name = 0;
	}
}

extern	PassWord	*
AuthAuthUser(
	char	*name,
	char	*pass)
{
	PassWord	*pw;

	if		(  ( pw = g_hash_table_lookup(PasswdTable,name) )  !=  NULL  ) {
		if		(  strcmp(pw->pass, crypt(pass,pw->pass))  !=  0  ) {
			pw = NULL;
		}
	}
	return	(pw);
}

extern	PassWord	*
AuthGetUser(
	char	*name)
{
	return	(g_hash_table_lookup(PasswdTable,name));
}

static Bool
scan_passwd_entry(const char *buf, PassWord *pw)
{
    int fields;
    static char *format = NULL;

    if (format == NULL){
        asprintf(&format, "%%%d[^:]:%%%d[^:]:%%d:%%d:%%%dc",
                 SIZE_USER, SIZE_PASS, SIZE_OTHER);
        if (format == NULL){
            Warning("can not make passwd format string");
            return FALSE;
        }
    }
    memset(pw, 0, sizeof(PassWord));
    fields = sscanf(buf, format,
        pw->name, pw->pass, &(pw->uid), &(pw->gid), pw->other);
    if (fields < 4) return FALSE;

    return TRUE;
}

extern	void
AuthLoadPasswd(char *fname)
{
	char        buff[SIZE_BUFF];
	FILE        *fp;
	PassWord    *pw;
	char        *p;
	int         line = 1;

    AuthClearEntry();
    if ((fp = fopen(fname,"r")) == NULL){
        Warning("[%s] can not open password file", fname);
        return;
    }
    while (fgets(buff, SIZE_BUFF, fp) != NULL){
        if ((p = strchr(buff, '\n')) == NULL){
            Warning("[%s:%d] input line is too long", fname, line);
            break;
        }
        *p = 0;
		pw = New(PassWord);
        if (scan_passwd_entry(buff, pw) != TRUE){
            Warning("[%s:%d] invalid format", fname, line);
            xfree(pw);
            break;
        }
        AuthAddEntry(pw);
        line++;
    }
    fclose(fp);
}

extern	Bool
AuthSingle(
	char	*fname,
	char	*name,
	char	*pass,
	char	*other)
{
	char	buff[SIZE_BUFF];
	FILE	*fp;
	char	*p;
	Bool	rc = FALSE;
	int		line = 1;
	PassWord	pw;

    if ((fp = fopen(fname,"r")) == NULL){
        Warning("[%s] can not open password file: %s", fname,strerror(errno));
        return FALSE;
    }

    while (fgets(buff, SIZE_BUFF, fp) != NULL){
        if ((p = strchr(buff, '\n')) == NULL){
            Warning("[%s:%d] input line is too long", fname, line);
            break;
        }
        *p = 0;
        if (scan_passwd_entry(buff, &pw) != TRUE){
            Warning("[%s:%d] invalid format", fname, line);
            break;
        }
        line++;
		if (strcmp(pw.name, name) != 0) continue;
        if (strcmp(pw.pass, crypt(pass, pw.pass)) != 0) continue;
		if (other != NULL) strcpy(other, pw.other);
        rc = TRUE;
        break;
    }
    fclose(fp);

    return rc;
}

extern	void
AuthSavePasswd(
	char	*fname)
{
	int		i;
	PassWord	*pw;
	FILE	*fp;

	if		(  ( fp = fopen(fname,"w") )  ==  NULL  ) {
		Error("can not open password file");
		return;
	}
	for	( i = 0 ; PasswdPool[i]  !=  NULL ; i ++ ) {
		pw = PasswdPool[i];
		if		(  *pw->name  !=  0  ) {
			fprintf(fp,"%s:%s:%d:%d:%s\n",
					pw->name,pw->pass,pw->uid,pw->gid,pw->other);
		}
	}
	fclose(fp);
}

static	int		MaxUID;
static	void
CheckMax(
	char	*name,
	PassWord	*pwd,
	void	*data)
{
	if		(  pwd->uid  >  MaxUID  ) {
		MaxUID = pwd->uid;
	}
}

extern	int
AuthMaxUID(void)
{
	MaxUID = 0;

	g_hash_table_foreach(PasswdTable,(GHFunc)CheckMax,NULL);
	return	(MaxUID);
}

#ifdef USE_SSL
/*
 * mapping X.509 subject to user name.
 */
static	GHashTable		*X509Table;

extern void
AuthAddX509(const char *user, const char *subject)
{
    if (!subject || subject[0] != '/'){
        Warning("invalid subject format: %s", subject);
        return;
    }
    assert(X509Table);
    AuthDelX509(user);
    AuthDelX509BySubject(subject);
    g_hash_table_insert(X509Table, g_strdup(subject), g_strdup(user));
}

extern void
AuthDelX509BySubject(const char *subject)
{
    char *u, *s;

    assert(X509Table);
    if (g_hash_table_lookup_extended(X509Table, subject, (gpointer*)&s,
                (gpointer*)&u)){
        g_hash_table_remove(X509Table, s);
        g_free(u);
        g_free(s);
    }
}

struct x509_copy_info {
    GHashTable  *table;
    const char *user;
};

static void
copy_except_one_user(gpointer key, gpointer value, gpointer user_data)
{
    struct x509_copy_info *tmp = (struct x509_copy_info*)user_data;

    if (strcmp(tmp->user, (char*)value) == 0){
        g_free(key);
        g_free(value);
        return;
    }
    g_hash_table_insert(tmp->table, key, value);
}

extern void
AuthDelX509(const char *user)
{
    struct x509_copy_info tmp;

    assert(X509Table);
    tmp.table = g_hash_table_new(g_str_hash, g_str_equal);
    tmp.user = user;
    g_hash_table_foreach(X509Table, copy_except_one_user, &tmp);
    g_hash_table_destroy(X509Table);
    X509Table = tmp.table;
}

static void
free_key_and_value(gpointer key, gpointer value, gpointer user_data)
{
    g_free(key);
    g_free(value);
}

extern void
AuthClearX509()
{
    assert(X509Table);
    g_hash_table_foreach_remove(X509Table, free_key_and_value, NULL);
}

extern  Bool
AuthLoadX509(const char *file)
{
    char buf[1000], user[SIZE_USER+1], subject[1000];
    char format[64], *p;
    int line = 1;
    FILE *fp;
    Bool result = TRUE;

    if (X509Table == NULL)
        X509Table = g_hash_table_new(g_str_hash, g_str_equal);
    AuthClearX509();
    if ((fp = fopen(file, "r")) == NULL){
        return TRUE;
    }
    snprintf(format, sizeof(format),
             "%%%ds %%%dc", sizeof(user)-1, sizeof(subject)-1);
    while (fgets(buf, sizeof(buf), fp) != NULL){
		if ( ( buf[0] == '\n' ) || (buf[0] == '#') ) {
			continue;
		}
        if ((p = strchr(buf, '\n')) == NULL){
            Warning("[%s:%d] input line is too long", file, line);
            result = FALSE;
            break;
        }
        *p = '\0';
        memset(user, 0, sizeof(user));
        memset(subject, 0, sizeof(subject));
        if (sscanf(buf, format, user, subject) != 2 || subject[0] != '/'){
            Warning("[%s:%d] invalid format", file, line);
            result = FALSE;
            break;
        }
        AuthAddX509(user, subject);
        line++;
    }
    fclose(fp);

    return result;
}

extern void
AuthSaveX509(const char *file)
{
    FILE *fp;

    if ((fp = fopen(file, "w")) == NULL){
        Warning("can not open password file: %s", strerror(errno));
        return;
    }
    AuthDumpX509(fp);
    fclose(fp);
}

static void
print_key_and_value(gpointer key, gpointer value, gpointer fp)
{
    fprintf((FILE*)fp, "%s\t%s\n", value, key);
}

extern  void
AuthDumpX509(FILE *fp)
{
    assert(X509Table);
    g_hash_table_foreach(X509Table, print_key_and_value, fp);
}

extern  Bool
AuthX509(const char *subject, char *user)
{
    const char *tmp;

    assert(X509Table);
    if ((tmp = (const char*)g_hash_table_lookup(X509Table, subject)) == NULL){
        Warning("invalid subject: %s", subject);
        return FALSE;
    }
    snprintf(user, SIZE_USER, "%s", tmp);

    return TRUE;
}
#endif /* USE_SSL */
