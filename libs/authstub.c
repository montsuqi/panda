/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 1998-1999 Ogochan.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<signal.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<glib.h>

#include	"const.h"
#include	"types.h"
#include	"libmondai.h"
#include	"auth.h"
#include	"comm.h"
#include	"socket.h"
#include	"option.h"
#include	"debug.h"

extern	Bool
AuthUser(
	URL		*auth,
	char	*user,
	char	*pass,
	char	*other,
	char	*id)
{
	struct	timeval	tv;
	int		fh;
	NETFILE	*fp;
	Bool	rc;
	char	buff[SIZE_OTHER+1];

ENTER_FUNC;
	if		(  strcmp(auth->protocol,"trust")  ==  0  ) {
		rc = TRUE;
	} else
	if		(  !stricmp(auth->protocol,"glauth")  ) {
		fh = ConnectIP_Socket(auth->port,SOCK_STREAM,auth->host);
		if	( fh > 0) {
			fp = SocketToNet(fh);
			SendString(fp,user);
			SendString(fp,pass);
			if		(  ( rc = RecvBool(fp) )  ) {
				RecvnString(fp, sizeof(buff), buff);
				if		(  other  !=  NULL  ) {
					strcpy(other,buff);
				}
			}
			CloseNet(fp);
		} else{
			Warning("can not connect glauth server");
			rc = FALSE;
		}
	} else
	if		(  !stricmp(auth->protocol,"file")  ) {
		rc = AuthSingle(auth->file,user,pass,NULL);
	} else {
		rc = FALSE;
	}
	if		(  id  !=  NULL  ) {
		if		(  rc  ) {
			gettimeofday(&tv, (struct timezone *) 0);
			strcpy(id,crypt(user,l64a(tv.tv_sec + getpid() + clock())));
		} else {
			*id = 0;
		}
	}
#ifdef	DEBUG
	if		(  rc ) {
		printf("OK\n");
	} else {
		printf("NG\n");
	}
#endif
LEAVE_FUNC;
	return	(rc);
}

extern	Bool
CheckAuthID(
	URL		*zone,
	char	*id,
	char	*user,
	char	*other)
{
	Bool	fOK;
	char	fname[SIZE_LONGNAME+1];
	char	buff[SIZE_BUFF];
	FILE	*fp;
	char	*p;

	if		(  !stricmp(zone->protocol,"file")  ) {
		sprintf(fname,"%s/%s",zone->file,id);
		if		(  ( fp = fopen(fname,"r") )  !=  NULL  ) {
			if		(  fgets(buff, SIZE_BUFF, fp)  !=  NULL  ) {
				StringChop(buff);
				if		(  ( p = strchr(buff,':') )  !=  NULL  ) {
					*p = 0;
					if		(  user  !=  NULL  ) {
						strcpy(user,buff);
					}
					if		(  other  !=  NULL  ) {
						strcpy(other,p+1);
					}
				} else {
					if		(  user  !=  NULL  ) {
						strcpy(user,buff);
					}
					if		(  other  !=  NULL  ) {
						*other = 0;
					}
				}
				fOK = TRUE;
			} else {
				fOK = FALSE;
			}
			fclose(fp);
		} else {
			fOK = FALSE;
		}
	} else {
		fOK = FALSE;
	}
	if		(  !fOK  ) {
		if		(  user  !=  NULL  ) {
			*user = 0;
		}
		if		(  other  !=  NULL  ) {
			*other = 0;
		}
	}
	return	(fOK);
}	

extern	Bool
SaveAuthID(
	URL		*zone,
	char	*id,
	char	*user,
	char	*other)
{
	Bool	fOK;
	char	fname[SIZE_LONGNAME+1];
	FILE	*fp;

	if		(  !stricmp(zone->protocol,"file")  ) {
		sprintf(fname,"%s/%s",zone->file,id);
		if		(  ( fp = fopen(fname,"w") )  !=  NULL  ) {
			if		(  user  !=  NULL  ) {
				if		(  other  !=  NULL  ) {
					fprintf(fp,"%s:%s",user,other);
				} else {
					fprintf(fp,"%s",user);
				}
			}
			fOK = TRUE;
			fclose(fp);
		} else {
			fOK = FALSE;
		}
	} else {
		fOK = FALSE;
	}
	return	(fOK);
}

#ifdef	MAIN
static	char		*AuthURL;
static	ARG_TABLE	option[] = {
	{	"auth",		STRING,		TRUE,	(void*)&AuthURL,
		"auth server URL"		 						},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	AuthURL = "glauth://localhost:9002";
}

extern	int
main(
	int		argc,
	char	**argv)
{
	char	other[SIZE_OTHER+1];

	SetDefault();
	(void)GetOption(option,argc,argv);
	ParseURL(&Auth,AuthURL);
	if		(  AuthUser(argv[1],argv[2],other)  ) {
		printf("OK\n");
		printf("[%s]\n",other);
	} else {
		printf("NG\n");
	}
}
#endif
