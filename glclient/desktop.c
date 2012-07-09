/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2009 NaCl & JMA (Japan Medical Association).
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
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/wait.h>
#include	<signal.h>
#include	<errno.h>

#define		DESKTOP_MAIN

#include	"glclient.h"
#include	"desktop.h"
#include	"gettext.h"
#include	"message.h"
#include	"debug.h"

static char *
GetSuffix(char *path)
{
	char *p,*q;
	char *ret = NULL;

	if (path != NULL) {
 		p = path;
		while ((q = strchr(p,'.')) != NULL) {
			q++;
			ret = q;
			p = q;
		}
	}
	return ret;
}

void
InitDesktop(void)
{
	FILE *fp;
	char fname[256];
	char buff[SIZE_BUFF+1];
	char *head;
	char *key;
	char *value;

	DesktopAppTable = NewNameHash();
	snprintf(fname, sizeof(fname), "%s/%s", 
		ConfDir, DESKTOP_LIST);
	fp = fopen(fname, "r");
	if (fp == NULL) {
		snprintf(fname, sizeof(fname), "%s/%s", 
			GLCLIENT_DATADIR, DESKTOP_LIST);
		fp = fopen(fname, "r");
	}
	if (fp != NULL) {
		while (fgets(buff, sizeof(buff), fp) != NULL) {
			head = strstr(buff, ":");
			if (head != NULL) {
				key = StrnDup(buff, head - buff);
				head += 1;
				value = StrnDup(head, strlen(head) - 1); /* chop */
				g_hash_table_insert(DesktopAppTable, key, value);
			}
		}
		fclose(fp);
	} else {
		MessageLog("cannot open applications list");
	}
}

extern int
CheckDesktop(char *filename)
{
	char *suffix;
	char *template;

	if (filename == NULL || strlen(filename) == 0) {
		return 0;
	}
	template = g_hash_table_lookup(DesktopAppTable, filename);
	if (template == NULL) {
		suffix = GetSuffix(filename);
		template = g_hash_table_lookup(DesktopAppTable, suffix);
    }
	if (template != NULL) {
		return 1;
	} else {
		return 0;
	}
}

static void
Exec(char *command,char *file)
{
	int argc;
	char *argv[DESKTOP_MAX_ARGC];
	char *p,*q;
	char message[SIZE_BUFF];

	argc = 0;
	p = q = command;
	while (1) {
		while (*q != ' ' && *q != 0) {q++;}
		argv[argc] = StrnDup(p, q - p);
		if (strstr(argv[argc],"%s")) {
			argv[argc] = file;
		}
		argc++;
		if (argc >= DESKTOP_MAX_ARGC) {
			MessageLogPrintf("can't exec [%s]; over argc size",command);
			return;
		}
		if (*q == 0) {
			argv[argc] = NULL;
			break;
		}
		while (*q == ' ') {q++;}
		p = q;
	}
	execvp(argv[0], argv);
    argv[0] = "zenity";
    argv[1] = "--error";
    argv[2] = "--title";
    argv[3] = _("Application start failure");
    argv[4] = "--text";
    sprintf(message, _("can't execute command.\\n\"%s\""),command);
    argv[5] = message;
    argv[6] = NULL;
	execvp(argv[0], argv);
	_exit(0);
}

extern void 
OpenDesktop(char *filename,LargeByteString *binary)
{
	int fd;
	int pid;
	int status;
	char *suffix;
	char *template;
	char path[SIZE_LONGNAME+1];
	struct sigaction sa;

	if (filename == NULL || strlen(filename) == 0) {
		return;
	}
	if (binary == NULL || LBS_Size(binary) == 0) {
		return;
	}

	snprintf(path,SIZE_LONGNAME,"%s/%s",TempDir, filename);
	path[SIZE_LONGNAME] = 0;
	fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd > 0) {
		if ( write(fd, LBS_Body(binary), LBS_Size(binary) ) > 0){
			close(fd);
		} else {
			MessageLogPrintf("write failure:%s",strerror(errno));
			return;
		}
	} else {
		MessageLogPrintf("open failure:%s",strerror(errno));
		return;
	}
	template = g_hash_table_lookup(DesktopAppTable, filename);
	if (template == NULL) {
		suffix = GetSuffix(filename);
		template = g_hash_table_lookup(DesktopAppTable, suffix);
    }
	if (template != NULL) {
		if ((pid = fork()) == 0) {
			memset(&sa, 0, sizeof(struct sigaction));
			sa.sa_handler = SIG_IGN;
			sa.sa_flags |= SA_RESTART;
			if (sigaction(SIGCHLD, &sa, NULL) != 0) {
				Error("sigaction(2) failure");
			}
			if ((pid = fork()) == 0) {
				Exec(template,path);
			} else if (pid < 0) {
				MessageLogPrintf("fork failure:%s",strerror(errno));
			} else {
				_exit(0);
			}
		} else if (pid < 0) {
			MessageLogPrintf("fork failure:%s",strerror(errno));
		} else {
			wait(&status);
		}
	}
}
